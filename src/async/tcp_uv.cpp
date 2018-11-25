#include "loop_uv.h"

#if ( BUILD_TCP )

#include "tcp_uv.h"

using namespace logicmill;

std::shared_ptr< tcp_channel_uv >
connect_request_uv::get_channel_shared_ptr( uv_connect_t* req )
{
	return std::dynamic_pointer_cast< tcp_channel_uv >( tcp_base_uv::get_base_shared_ptr( req->handle ) );
}

void
connect_request_uv::on_connect( uv_connect_t* req, int status )
{
	auto request_ptr = reinterpret_cast< connect_request_uv* >( req );
	std::error_code err = map_uv_error( status );
	request_ptr->m_handler( get_channel_shared_ptr( req ), err );
	request_ptr->m_handler = nullptr;
	delete req;
}

/* tcp_write_request_uv */

tcp_channel_uv::ptr
tcp_write_req_uv::get_channel_shared_ptr( uv_write_t* req )
{
	return std::dynamic_pointer_cast< tcp_channel_uv >( tcp_base_uv::get_base_shared_ptr( req->handle ) );
}

void
tcp_write_req_uv::on_write( uv_write_t* req, int status )
{
	auto target = reinterpret_cast< tcp_write_req_uv* >( req );
	target->m_write_handler( get_channel_shared_ptr( req ), std::move( target->m_buffers ), map_uv_error( status ) );
	delete target;
}

/* tcp_channel_uv */

void
tcp_channel_uv::init( uv_loop_t* lp, ptr const& self, std::error_code& err )
{
	auto stat = uv_tcp_init( lp, get_tcp_handle() );
	uv_handle_set_data( get_handle(), get_handle_data() );
	set_self_ptr( self );
	UV_ERROR_CHECK( stat, err, exit );
exit:
	return;
}

void 
tcp_channel_uv::on_read( uv_stream_t* stream_handle , ssize_t nread, const uv_buf_t* buf )
{
	std::cout << "on_read called, nread is " << nread <<  ", buffer size is " <<  buf->len << std::endl;
	std::error_code err;
	ptr channel_ptr = std::dynamic_pointer_cast< tcp_channel_uv >( get_base_shared_ptr( stream_handle ) );
	assert( channel_ptr );
	if ( nread < 0 )
	{
		err = map_uv_error( nread );
		channel_ptr->m_read_handler( channel_ptr, bstream::const_buffer{}, err );
	}
	else if ( nread == 0 )
	{
		channel_ptr->m_read_handler( channel_ptr, bstream::const_buffer{}, err );
	}
	else if ( nread > 0 )
	{
		std::cout << "in branch invoking read handler with buffer contents" << std::endl;
		channel_ptr->m_read_handler( channel_ptr,
									 bstream::const_buffer{ reinterpret_cast<bstream::byte_type*>( buf->base ),
															static_cast< bstream::size_type >( nread ),
															bstream::buffer::default_deallocator{} },
									 err );
		std::cout << "after invoking read handler" << std::endl;
	}
}

void 
tcp_channel_uv::on_allocate( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
{
	std::cout << "on_allocate called" << std::endl;
	static bstream::buffer::memory_broker::ptr broker = bstream::buffer::default_broker::get();
	buf->base = reinterpret_cast< char * >( broker->allocate( suggested_size ) );
	buf->len = suggested_size;
}

void
tcp_channel_uv::close()
{
	if ( ! uv_is_closing( get_handle() ) )
	{
		uv_close( get_handle(), tcp_base_uv::on_close );
	}
}

void
tcp_channel_uv::start_read( async::tcp::channel::read_handler&& handler )
{
	std::cout << "in start_read ( rvalue handler )" << std::endl;
	m_read_handler = std::move( handler );
	auto stat = uv_read_start( get_stream_handle(), on_allocate, on_read );
	std::cout << "result of uv_read_start is " << stat << std::endl;
}

void
tcp_channel_uv::start_read( async::tcp::channel::read_handler const& handler )
{
	std::cout << "in start_read ( lvalue handler )" << std::endl;
	m_read_handler = handler;
	uv_read_start( get_stream_handle(), on_allocate, on_read );
}

void
tcp_channel_uv::stop_read()
{
	uv_read_stop( get_stream_handle() );
}

void
tcp_channel_uv::write( bstream::mutable_buffer&& buf, async::tcp::channel::write_handler&& handler )
{
	std::deque< bstream::mutable_buffer > buf_deque;
	buf_deque.emplace_back( std::move( buf ) );
	auto request = new tcp_write_req_uv{ std::move( buf_deque ), std::move( handler ) };
	request->start( get_stream_handle() );
}

void
tcp_channel_uv::write( bstream::mutable_buffer&& buf, async::tcp::channel::write_handler const& handler )
{
	std::deque< bstream::mutable_buffer > buf_deque;
	buf_deque.emplace_back( std::move( buf ) );
	auto request = new tcp_write_req_uv{ std::move( buf_deque ), handler };
	request->start( get_stream_handle() );
}

void
tcp_channel_uv::write( std::deque< bstream::mutable_buffer >&& bufs, async::tcp::channel::write_handler&& handler )
{
	auto request = new tcp_write_req_uv{ std::move( bufs ), std::move( handler ) };
	request->start( get_stream_handle() );
}

void
tcp_channel_uv::write( std::deque< bstream::mutable_buffer >&& bufs, async::tcp::channel::write_handler const& handler )
{
	auto request = new tcp_write_req_uv{ std::move( bufs ), handler };
	request->start( get_stream_handle() );
}


void 
tcp_listener_uv::init( uv_loop_t* lp, ptr const& self, std::error_code& err )
{
	err.clear();
	set_self_ptr( self );
	uv_handle_set_data( get_handle(), get_handle_data() );
	auto stat = uv_tcp_init( lp, get_tcp_handle() );
	UV_ERROR_CHECK( stat, err, exit );
	sockaddr_storage saddr;
	m_endpoint.to_sockaddr( saddr );
	stat = uv_tcp_bind( get_tcp_handle(), reinterpret_cast< sockaddr* >( &saddr ), 0 );
	UV_ERROR_CHECK( stat, err, exit );
	stat = uv_listen( reinterpret_cast< uv_stream_t* >( get_tcp_handle() ), 128, on_connection );
	UV_ERROR_CHECK( stat, err, exit );
exit:
	return;
}

void 
tcp_listener_uv::on_connection( uv_stream_t* handle, int stat )
{
	auto listener_ptr = std::dynamic_pointer_cast< tcp_listener_uv >( get_base_shared_ptr( handle ) );

	if ( stat < 0 )
	{
		listener_ptr->m_connection_handler( listener_ptr, nullptr, map_uv_error( stat ) );
	}
	else
	{
		std::error_code err;
		auto channel_ptr = std::make_shared< tcp_channel_uv >();
		channel_ptr->init( listener_ptr->get_handle()->loop, channel_ptr, err );
		if ( err )
		{
			listener_ptr->m_connection_handler( listener_ptr, channel_ptr, err );
		}
		else
		{
			int status = uv_accept( listener_ptr->get_stream_handle(), channel_ptr->get_stream_handle() );
			if ( status )
			{
				err = map_uv_error( status );
			}
			listener_ptr->m_connection_handler( listener_ptr, channel_ptr, err );
		}
	}
}

void
tcp_listener_uv::close()
{
	auto handle = get_handle();
	if ( ! uv_is_closing( handle ) )
	{
		uv_close( handle, on_close );
	}
}

#endif