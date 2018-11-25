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

#ifndef LOGICMILL_ASYNC_TCP_UV_H
#define LOGICMILL_ASYNC_TCP_UV_H

#include <uv.h>
#include <logicmill/async/tcp.h>
#include "uv_error.h"

class tcp_write_request_uv;
class tcp_channel_uv;
class tcp_listener_uv;

class connect_request_uv
{
public:

	uv_connect_t										m_uv_connect_request;
	logicmill::async::tcp::channel::connect_handler		m_handler;


	template< class Handler, class = std::enable_if_t< std::is_convertible< Handler, logicmill::async::tcp::channel::connect_handler >::value > >
	connect_request_uv( Handler&& handler )
	:
	m_handler{ std::forward< Handler >( handler ) }
	{
		assert( reinterpret_cast< uv_connect_t* >( this ) == &m_uv_connect_request );
	}


	static std::shared_ptr< tcp_channel_uv >
	get_channel_shared_ptr( uv_connect_t* req );

	static void
	on_connect( uv_connect_t* req, int status );
};

class tcp_write_request_uv : public logicmill::async::tcp::write_request, public std::enable_shared_from_this< tcp_write_request_uv >
{
public:
	using ptr = std::shared_ptr< tcp_write_request_uv >;

	struct request_data
	{
		ptr		m_impl_ptr;
	};

	template< class Handler, class = std::enable_if_t< std::is_convertible< Handler, logicmill::async::tcp::channel::write_handler >::value > >
	tcp_write_request_uv( std::deque< logicmill::bstream::mutable_buffer >&& bufs, Handler&& handler )
	:
	m_write_handler{ std::forward< Handler >( handler ) },
	m_buffers{ std::move( bufs ) },
	m_uv_buffers{ new uv_buf_t[ m_buffers.size() ] }
	{
		std::size_t i = 0;
		for ( auto it = m_buffers.begin(); it != m_buffers.end(); ++it )
		{
			m_uv_buffers[ i ].base = reinterpret_cast< char* >( it->data() );
			m_uv_buffers[ i ].len = it->size();
			++i;
		}
	}

	tcp_write_request_uv( tcp_write_request_uv&& rhs );

	static void
	on_write( uv_write_t* req, int status );

	static std::shared_ptr< tcp_channel_uv >
	get_channel_shared_ptr( uv_write_t* req );

	void
	start( uv_stream_t* chan, ptr const& self );

	virtual ~tcp_write_request_uv();

	virtual std::deque< logicmill::bstream::mutable_buffer >
	release() override;

	virtual std::deque< logicmill::bstream::mutable_buffer > const&
	buffers() const override;

	logicmill::async::tcp::channel::write_handler			m_write_handler;
	uv_write_t												m_uv_write_request;
	request_data											m_data;
	std::deque< logicmill::bstream::mutable_buffer >		m_buffers;
	uv_buf_t*												m_uv_buffers;
};

#if 1

class tcp_base_uv
{
public:

	using ptr = std::shared_ptr< tcp_base_uv >;

	struct handle_data
	{
		ptr	m_self_ptr;
	};

	static void
	on_close( uv_handle_t* handle )
	{
		assert( uv_handle_get_type( handle ) == uv_handle_type::UV_TCP );
		auto tcp_base = get_base_raw_ptr( handle );
		assert( tcp_base->get_handle() == handle );
		tcp_base->clear();
	}

	virtual ~tcp_base_uv() {}

	tcp_base_uv*
	get_self_raw_ptr() const
	{
		return m_data.m_self_ptr.get();
	}

	void 
	set_self_ptr( ptr const& self)
	{
		m_data.m_self_ptr = self;
	}

	handle_data*
	get_handle_data()
	{
		return &m_data;
	}

	static handle_data*
	get_handle_data( uv_handle_t* handle )
	{
		return reinterpret_cast< handle_data* >( handle->data );
	}

	static ptr
	get_base_shared_ptr( uv_handle_t* handle )
	{
		return get_handle_data( handle )->m_self_ptr;
	}

	static ptr
	get_base_shared_ptr( uv_stream_t* stream_handle )
	{
		return get_base_shared_ptr( reinterpret_cast< uv_handle_t* >( stream_handle ) );
	}

	static uv_handle_t*
	get_handle_indirect( uv_handle_t* handle )
	{
		return reinterpret_cast< uv_handle_t* >( &( get_handle_data( handle )->m_self_ptr->m_tcp_handle ) );
	}

	static tcp_base_uv*
	get_base_raw_ptr( uv_handle_t* handle )
	{
		return  get_handle_data( handle )->m_self_ptr.get();
	}


	uv_tcp_t*
	get_tcp_handle()
	{
		return &m_tcp_handle;
	}

	uv_handle_t*
	get_handle()
	{
		return reinterpret_cast< uv_handle_t* >( &m_tcp_handle );
	}

