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

#include <list>
#include <logicmill/async/channel.h>
#include <logicmill/async/event.h>
#include <logicmill/async/loop.h>
#include <logicmill/async/options.h>

namespace logicmill
{
namespace async
{

class tcp_channel : public channel
{
public:
	using ptr = std::shared_ptr<tcp_channel>;
};

class tcp_acceptor : public acceptor
{
public:
	using ptr = std::shared_ptr<tcp_acceptor>;
};

enum class stream_event
{
	mutable_data,
	const_data,
	control,
	error
};

enum class control_state
{
	stop,
	start
};

using mutable_data_event = event<stream_event, stream_event::mutable_data, std::deque<bstream::mutable_buffer>&&>;

using const_data_event = event<stream_event, stream_event::const_data, std::deque<bstream::const_buffer>&&>;

using control_event = event<stream_event, stream_event::control, control_state>;

using error_event = event<stream_event, stream_event::error, std::error_code>;

using mutable_data_in_connector
		= connector<sink<mutable_data_event>, source<control_event>, source<error_event>>;

using mutable_data_out_connector = complement<mutable_data_in_connector>::type;

using const_data_in_connector
		= connector<sink<const_data_event>, source<control_event>, source<error_event>>;

using const_data_out_connector = complement<const_data_in_connector>::type;

using stream_duplex_top    = surface<mutable_data_in_connector, const_data_out_connector>;
using stream_duplex_bottom = complement<stream_duplex_top>::type;

class tcp_anchor : public stackable<stream_duplex_top, tcp_anchor>
{
public:
	using base = stackable<stream_duplex_top, tcp_anchor>;

	using emitter<const_data_event>::send;
	using emitter<control_event>::send;
	using emitter<error_event>::send;
	using base::get_surface;

	static constexpr std::size_t default_queue_limit = 1UL << 24;    // 16 Mb

	tcp_anchor(channel::ptr chan, std::size_t write_queue_limit = default_queue_limit)
		: m_channel{chan}, m_loop{m_channel->loop()}, m_write_queue_full{false}, m_write_queue_limit{write_queue_limit}
	{}

	tcp_anchor(tcp_anchor&& rhs)
		: base{},
		  m_channel{std::move(rhs.m_channel)},
		  m_loop{std::move(rhs.m_loop)},
		  m_write_queue_full{rhs.m_write_queue_full},
		  m_write_queue_limit{rhs.m_write_queue_limit}
	{}

	tcp_anchor(tcp_anchor const& rhs)
		: base{},
		  m_channel{rhs.m_channel},
		  m_loop{rhs.m_loop},
		  m_write_queue_full{rhs.m_write_queue_full},
		  m_write_queue_limit{rhs.m_write_queue_limit}
	{}

	stream_duplex_top&
	get_top()
	{
		return get_surface<stream_duplex_top>();
	}

	void
	dispatch_error(std::error_code err)
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](loop::ptr lp) { send<error_event>(err); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched send as last ditch effort
		{
			send<error_event>(loop_error);
		}
	}

	void
	dispatch_start()
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](loop::ptr lp) { send<control_event>(control_state::start); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched send error as last ditch effort
		{
			send<error_event>(loop_error);
		}
	}

