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

#ifndef LOGICMILL_ASYNC_LOOP_H
#define LOGICMILL_ASYNC_LOOP_H

#include <chrono>
#include <functional>
#include <logicmill/async/channel.h>
#include <logicmill/async/transceiver.h>
#include <logicmill/async/endpoint.h>
#include <logicmill/async/options.h>
#include <logicmill/async/timer.h>
#include <logicmill/util/promise.h>
#include <memory>
#include <system_error>

namespace logicmill
{
namespace async
{

class loop
{
public:
	using ptr = std::shared_ptr<loop>;
	using resolve_handler = std::function<
			void(std::string const& hostname, std::deque<ip::address>&& addresses, std::error_code err)>;
	using dispatch_handler = std::function<void(loop::ptr const&)>;
	using dispatch_void_handler = std::function<void()>;
	using scheduled_handler = std::function<void(loop::ptr const&)>;
	using scheduled_void_handler = std::function<void()>;

	static loop::ptr
	create();

	static loop::ptr
	get_default();

	virtual ~loop() {}

	int
	run(std::error_code& err)
	{
		return really_run(err);
	}

	int
	run()
	{
		std::error_code err;
		auto result = really_run(err);
		if (err)
		{
			throw std::system_error{err};
		}
		return result;
	}

	int
	run_once(std::error_code& err)
	{
		return really_run_once(err);
	}

	int
	run_once()
	{
		std::error_code err;
		auto result = really_run_once(err);
		if (err)
		{
			throw std::system_error{err};
		}
		return result;
	}

	int
	run_nowait(std::error_code& err)
	{
		return really_run_nowait(err);
	}

	int
	run_nowait()
	{
		std::error_code err;
		auto result = really_run_nowait(err);
		if (err)
		{
			throw std::system_error{err};
		}
		return result;
	}

	void
	stop(std::error_code& err)
	{
		really_stop(err);
	}

	void
	stop()
	{
		std::error_code err;
		really_stop(err);
		if (err)
		{
			throw std::system_error{err};
		}
	}

	void
	close(std::error_code& err)
	{
		really_close(err);
	}