	uv_stream_t*
	get_stream_handle()
	{
		return reinterpret_cast< uv_stream_t* >( &m_tcp_handle );
	}

	virtual void
	clear_handler() = 0;

	void clear()
	{
		clear_handler();
		m_data.m_self_ptr.reset();
	}

	handle_data 	m_data;
	uv_tcp_t		m_tcp_handle;
};

class tcp_channel_uv : public tcp_base_uv, public logicmill::async::tcp::channel // , public std::enable_shared_from_this< tcp_channel_uv >
{
public:

	using ptr = std::shared_ptr< tcp_channel_uv >;

	void
	init( uv_loop_t* lp, ptr const& self, std::error_code& err );

	virtual void
	clear_handler() override
	{
		m_read_handler = nullptr;
	}

	static void 
	on_read( uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf );

	static void 
	on_allocate( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

	static void
	on_close( uv_handle_t* handle );

	template< class Handler >
	typename std::enable_if_t< std::is_convertible< Handler, logicmill::async::tcp::channel::connect_handler >::value >
	connect( logicmill::async::ip::endpoint const& ep, std::error_code& err, Handler&& handler )
	{
		err.clear();
		sockaddr_storage saddr;
		ep.to_sockaddr( saddr );
		auto req = new connect_request_uv( std::forward< Handler >( handler ) );
		int stat = uv_tcp_connect( &req->m_uv_connect_request, get_tcp_handle(), reinterpret_cast< sockaddr* >( &saddr ), connect_request_uv::on_connect );
		if ( stat < 0 )
		{
			err = map_uv_error( stat );
			if ( ! uv_is_active( get_handle() ) )
			{
				req->m_handler = nullptr;
				delete req;
			}
		}
	}

	virtual void
	start_read( logicmill::async::tcp::channel::read_handler&& handler ) override;

	virtual void
	start_read( logicmill::async::tcp::channel::read_handler const& handler ) override;

	virtual void
	stop_read() override;

	virtual logicmill::async::tcp::write_request::ptr
	write( logicmill::bstream::mutable_buffer&& buf, logicmill::async::tcp::channel::write_handler&& handler ) override;

	virtual logicmill::async::tcp::write_request::ptr
	write( logicmill::bstream::mutable_buffer&& buf, logicmill::async::tcp::channel::write_handler const& handler ) override;

	virtual logicmill::async::tcp::write_request::ptr
	write( std::deque< logicmill::bstream::mutable_buffer >&& bufs, logicmill::async::tcp::channel::write_handler&& handler ) override;

	virtual logicmill::async::tcp::write_request::ptr
	write( std::deque< logicmill::bstream::mutable_buffer >&& bufs, logicmill::async::tcp::channel::write_handler const& handler ) override;

	virtual void
	close() override;

	logicmill::async::tcp::channel::read_handler		m_read_handler;
};

class tcp_listener_uv : public tcp_base_uv, public logicmill::async::tcp::listener
{
public:
	using ptr = std::shared_ptr< tcp_listener_uv >;

	template< class Handler, class = std::enable_if_t< std::is_convertible< Handler, logicmill::async::tcp::listener::connection_handler >::value > >
	tcp_listener_uv( logicmill::async::ip::endpoint const& ep, Handler&& handler )
	:
	m_endpoint{ ep },
	m_connection_handler{ std::forward< Handler >( handler ) }
	{}


	void 
	init( uv_loop_t* lp, ptr const& self, std::error_code& err );

	static ptr
	get_shared_listener( uv_stream_t* handle )
	{
		return std::dynamic_pointer_cast< tcp_listener_uv >( get_base_shared_ptr( reinterpret_cast< uv_handle_t* >( handle ) ) );
	}

	static ptr
	get_shared_listener( uv_tcp_t* handle )
	{
		return std::dynamic_pointer_cast< tcp_listener_uv >( get_base_shared_ptr( reinterpret_cast< uv_handle_t* >( handle ) ) );
	}

	static ptr
	get_shared_listener( uv_handle_t* handle )
	{
		return std::dynamic_pointer_cast< tcp_listener_uv >( get_base_shared_ptr( handle ) );
	}

	static tcp_listener_uv* 
	get_listener_impl_raw( uv_handle_t* handle )
	{
		return std::dynamic_pointer_cast< tcp_listener_uv >( get_base_shared_ptr( handle ) ).get();
	}

	static void 
	on_connection( uv_stream_t* handle, int stat );

	virtual void
	close() override;

	virtual void
	clear_handler() override
	{
		m_connection_handler = nullptr;
	}

private:
	logicmill::async::ip::endpoint								m_endpoint;
	logicmill::async::tcp::listener::connection_handler			m_connection_handler;
};

#endif

#endif /* LOGICMILL_ASYNC_TCP_UV_H */