	void
	dispatch_stop()
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](loop::ptr lp) { send<control_event>(control_state::stop); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched send error as last ditch effort
		{
			send<error_event>(loop_error);
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
				m_channel->write(
						std::move(m_local_write_queue.front()),
						err,
						[=](channel::ptr chan, std::deque<bstream::mutable_buffer>&& bufs, std::error_code err) {
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
	on(mutable_data_event, std::deque<bstream::mutable_buffer>&& bufs)
	{
		if (m_write_queue_full)
		{
			m_local_write_queue.emplace_back(std::move(bufs));
		}
		else if (m_channel->get_queue_size() >= m_write_queue_limit)
		{
			m_write_queue_full = true;
			m_local_write_queue.emplace_back(std::move(bufs));
			send<control_event>(control_state::stop);
		}
		else
		{
			std::error_code err;
			m_channel->write(
					std::move(bufs),
					err,
					[=](channel::ptr chan, std::deque<bstream::mutable_buffer>&& bufs, std::error_code err) {
						this->on_write_complete(std::move(bufs), err);
					});
		}
	}

	void
	on(control_event, control_state cstate)
	{
		if (cstate == control_state::start)
		{
			std::error_code err;
			m_channel->start_read(err, [=](channel::ptr const& chan, bstream::const_buffer&& buf, std::error_code err) {
				std::deque<bstream::const_buffer> bufs;
				bufs.emplace_back(std::move(buf));
				send<const_data_event>(std::move(bufs));
			});
		}
		else    // if (cstate == control_state::stop)
		{
			assert(cstate == control_state::stop);
			m_channel->stop_read();
		}
	}

	void
	on(error_event, std::error_code err)
	{
		// shouldn't happen, no downstream errors
		assert(false);
	}


private:
	channel::ptr                                   m_channel;
	loop::ptr                                      m_loop;
	bool                                           m_write_queue_full;
	std::size_t                                    m_write_queue_limit;
	std::list<std::deque<bstream::mutable_buffer>> m_local_write_queue;
};

class stream_driver : public stackable<stream_duplex_bottom, stream_driver>
{
public:
	using base = stackable<stream_duplex_bottom, stream_driver>;
	using emitter<mutable_data_event>::send;
	using emitter<control_event>::send;
	using emitter<error_event>::send;

	using base::get_surface;

	using read_handler    = std::function<void(std::deque<bstream::const_buffer>&&)>;
	using control_handler = std::function<void(control_state)>;
	using error_handler   = std::function<void(std::error_code)>;

	stream_duplex_bottom&
	get_bottom()
	{
		return get_surface<stream_duplex_bottom>();
	}

	stream_driver() : base{} {}

	stream_driver(stream_driver&& rhs)
		: base{},
		  m_read_handler{std::move(rhs.m_read_handler)},
		  m_control_handler{std::move(rhs.m_control_handler)},
		  m_error_handler{std::move(rhs.m_error_handler)}
	{}

	stream_driver(stream_driver const& rhs)
		: base{},
		  m_read_handler{rhs.m_read_handler},
		  m_control_handler{rhs.m_control_handler},
		  m_error_handler{rhs.m_error_handler}
	{}

	~stream_driver()
	{
		m_read_handler    = nullptr;
		m_control_handler = nullptr;
		m_error_handler   = nullptr;
	}

	void
	write(std::deque<bstream::mutable_buffer>&& bufs)
	{
		send<mutable_data_event>(std::move(bufs));
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
		send<control_event>(control_state::start);
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, control_handler>::value>
	on_control(T&& handler)
	{
		m_control_handler = std::forward<T>(handler);
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, error_handler>::value>
	on_error(T&& handler)
	{
		m_error_handler = std::forward<T>(handler);
	}

	void
	stop_read()
	{
		send<control_event>(control_state::stop);
	}

	void
	on(const_data_event, std::deque<bstream::const_buffer>&& bufs)
	{
		m_read_handler(std::move(bufs));
	}

	void
	on(control_event, control_state cstate)
	{
		m_control_handler(cstate);
	}

	void
	on(error_event, std::error_code err)
	{
		m_error_handler(err);
	}

private:
	read_handler    m_read_handler;
	control_handler m_control_handler;
	error_handler   m_error_handler;
};

#if 0

class tcp_stream : public connectable<data_out_connector, tcp_stream>, public connectable<data_in_connector, tcp_stream>
{
public:
	using ptr = std::shared_ptr<tcp_stream>;

	using out_base = connectable<data_out_connector, tcp_stream>;
	using in_base  = connectable<data_in_connector, tcp_stream>;

	using emitter<data_event>::send;
	using emitter<control_event>::send;

	using out_base::get_connector;
	using in_base::get_connector;

	static constexpr std::size_t default_queue_limit = 1UL << 24;    // 16 Mb

	tcp_stream(channel::ptr chan, std::size_t write_queue_limit = default_queue_limit)
		: m_channel{chan}, m_loop{m_channel->loop()}, m_write_queue_full{false}, m_write_queue_limit{write_queue_limit}
	{}

	void
	dispatch_error(std::error_code err)
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](loop::ptr lp) { send<control_event>(control_state::error, err); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched send as last ditch effort
		{
			send<control_event>(control_state::error, err);
		}
	}

	void
	dispatch_start()
	{
		std::error_code loop_error;
		m_loop->dispatch(
				loop_error, [=](loop::ptr lp) { send<control_event>(control_state::start, std::error_code{}); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched send error as last ditch effort
		{
			send<control_event>(control_state::error, loop_error);
		}
	}

	void
	dispatch_stop()
	{
		std::error_code loop_error;
		m_loop->dispatch(
				loop_error, [=](loop::ptr lp) { send<control_event>(control_state::stop, std::error_code{}); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched send error as last ditch effort
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
				m_channel->write(
						std::move(m_local_write_queue.front()),
						err,
						[=](channel::ptr chan, std::deque<bstream::mutable_buffer>&& bufs, std::error_code err) {
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
			m_channel->write(
					std::move(bufs),
					err,
					[=](channel::ptr chan, std::deque<bstream::mutable_buffer>&& bufs, std::error_code err) {
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
			m_channel->start_read(err, [=](channel::ptr const& chan, bstream::const_buffer&& buf, std::error_code err) {
				std::deque<bstream::mutable_buffer> bufs;
				bufs.emplace_back(bstream::mutable_buffer{std::move(buf)});
				send<data_event>(std::move(bufs));
			});
		}
		else if (cstate == control_state::stop)
		{
			m_channel->stop_read();
		}
		else    // shouldn't happen, no downstream errors
		{
			assert(cstate == control_state::error);
			assert(false);
		}
	}

private:
	channel::ptr                                   m_channel;
	loop::ptr                                      m_loop;
	bool                                           m_write_queue_full;
	std::size_t                                    m_write_queue_limit;
	std::list<std::deque<bstream::mutable_buffer>> m_local_write_queue;
};

class tcp_stream_driver : public connectable<data_out_connector, tcp_stream_driver>,
						  public connectable<data_in_connector, tcp_stream_driver>
{
public:
	using ptr = std::shared_ptr<tcp_stream_driver>;

	using out_base = connectable<data_out_connector, tcp_stream_driver>;
	using in_base  = connectable<data_in_connector, tcp_stream_driver>;

	using data_emitter    = typename out_base::template emitter<data_event>;
	using control_emitter = typename in_base::template emitter<control_event>;

	using emitter<data_event>::send;
	using emitter<control_event>::send;

	using out_base::get_connector;
	using in_base::get_connector;
	using out_base::mate;
	using in_base::mate;

	using read_handler    = std::function<void(std::deque<bstream::mutable_buffer>&& bufs)>;
	using control_handler = std::function<void(control_state, std::error_code)>;

	tcp_stream_driver(channel::ptr chan) : m_stream{chan}
	{
		out_base::mate(m_stream);
		in_base::mate(m_stream);
	}

	~tcp_stream_driver()
	{
		m_read_handler    = nullptr;
		m_control_handler = nullptr;
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
	tcp_stream      m_stream;
	read_handler    m_read_handler;
	control_handler m_control_handler;
};

#endif

}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ASYNC_TCP_H