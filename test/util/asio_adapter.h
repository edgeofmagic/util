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

#ifndef ARMI_ADAPTERS_ASIO_ADAPTER_H
#define ARMI_ADAPTERS_ASIO_ADAPTER_H

#include "map_error_code.h"
#include <boost/asio.hpp>
#include <mutex>
#include <unordered_map>
#include <util/promise.h>
#include <util/shared_ptr.h>

namespace asio_adapter
{

class async_adapter
{
private:
	class loop_impl
	{
	public:
		friend class async_adapter;

		using work_type     = boost::asio::io_service::work;
		using work_map_type = std::unordered_map<loop_impl*, work_type>;

		loop_impl() : m_is_shutting_down{false}, m_was_started{false} {}

		~loop_impl()
		{
			delete_work();
		}

		void
		run()
		{
			if (!m_was_started)
			{
				m_was_started   = true;
				bool work_added = add_work();
				assert(work_added);
				m_io_context.run();
			}
		}

		bool
		is_running()
		{
			return m_was_started && !m_io_context.stopped();
		}

		void
		stop()
		{
			if (m_was_started && !m_io_context.stopped())
			{
				m_io_context.stop();
				bool work_deleted = delete_work();
				assert(work_deleted);
			}
		}

		void
		shutdown()
		{
			if (!m_is_shutting_down && m_was_started && m_io_context.stopped())
			{
				m_is_shutting_down = true;
				m_io_context.restart();
				// std::cout << "polling in loop shutdown: ";
				/* auto count = */ m_io_context.poll();
				// std::cout << count << std::endl;
			}
		}

		boost::asio::io_context&
		context()
		{
			return m_io_context;
		}

		work_map_type&
		get_work_map()
		{
			static work_map_type work_map;
			return work_map;
		}

		std::recursive_mutex&
		get_work_mutex()
		{
			static std::recursive_mutex work_mutex;
			return work_mutex;
		}

		bool
		add_work()
		{
			std::lock_guard<std::recursive_mutex> guard(get_work_mutex());
			bool                                  result{false};
			auto                                  it = get_work_map().find(this);
			if (it == get_work_map().end())
			{
				get_work_map().emplace(this, m_io_context);
				result = true;
			}
			return result;
		}

		bool
		delete_work()
		{
			std::lock_guard<std::recursive_mutex> guard(get_work_mutex());
			return static_cast<bool>(get_work_map().erase(this));
		}

		boost::asio::io_context m_io_context;
		bool                    m_is_shutting_down;
		bool                    m_was_started;
	};

public:
	using loop_type         = std::shared_ptr<loop_impl>;
	using loop_param_type   = loop_type const&;
	using timer_type        = util::shared_ptr<boost::asio::deadline_timer>;
	using timer_param_type  = timer_type const&;
	using scheduled_action  = std::function<void(std::error_code const&)>;
	using dispatched_action = std::function<void()>;

	static void
	clear(loop_type& loop)
	{
		loop.reset();
	}

	static void
	clear(timer_type& timer)
	{
		timer.reset();
	}

	static timer_type
	create_timer(loop_param_type loop)
	{
		return util::make_shared<boost::asio::deadline_timer>(loop->context());
	}

	static loop_type
	create_loop()
	{
		return std::make_shared<async_adapter::loop_impl>();
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
		boost::system::error_code ec;
		timer->expires_from_now(boost::posix_time::millisec(ms.count()), ec);
		if (ec)
		{
			action(map_error_code(ec));
		}
		else
		{
			timer->async_wait([=, action{std::move(action)}](boost::system::error_code const& ec) {
				if (ec)
				{
					auto err = map_error_code(ec);
					if (err != std::errc::operation_canceled)
					{
						action(err);
					}
				}
				else
					action(make_error_code(std::errc::timed_out));
			});
		}
	}

	static void
	cancel_timer(timer_param_type timer)
	{
		boost::system::error_code ec;
		timer->cancel(ec);
		// fail silently
	}

	static void
	schedule(loop_param_type loop, std::chrono::milliseconds ms, scheduled_action action)
	{
		auto timer = util::make_shared<boost::asio::deadline_timer>(
				loop->context(), boost::posix_time::millisec(ms.count()));
		timer->async_wait([=, action{std::move(action)}](boost::system::error_code const& ec) mutable {
			if (ec)
			{
				action(map_error_code(ec));
			}
			else
			{
				action(make_error_code(std::errc::timed_out));
			}
			timer.reset();
		});
	}

	static void
	dispatch(loop_param_type loop, dispatched_action action)
	{
		loop->context().post(action);
	}

	static bool
	is_loop_running(loop_param_type loop)
	{
		return loop->is_running();
	}
};

}    // namespace asio_adapter

#endif    // ARMI_ADAPTERS_ASIO_ADAPTER_H