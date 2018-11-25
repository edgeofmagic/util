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

#ifndef LOGICMILL_ASYNC_LOOP_UV_H
#define LOGICMILL_ASYNC_LOOP_UV_H

#include <uv.h>
#include <logicmill/async/loop.h>
#include "uv_error.h"


class resolve_req_uv
{
public:
	uv_getaddrinfo_t									m_uv_req;
	std::string											m_hostname;
	logicmill::async::loop::resolve_handler				m_handler;

	template< class Handler, class = std::enable_if_t< std::is_convertible< Handler, logicmill::async::loop::resolve_handler >::value > >
	resolve_req_uv( std::string const& hostname, Handler&& handler )
	:
	m_hostname{ hostname },
	m_handler{ std::forward< Handler >( handler ) }
	{
		assert( reinterpret_cast< uv_getaddrinfo_t* >( this ) == &m_uv_req );
	}

	void
	start( uv_loop_t* lp, std::error_code& err );

	static void 
	on_resolve( uv_getaddrinfo_t* req, int status, struct addrinfo* result );

};

class loop_uv;

struct loop_data
{
	std::weak_ptr< loop_uv >	m_impl_wptr;
	std::shared_ptr< loop_uv >	get_loop_ptr();
};

class loop_uv : public logicmill::async::loop 
{
public:

	struct use_default_loop {};

	using ptr = std::shared_ptr< loop_uv >;
	using wptr = std::weak_ptr< loop_uv >;

	loop_uv( loop_uv const& ) = delete;
	loop_uv( loop_uv&& ) = delete;

	loop_uv( use_default_loop flag );	// only use to construct for default loop

	static ptr
	create_from_default();

	loop_uv&
	operator=( loop_uv const& ) = delete;

	loop_uv&
	operator=( loop_uv&& ) = delete;

	loop_uv();

	void 
	init( wptr self );

	virtual ~loop_uv();

	virtual logicmill::async::timer::ptr
	create_timer( std::error_code& err, logicmill::async::timer::handler hf ) override;

	virtual void
	run( std::error_code& err ) override;

	virtual void
	stop( std::error_code& err ) override;

	static void
	on_walk( uv_handle_t* handle, void* );

	virtual void
	close( std::error_code& err ) override; // probably should NOT be called from any handler

#if ( BUILD_TCP )
	
	virtual logicmill::async::tcp::listener::ptr
	create_tcp_listener( logicmill::async::ip::endpoint const& ep, std::error_code& err, logicmill::async::tcp::listener::connection_handler&& handler ) override;
	
	virtual logicmill::async::tcp::listener::ptr
	create_tcp_listener( logicmill::async::ip::endpoint const& ep, std::error_code& err, logicmill::async::tcp::listener::connection_handler const& handler ) override;
		
	virtual logicmill::async::tcp::channel::ptr
	connect_tcp_channel( logicmill::async::ip::endpoint const& ep, std::error_code& err, logicmill::async::tcp::channel::connect_handler&& handler ) override;

	virtual logicmill::async::tcp::channel::ptr
	connect_tcp_channel( logicmill::async::ip::endpoint const& ep, std::error_code& err, logicmill::async::tcp::channel::connect_handler const& handler ) override;

#endif

	virtual void
	resolve( std::string const& hostname, std::error_code& err, resolve_handler&& handler ) override;

	virtual void
	resolve( std::string const& hostname, std::error_code& err, resolve_handler const& handler ) override;

	uv_loop_t*		m_uv_loop;
	loop_data		m_data;
	bool			m_is_default_loop;
};

#endif /* LOGICMILL_ASYNC_LOOP_UV_H */