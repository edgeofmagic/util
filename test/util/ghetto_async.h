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

#ifndef UTIL_TEST_GHETTO_ASYNC_H
#define UTIL_TEST_GHETTO_ASYNC_H

#include <chrono>
#include <functional>
#include <map>
#include <thread>

namespace ghetto_async
{

using void_handler    = std::function<void()>;
using err_handler     = std::function<void(std::error_code const&)>;
using event_id_type   = std::uint32_t;
using time_point_type = std::chrono::time_point<std::chrono::system_clock>;
using millisecs       = std::chrono::milliseconds;

class timer_impl;

using timer_type = std::shared_ptr<timer_impl>;

class event_base
{
public:
	using ptr = std::unique_ptr<event_base>;

	virtual ~event_base() {}

	event_base(event_id_type id, time_point_type expiry) : m_id{id}, m_expiry{expiry} {}

	virtual void
	trigger(std::error_code const& err)
			= 0;

	time_point_type
	expiry() const
	{
		return m_expiry;
	}

	event_id_type
	id() const
	{
		return m_id;
	}

private:
	event_id_type   m_id;
	time_point_type m_expiry;
};

class dispatched_event : public event_base
{
public:
	dispatched_event(event_id_type id, void_handler&& handler)
		: event_base{id, time_point_type{}}, m_handler{std::move(handler)}
	{}

	virtual void
	trigger(std::error_code const& err) override
	{
		auto handler{std::move(m_handler)};
		handler();
	}

private:
	void_handler m_handler;
};

class scheduled_event : public event_base
{
public:
	scheduled_event(event_id_type id, time_point_type expiry, err_handler&& handler)
		: event_base{id, expiry}, m_handler{std::move(handler)}
	{}

	virtual void
	trigger(std::error_code const& err) override
	{
		auto handler{std::move(m_handler)};
		handler(err);
	}

private:
	err_handler m_handler;
};

class event_queue
{
public:
	using event_map_type       = std::multimap<time_point_type, event_base::ptr>;
	using event_map_iter       = event_map_type::iterator;
	using event_map_const_iter = event_map_type::const_iterator;

	void
	add_event(event_base::ptr&& event)
	{
		time_point_type expiry = event->expiry();
		m_events.emplace(expiry, std::move(event));    // parameter eval ordering?
	}

	void
	cancel_event(event_id_type id)
	{
		for (event_map_iter it = m_events.begin(); it != m_events.end(); ++it)
		{
			if (it->second->id() == id)
			{
				auto event = std::move(it->second);
				m_events.erase(it);
				event->trigger(make_error_code(std::errc::operation_canceled));
				break;
			}
		}
	}

	time_point_type
	front_expiry() const
	{
		event_map_const_iter front = m_events.cbegin();
		assert(front != m_events.cend());
		return front->second->expiry();
	}

	void
	pop_front_and_trigger()
	{
		event_map_iter front = m_events.begin();
		assert(front != m_events.end());
		auto event = std::move(front->second);
		m_events.erase(front);
		event->trigger(make_error_code(std::errc::timed_out));
	}

	bool
	already_in_queue(event_id_type id)
	{
		bool result{false};
		for (event_map_iter it = m_events.begin(); it != m_events.end(); ++it)
		{
			if (it->second->id() == id)
			{
				result = true;
				break;
			}
		}
		return result;
	}

	unsigned
	cancel_all()
	{
		unsigned count{0};
		for (event_map_iter it = m_events.begin(); it != m_events.end(); ++it)
		{
			it->second->trigger(make_error_code(std::errc::operation_canceled));
			++count;
		}
		m_events.clear();
		return count;
	}

	bool
	empty() const
	{
		return m_events.empty();
	}

private:
	event_map_type m_events;
};

class loop_impl;

class timer_impl
{
public:
	timer_impl(std::shared_ptr<loop_impl> const& lp, event_id_type id) : m_loop{lp}, m_id{id} {}

	std::shared_ptr<loop_impl>
	get_loop()
	{
		return m_loop;
	}

	event_id_type
	id() const
	{
		return m_id;
	}

private:
	std::shared_ptr<loop_impl> m_loop;
	event_id_type              m_id;
};

class loop_impl : public std::enable_shared_from_this<loop_impl>
{
public:
	using timer_type       = std::shared_ptr<timer_impl>;
	using timer_param_type = timer_type const&;

	loop_impl() : m_next_id{1}, m_running{false}, m_shutting_down{false} {}