	void
	close()
	{
		std::error_code err;
		really_close(err);
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<
			std::is_convertible_v<Handler, dispatch_handler> && !std::is_same_v<Handler, std::nullptr_t>>
	dispatch(std::error_code& err, Handler&& handler)
	{
		really_dispatch(err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<
			std::is_convertible_v<Handler, dispatch_handler> && !std::is_same_v<Handler, std::nullptr_t>>
	dispatch(Handler&& handler)
	{
		std::error_code err;
		really_dispatch(err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible_v<Handler, dispatch_void_handler>  && !std::is_same_v<Handler, std::nullptr_t>>
	dispatch(std::error_code& err, Handler&& handler)
	{
		really_dispatch_void(err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible_v<Handler, dispatch_void_handler>  && !std::is_same_v<Handler, std::nullptr_t>>
	dispatch(Handler&& handler)
	{
		std::error_code err;
		really_dispatch_void(err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<
			std::is_convertible_v<Handler, scheduled_handler> && !std::is_same_v<Handler, std::nullptr_t>>
	schedule(std::chrono::milliseconds timeout, std::error_code& err, Handler&& handler)
	{
		really_schedule(timeout, err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<
			std::is_convertible_v<Handler, scheduled_handler> && !std::is_same_v<Handler, std::nullptr_t>>
	schedule(std::chrono::milliseconds timeout, Handler&& handler)
	{
		std::error_code err;
		really_schedule(timeout, err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible_v<Handler, scheduled_void_handler>  && !std::is_same_v<Handler, std::nullptr_t>>
	schedule(std::chrono::milliseconds timeout, std::error_code& err, Handler&& handler)
	{
		really_schedule_void(timeout, err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible_v<Handler, scheduled_void_handler>  && !std::is_same_v<Handler, std::nullptr_t>>
	schedule(std::chrono::milliseconds timeout, Handler&& handler)
	{
		std::error_code err;
		really_schedule_void(timeout, err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible_v<Handler, timer::handler>, timer::ptr>
	create_timer(std::error_code& err, Handler&& handler)
	{
		return really_create_timer(err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible_v<Handler, timer::handler>, timer::ptr>
	create_timer(Handler&& handler)
	{
		std::error_code err;
		return really_create_timer(err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible_v<Handler, timer::void_handler>, timer::ptr>
	create_timer(std::error_code& err, Handler&& handler)
	{
		return really_create_timer_void(err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible_v<Handler, timer::void_handler>, timer::ptr>
	create_timer(Handler&& handler)
	{
		std::error_code err;
		return really_create_timer_void(err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, acceptor::connection_handler>::value, acceptor::ptr>
	create_acceptor(options const& opts, std::error_code& err, Handler&& handler)
	{
		return really_create_acceptor(opts, err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, acceptor::connection_handler>::value, acceptor::ptr>
	create_acceptor(options const& opts, Handler&& handler)
	{
		std::error_code err;
		return really_create_acceptor(opts, err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, channel::connect_handler>::value, channel::ptr>
	connect_channel(options const& opts, std::error_code& err, Handler&& handler)
	{
		return really_connect_channel(opts, err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, channel::connect_handler>::value, channel::ptr>
	connect_channel(options const& opts, Handler&& handler)
	{
		std::error_code err;
		return really_connect_channel(opts, err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	transceiver::ptr
	create_transceiver(options const& opts, std::error_code& err)
	{
		return really_create_transceiver(opts, err);
	}

	transceiver::ptr
	create_transceiver(options const& opts)
	{
		std::error_code err;
		return really_create_transceiver(opts, err);
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, transceiver::receive_handler>::value, transceiver::ptr>
	create_transceiver(options const& opts, std::error_code& err, Handler&& handler)
	{
		return really_create_transceiver(opts, err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, transceiver::receive_handler>::value, transceiver::ptr>
	create_transceiver(options const& opts, Handler&& handler)
	{
		std::error_code err;
		return really_create_transceiver(opts, err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, resolve_handler>::value>
	resolve(std::string const& hostname, std::error_code& err, Handler&& handler)
	{
		really_resolve(hostname, err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, resolve_handler>::value>
	resolve(std::string const& hostname, Handler&& handler)
	{
		std::error_code err;
		really_resolve(hostname, err, std::forward<Handler>(handler));
		if (err)
		{
			throw std::system_error{err};
		}
	}

	virtual bool
	is_alive() const = 0;

protected:

	virtual int
	really_run(std::error_code& err) =0;

	virtual int
	really_run_once(std::error_code& err) =0;

	virtual int
	really_run_nowait(std::error_code& err) =0;

	virtual void
	really_stop(std::error_code& err) = 0;

	virtual void
	really_close(std::error_code& err) = 0;

	virtual timer::ptr
	really_create_timer(std::error_code& err, timer::handler handler) = 0;

	virtual timer::ptr
	really_create_timer_void(std::error_code& err, timer::void_handler handler) = 0;

	virtual void
	really_dispatch(std::error_code& err, dispatch_handler handler) = 0;

	virtual void
	really_dispatch_void(std::error_code& err, dispatch_void_handler handler) = 0;

	virtual void
	really_schedule(std::chrono::milliseconds timeout, std::error_code& err, scheduled_handler handler) = 0;

	virtual void
	really_schedule_void(std::chrono::milliseconds timeout, std::error_code& err, scheduled_void_handler handler) = 0;

	virtual acceptor::ptr
	really_create_acceptor(options const& opt, std::error_code& err, acceptor::connection_handler handler) = 0;

	virtual channel::ptr
	really_connect_channel(options const& opt, std::error_code& err, channel::connect_handler handler) = 0;

	virtual transceiver::ptr
	really_create_transceiver(options const& opt, std::error_code& err, transceiver::receive_handler handler) = 0;

	virtual transceiver::ptr
	really_create_transceiver(options const& opt, std::error_code& err) = 0;

	virtual void
	really_resolve(std::string const& hostname, std::error_code& err, resolve_handler handler) = 0;

};

}    // namespace async
}    // namespace logicmill

template<>
class logicmill::util::promise_timer<logicmill::async::loop::ptr>
{
public:
	promise_timer(std::chrono::milliseconds t, logicmill::async::loop::ptr const& lp) : m_timeout{t}, m_loop{lp} {}

	promise_timer(std::chrono::milliseconds t) : m_timeout{t},
	m_loop{logicmill::async::loop::get_default()} {}

	template<class T>
	void operator()(promise<T>& p)
	{
		p.cancel_timer();
		if (p.m_shared)
		{
			std::error_code err;
			auto tp = m_loop->create_timer(err, [p]() mutable
			{
				p.cancel_timer();
				if (!p.is_finished())
				{
					p.reject(make_error_code(std::errc::timed_out));
				}
			});

			p.m_shared->cancel_timer = [tp]()
			{
				tp->close();
			};

			tp->start(m_timeout, err);

		}
	}

private:
	std::chrono::milliseconds m_timeout;
	logicmill::async::loop::ptr m_loop;
};


#endif    // LOGICMILL_ASYNC_LOOP_H