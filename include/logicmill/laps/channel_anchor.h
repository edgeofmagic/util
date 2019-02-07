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

#ifndef LOGICMILL_LAPS_CHANNEL_ANCHOR_H
#define LOGICMILL_LAPS_CHANNEL_ANCHOR_H

#include <list>
#include <logicmill/async/channel.h>
#include <logicmill/async/event_flow.h>
#include <logicmill/async/loop.h>
#include <logicmill/async/options.h>
#include <logicmill/laps/types.h>

namespace logicmill
{
namespace laps
{

class channel_anchor : public flow::stackable<stream_duplex_top, channel_anchor>,
					   public face<channel_anchor, channel_anchor>
{
public:
	using connector_base = flow::stackable<stream_duplex_top, channel_anchor>;
	using emitter<const_buffer_event>::emit;
	using emitter<control_event>::emit;
	using emitter<error_event>::emit;
	using connector_base::get_surface;
	using base = face<channel_anchor, channel_anchor>;

	static constexpr std::size_t default_queue_limit = 1UL << 24;    // 16 Mb

	channel_anchor(std::size_t write_queue_limit = default_queue_limit)
		: connector_base{},
		  base{this},
		  m_channel{nullptr},
		  m_loop{nullptr},
		  m_write_queue_full{false},
		  m_write_queue_limit{write_queue_limit}
	{}

	channel_anchor(async::channel::ptr chan, std::size_t write_queue_limit = default_queue_limit)
		: connector_base{},
		  base{this},
		  m_channel{chan},
		  m_loop{m_channel->loop()},
		  m_write_queue_full{false},
		  m_write_queue_limit{write_queue_limit}
	{}

	channel_anchor(channel_anchor&& rhs)
		: connector_base{},
		  base{this},
		  m_channel{std::move(rhs.m_channel)},
		  m_loop{std::move(rhs.m_loop)},
		  m_write_queue_full{rhs.m_write_queue_full},
		  m_write_queue_limit{rhs.m_write_queue_limit}
	{}

	channel_anchor(channel_anchor const& rhs) = delete;

	void
	connect(async::channel::ptr chan)
	{
		m_loop    = chan->loop();
		m_channel = chan;
		// TODO: add check for connectedness to channel, employ here
		// for now, assume chan is connected
		propagate_start();
	}

	void
	connect(async::loop::ptr lp, async::options const& opts)
	{
		m_loop = lp;
		m_loop->connect_channel(opts, [=](async::channel::ptr cp, std::error_code err) {
			if (err)
			{
				emit<error_event>(err);
			}
			else
			{
				m_channel = cp;
				propagate_start();
			}
		});
	}

	stream_duplex_top&
	get_top()
	{
		return get_surface<stream_duplex_top>();
	}

	void
	dispatch_error(std::error_code err)
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](async::loop::ptr lp) { emit<error_event>(err); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched emit as last ditch effort
		{
			emit<error_event>(loop_error);
		}
	}

	void
	dispatch_start()
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](async::loop::ptr lp) { propagate_start(); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched emit error as last ditch effort
		{
			emit<error_event>(loop_error);
		}
	}

	void
	dispatch_stop()
	{
		std::error_code loop_error;
		m_loop->dispatch(loop_error, [=](async::loop::ptr lp) { propagate_stop(); });
		if (loop_error)    // probably screwed beyond repair; try non-dispatched emit error as last ditch effort
		{
			emit<error_event>(loop_error);
		}
	}

	void
	on_write_complete(std::deque<util::mutable_buffer>&& bufs, std::error_code err)
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
						[=](async::channel::ptr chan, util::mutable_buffer&& buf, std::error_code err) {
							this->on_write_complete(std::move(buf), err);
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
	on_write_complete(util::mutable_buffer&& buf, std::error_code err)
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
						[=](async::channel::ptr chan, util::mutable_buffer&& buf, std::error_code err) {
							this->on_write_complete(std::move(buf), err);
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
	on(mutable_data_event, std::deque<util::mutable_buffer>&& bufs)
	{
		if (m_write_queue_full)
		{
			while (!bufs.empty())
			{
				m_local_write_queue.emplace_back(std::move(bufs.front()));
				bufs.pop_front();
			}
		}
		else if (m_channel->get_queue_size() >= m_write_queue_limit)
		{
			m_write_queue_full = true;
			while (!bufs.empty())
			{
				m_local_write_queue.emplace_back(std::move(bufs.front()));
				bufs.pop_front();
			}
			emit<control_event>(control_state::stop);
		}
		else
		{
			std::error_code err;
			m_channel->write(
					std::move(bufs),
					err,
					[=](async::channel::ptr chan, std::deque<util::mutable_buffer>&& bufs, std::error_code err) {
						this->on_write_complete(std::move(bufs), err);
					});
		}
	}

	void
	on(mutable_buffer_event, util::mutable_buffer&& buf)
	{
		if (m_write_queue_full)
		{
			m_local_write_queue.emplace_back(std::move(buf));
		}
		else if (m_channel->get_queue_size() >= m_write_queue_limit)
		{
			m_write_queue_full = true;
			m_local_write_queue.emplace_back(std::move(buf));
			emit<control_event>(control_state::stop);
		}
		else
		{
			std::error_code err;
			m_channel->write(
					std::move(buf),
					err,
					[=](async::channel::ptr chan, util::mutable_buffer&& buf, std::error_code err) {
						this->on_write_complete(std::move(buf), err);
					});
		}
	}

	void
	on(control_event, control_state cstate)
	{
		if (cstate == control_state::start)
		{
			std::error_code err;
			m_channel->start_read(
					err, [=](async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err) {
						emit<const_buffer_event>(std::move(buf));
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
	async::channel::ptr              m_channel;
	async::loop::ptr                 m_loop;
	bool                             m_write_queue_full;
	std::size_t                      m_write_queue_limit;
	std::deque<util::mutable_buffer> m_local_write_queue;
};

}    // namespace laps
}    // namespace logicmill

#endif    // LOGICMILL_LAPS_CHANNEL_ANCHOR_H