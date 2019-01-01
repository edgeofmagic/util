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

	static loop::ptr
	create();

	static loop::ptr
	get_default();

	virtual ~loop() {}

	virtual void
	run(std::error_code& err) =0;

	virtual void
	stop(std::error_code& err) = 0;

	virtual void
	close(std::error_code& err) = 0;

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, dispatch_handler>::value>
	dispatch(std::error_code& err, Handler&& handler)
	{
		really_dispatch(err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, timer::handler>::value, timer::ptr>
	create_timer(std::error_code& err, Handler&& handler)
	{
		return really_create_timer(err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, acceptor::connection_handler>::value, acceptor::ptr>
	create_acceptor(options const& opts, std::error_code& err, Handler&& handler)
	{
		return really_create_acceptor(opts, err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, channel::connect_handler>::value, channel::ptr>
	connect_channel(options const& opts, std::error_code& err, Handler&& handler)
	{
		return really_connect_channel(opts, err, std::forward<Handler>(handler));
	}

	transceiver::ptr
	create_transceiver(options const& opts, std::error_code& err)
	{
		return really_create_transceiver(opts, err);
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, transceiver::receive_handler>::value, transceiver::ptr>
	create_transceiver(options const& opts, std::error_code& err, Handler&& handler)
	{
		return really_create_transceiver(opts, err, std::forward<Handler>(handler));
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, resolve_handler>::value>
	resolve(std::string const& hostname, std::error_code& err, Handler&& handler)
	{
		really_resolve(hostname, err, std::forward<Handler>(handler));
	}

protected:
	virtual timer::ptr
	really_create_timer(std::error_code& err, timer::handler const& handler) = 0;

	virtual timer::ptr
	really_create_timer(std::error_code& err, timer::handler&& handler) = 0;

	virtual void
	really_dispatch(std::error_code& err, dispatch_handler&& handler) = 0;

	virtual void
	really_dispatch(std::error_code& err, dispatch_handler const& handler) = 0;

	virtual acceptor::ptr
	really_create_acceptor(options const& opt, std::error_code& err, acceptor::connection_handler&& handler) = 0;

	virtual acceptor::ptr
	really_create_acceptor(options const& opt, std::error_code& err, acceptor::connection_handler const& handler) = 0;

	virtual channel::ptr
	really_connect_channel(options const& opt, std::error_code& err, channel::connect_handler&& handler) = 0;

	virtual channel::ptr
	really_connect_channel(options const& opt, std::error_code& err, channel::connect_handler const& handler) = 0;

	virtual transceiver::ptr
	really_create_transceiver(options const& opt, std::error_code& err, transceiver::receive_handler&& handler) = 0;

	virtual transceiver::ptr
	really_create_transceiver(options const& opt, std::error_code& err, transceiver::receive_handler const& handler) = 0;

	virtual transceiver::ptr
	really_create_transceiver(options const& opt, std::error_code& err) = 0;

	virtual void
	really_resolve(std::string const& hostname, std::error_code& err, resolve_handler&& handler) = 0;

	virtual void
	really_resolve(std::string const& hostname, std::error_code& err, resolve_handler const& handler) = 0;
};

}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ASYNC_LOOP_H