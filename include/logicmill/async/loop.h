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

#define BUILD_TCP 1

#include <memory>
#include <functional>
#include <system_error>
#include <chrono>
#include <logicmill/async/timer.h>
#include <logicmill/async/endpoint.h>

#if ( BUILD_TCP )
#include <logicmill/async/tcp.h>
#endif

namespace logicmill
{
namespace async
{

class loop
{
public:
	using ptr = std::shared_ptr< loop >;

	using resolve_handler = std::function< void( std::string const& hostname, std::deque< ip::address >&& addresses, std::error_code const& err ) >;

	static loop::ptr
	create();

	static loop::ptr
	get_default();

	virtual
	~loop() {}

	virtual void 
	run( std::error_code& err ) = 0;

	virtual void
	stop( std::error_code& err ) = 0;

	virtual void
	close( std::error_code& err ) = 0;

	virtual timer::ptr
	create_timer( std::error_code& err, timer::handler hf ) = 0;

#if ( BUILD_TCP )

	virtual tcp::listener::ptr
	create_tcp_listener( ip::endpoint const& ep, std::error_code& err, tcp::listener::connection_handler&& handler ) = 0;
		
	virtual tcp::listener::ptr
	create_tcp_listener( ip::endpoint const& ep, std::error_code& err, tcp::listener::connection_handler const& handler ) = 0;
	
	virtual tcp::channel::ptr
	connect_tcp_channel( ip::endpoint const& ep, std::error_code& err, tcp::channel::connect_handler&& handler ) = 0;

	virtual tcp::channel::ptr
	connect_tcp_channel( ip::endpoint const& ep, std::error_code& err, tcp::channel::connect_handler const& handler ) = 0;

#endif

	virtual void
	resolve( std::string const& hostname, std::error_code& err, resolve_handler&& handler ) = 0;

	virtual void
	resolve( std::string const& hostname, std::error_code& err, resolve_handler const& handler ) = 0;

};

} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_LOOP_H