/*
 * The MIT License
 *
 * Copyright 2017 David Curtis.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef LOGICMILL_ASYNC_TCP_H
#define LOGICMILL_ASYNC_TCP_H

#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/async/options.h>
#include <logicmill/async/event.h>
#include <list>

namespace logicmill
{
namespace async
{


class tcp_channel : public channel
{
public:
	using ptr = std::shared_ptr<tcp_channel>;
};

class tcp_listener : public listener
{
public:
	using ptr = std::shared_ptr<tcp_listener>;
};

enum class stream_event
{
	data,
	control
};

enum class control_state
{
	stop,
	start,
	error
};

using data_event
		= event<stream_event, stream_event::data, std::deque<bstream::mutable_buffer>&&>;

using control_event = event<stream_event, stream_event::control, control_state, std::error_code>;


using data_in_connector = connector<sink<data_event>, source<control_event>>;
using data_out_connector = complement<data_in_connector>::type;


class tcp_stream : public connectable<data_out_connector, tcp_stream>,
				   public connectable<data_in_connector, tcp_stream>
{
public:

	using ptr = std::shared_ptr<tcp_stream>;

	using out_base = connectable<data_out_connector, tcp_stream>;
	using in_base = connectable<data_in_connector, tcp_stream>;

	using source_base<data_event>::send;
	using source_base<control_event>::send;

	using out_base::get_connector;
	using in_base::get_connector;

	static constexpr std::size_t default_queue_limit = 1UL << 24; // 16 Mb

	tcp_stream(channel::ptr chan, std::size_t write_queue_limit = default_queue_limit)
		: m_channel{chan}, m_loop{m_channel->loop()}, m_write_queue_full{false}, m_write_queue_limit{write_queue_limit}
	{}

	void
	dispatch_error(std::error_code err)
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](loop::ptr lp)
		{
			send<control_event>(control_state::error, err);
		});
		if (loop_error) // probably screwed beyond repair; try non-dispatched send as last ditch effort
		{
			send<control_event>(control_state::error, err);
		}
	}

	void dispatch_start()
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](loop::ptr lp)
		{
			send<control_event>(control_state::start, std::error_code{});
		});
		if (loop_error) // probably screwed beyond repair; try non-dispatched send error as last ditch effort
		{
			send<control_event>(control_state::error, loop_error);
		}
	}

	void dispatch_stop()
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](loop::ptr lp)
		{
			send<control_event>(control_state::stop, std::error_code{});
		});
		if (loop_error) // probably screwed beyond repair; try non-dispatched send error as last ditch effort
		{
			send<control_event>(control_state::error, loop_error);
		}
	}

	void
	on_write_complete(std::deque<bstream::mutable_buffer>&& bufs, std::error_code err)
	{
		if (err)
		{
			dispatch_error(err);
			m_channel->close();
			goto exit;
		}

		if (m_write_queue_full)
		{
			while (m_channel->get_queue_size() < m_write_queue_limit && !m_local_write_queue.empty())
			{
				m_channel->write(std::move(m_local_write_queue.front()), err, 
				[=] (channel::ptr chan, std::deque<bstream::mutable_buffer>&& bufs, std::error_code err)
				{
					this->on_write_complete(std::move(bufs), err);
				});
				if (err)
				{
					dispatch_error(err);
					m_channel->close();
					goto exit;
				}
			}
			if (m_channel->get_queue_size() < m_write_queue_limit)
			{
				m_write_queue_full = false;
				dispatch_start();
			}
		}

	exit:
		return;

	}

	void
	on(data_event, std::deque<bstream::mutable_buffer>&& bufs)
	{
		if (m_write_queue_full)
		{
			m_local_write_queue.emplace_back(std::move(bufs));
		}
		else if (m_channel->get_queue_size() >= m_write_queue_limit)
		{
			m_write_queue_full = true;
			m_local_write_queue.emplace_back(std::move(bufs));
			send<control_event>(control_state::stop, std::error_code{});
		}
		else
		{
			std::error_code err;
			m_channel->write(std::move(bufs), err, [=](channel::ptr chan, std::deque<bstream::mutable_buffer>&& bufs, std::error_code err)
			{
				this->on_write_complete(std::move(bufs), err);
			});
		}
	}

	void
	on(control_event, control_state cstate, std::error_code err)
	{
		if (cstate == control_state::start)
		{
			std::error_code err;
			m_channel->start_read(err, [=](channel::ptr const& chan, bstream::const_buffer&& buf, std::error_code err)
			{
				std::deque<bstream::mutable_buffer> bufs;
				bufs.emplace_back(bstream::mutable_buffer{std::move(buf)});
				send<data_event>(std::move(bufs));
			});
		}
		else if (cstate == control_state::stop)
		{
			m_channel->stop_read();
		}
		else // shouldn't happen, no downstream errors
		{
			assert(cstate == control_state::error);
			assert(false);
		}
	}

private:
	channel::ptr m_channel;
	loop::ptr m_loop;
	bool m_write_queue_full;
	std::size_t m_write_queue_limit;
	std::list<std::deque<bstream::mutable_buffer>> m_local_write_queue;
};

class tcp_stream_driver : public connectable<data_out_connector, tcp_stream_driver>,
				   public connectable<data_in_connector, tcp_stream_driver>
{
public:

	using ptr = std::shared_ptr<tcp_stream_driver>;

	using out_base = connectable<data_out_connector, tcp_stream_driver>;
	using in_base = connectable<data_in_connector, tcp_stream_driver>;

	using data_source_base = typename out_base::template source_base<data_event>;
	// using control_sink_base = typename out_base::template sink_base<control_event>;

	// using data_sink_base = typename in_base::template sink_base<data_event>;
	using control_source_base = typename in_base::template source_base<control_event>;

	using source_base<data_event>::send;
	using source_base<control_event>::send;

	using out_base::get_connector;
	using in_base::get_connector;
	using out_base::mate;
	using in_base::mate;

	using read_handler = std::function<void(std::deque<bstream::mutable_buffer>&& bufs)>;
	using control_handler = std::function<void(control_state, std::error_code)>;

	// tcp_stream_driver(tcp_stream::ptr strm) : m_stream{strm}
	// {
		// source_base<data_event>::get_source<data_event>().fit(m_stream->sink_base<data_event, tcp_stream>::get_sink<data_event>());
		// source_base<control_event>::get_source<control_event>().fit(m_stream->sink_base<control_event, tcp_stream>::get_sink<control_event>());
		// m_stream->source_base<data_event>::get_source<data_event>().fit(this->sink_base<data_event, tcp_stream_driver>::get_sink<data_event>());
		// m_stream->source_base<control_event>::get_source<control_event>().fit(this->sink_base<control_event, tcp_stream_driver>::get_sink<control_event>());


		// get_connector<data_in_connector>().mate(m_stream->get_connector<data_out_connector>());
		// get_connector<data_out_connector>().mate(m_stream->get_connector<data_in_connector>());

		// out_base::mate(*m_stream);
		// in_base::mate(*m_stream);
	// }

	tcp_stream_driver(channel::ptr chan) : m_stream{chan} 
	{
		out_base::mate(m_stream);
		in_base::mate(m_stream);
	}

	~tcp_stream_driver()
	{
		m_read_handler = nullptr;
		m_control_handler = nullptr;
		// m_stream.close();
	}

	void
	write(std::deque<bstream::mutable_buffer>&& bufs)
	{
		send<data_event>(std::move(bufs));
	}


	template<class T>
	std::enable_if_t<std::is_convertible<T, read_handler>::value>
	on_read(T&& handler)
	{
		m_read_handler = std::forward<T>(handler);
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, read_handler>::value>
	start_read(T&& handler)
	{
		m_read_handler = std::forward<T>(handler);
		send<control_event>(control_state::start, std::error_code{});
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, control_handler>::value>
	on_control(T&& handler)
	{
		m_control_handler = std::forward<T>(handler);
	}

	void
	stop_read()
	{
		send<control_event>(control_state::stop, std::error_code{});
	}

	void
	on(data_event, std::deque<bstream::mutable_buffer>&& bufs)
	{
		m_read_handler(std::move(bufs));
	}

	void
	on(control_event, control_state cstate, std::error_code err)
	{
		m_control_handler(cstate, err);
	}


private:
	tcp_stream m_stream;
	read_handler m_read_handler;
	control_handler m_control_handler;
};

}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ASYNC_TCP_H