	unsigned
	run()
	{
		unsigned count{0};
		assert(!m_running);
		assert(!m_shutting_down);

		m_stop_requested = false;

		if (m_shutdown_requested)
		{
			m_shutting_down = true;
			count += m_event_queue.cancel_all();
			m_shutting_down      = false;
			m_shutdown_requested = false;

			assert(!m_running);
			assert(!m_shutting_down);
			assert(!m_stop_requested);
			assert(!m_shutdown_requested);
		}
		else
		{
			time_point_type now = std::chrono::system_clock::now();
			m_running           = true;
			unsigned count{0};
			while (!m_event_queue.empty() && !m_stop_requested && !m_shutdown_requested)
			{
				if (m_event_queue.front_expiry() <= now)
				{
					m_event_queue.pop_front_and_trigger();
					++count;
				}
				else
				{
					millisecs delay_ms = std::chrono::duration_cast<millisecs>(m_event_queue.front_expiry() - now);
					if (delay_ms.count() < 1)
						delay_ms = millisecs{1};    // round up if (expiry - now) < 1 ms
					std::this_thread::sleep_for(delay_ms);
					now = std::chrono::system_clock::now();
				}
			}
			m_running = false;
			if (m_shutdown_requested)
			{
				m_shutdown_requested = false;
				m_shutting_down      = true;
				count += m_event_queue.cancel_all();
				m_shutting_down = false;
			}
			m_stop_requested = false;
		}

		assert(!m_running);
		assert(!m_shutting_down);
		assert(!m_stop_requested);
		assert(!m_shutdown_requested);

		return count;
	}

	void
	stop()
	{
		m_stop_requested = true;
	}

	void
	shutdown()
	{
		m_shutdown_requested = true;
	}

	timer_type
	create_timer()
	{
		return std::make_shared<timer_impl>(shared_from_this(), m_next_id++);
	}

	void
	start_timer(timer_param_type timer, millisecs delay, err_handler handler)
	{
		if (m_shutting_down)
		{
			handler(make_error_code(std::errc::operation_canceled));
		}
		else if (m_event_queue.already_in_queue(timer->id()))
		{
			handler(make_error_code(std::errc::operation_in_progress));
		}
		else
		{
			time_point_type expiry = std::chrono::system_clock::now()
									 + std::chrono::duration_cast<std::chrono::system_clock::duration>(delay);
			m_event_queue.add_event(std::make_unique<scheduled_event>(timer->id(), expiry, std::move(handler)));
		}
	}

	void
	cancel_timer(timer_param_type timer)
	{
		m_event_queue.cancel_event(timer->id());
	}

	void
	schedule(millisecs delay, err_handler handler)
	{
		if (m_shutting_down)
		{
			handler(make_error_code(std::errc::operation_canceled));
		}
		else
		{
			auto            id     = m_next_id++;    // not really necessary, but whatever
			time_point_type expiry = std::chrono::system_clock::now()
									 + std::chrono::duration_cast<std::chrono::system_clock::duration>(delay);
			m_event_queue.add_event(std::make_unique<scheduled_event>(id, expiry, std::move(handler)));
		}
	}

	void
	dispatch(void_handler handler)
	{
		if (!m_shutting_down)
		{
			m_event_queue.add_event(std::make_unique<dispatched_event>(m_next_id++, std::move(handler)));
		}
	}

private:
	event_id_type m_next_id;
	event_queue   m_event_queue;
	bool          m_running;
	bool          m_shutting_down;
	bool          m_stop_requested;
	bool          m_shutdown_requested;
};

class async_adapter
{
public:
	using loop_type         = std::shared_ptr<loop_impl>;
	using loop_param_type   = loop_type const&;
	using timer_type        = loop_impl::timer_type;
	using timer_param_type  = loop_impl::timer_param_type;
	using scheduled_action  = std::function<void(std::error_code const&)>;
	using dispatched_action = std::function<void()>;

	static timer_type
	create_timer(loop_param_type loop)
	{
		return loop->create_timer();
	}

	static loop_type
	create_loop()
	{
		return std::make_shared<loop_impl>();
	}

	static void
	run_loop(loop_param_type loop)
	{
		loop->run();
	}

	static void
	stop_loop(loop_param_type loop)
	{
		loop->stop();
	}

	static void
	shutdown_loop(loop_param_type loop)
	{
		loop->shutdown();
	}

	static void
	start_timer(timer_param_type timer, std::chrono::milliseconds ms, scheduled_action action)
	{
		timer->get_loop()->start_timer(timer, ms, std::move(action));
	}

	static void
	cancel_timer(timer_param_type timer)
	{
		timer->get_loop()->cancel_timer(timer);
	}

	static void
	schedule(loop_param_type loop, std::chrono::milliseconds ms, scheduled_action action)
	{
		loop->schedule(ms, std::move(action));
	}

	static void
	dispatch(loop_param_type loop, dispatched_action action)
	{
		loop->dispatch(action);
	}
};

}    // namespace ghetto_async

#endif    // UTIL_TEST_GHETTO_ASYNC_H