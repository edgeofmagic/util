#include "loop_uv.h"
#include "timer_uv.h"

#if ( BUILD_TCP )
#include "tcp_uv.h"
#endif

using namespace logicmill;


void
resolve_req_uv::start( uv_loop_t* lp, std::error_code& err )
{
	err.clear();
	auto status = uv_getaddrinfo( lp, &m_uv_req, on_resolve, m_hostname.c_str(), nullptr, nullptr );
	UV_ERROR_CHECK( status, err, exit );	
exit:
	return;
}

void 
resolve_req_uv::on_resolve( uv_getaddrinfo_t* req, int status, struct addrinfo* result ) 
{
	auto request = reinterpret_cast< resolve_req_uv* >( req );
	std::error_code err = map_uv_error( status );
	std::deque< logicmill::async::ip::address > addresses;

	if ( ! err )
	{

		for ( auto info = result; info != nullptr; info = info->ai_next )
		{
			struct sockaddr_storage storage;

			memset( &storage, 0, sizeof( storage ) );
			memcpy( &storage, info->ai_addr, info->ai_addrlen );

			logicmill::async::ip::address addr;

			if ( info->ai_family == AF_INET )
			{
				addr = reinterpret_cast< struct sockaddr_in* >( info->ai_addr )->sin_addr;
			}
			else if ( info->ai_family == AF_INET6 )
			{
				addr = reinterpret_cast< struct sockaddr_in6* >( info->ai_addr )->sin6_addr;
			}

			auto it = std::find( addresses.begin(), addresses.end(), addr );

			if ( it == addresses.end() )
			{
				addresses.emplace_back( addr );
			}
		}
	}
	uv_freeaddrinfo( result );
	request->m_handler( request->m_hostname, std::move( addresses ), err );
	request->m_handler = nullptr;
	delete request;
}


loop_uv::loop_uv( use_default_loop flag )
:
m_uv_loop{ uv_default_loop() },
m_is_default_loop{ true }
{}

loop_uv::loop_uv()
:
m_uv_loop{ new uv_loop_t },
m_is_default_loop{ false }
{
	uv_loop_init( m_uv_loop );
}

void 
loop_uv::init( loop_uv::wptr self )
{
	m_data.m_impl_wptr = self;
	uv_loop_set_data( m_uv_loop, &m_data );
}

loop_uv::~loop_uv()
{
	std::cout << "starting loop destructor" << std::endl;
	std::error_code err;
	close( err );
	if ( ! err )
	{
		std::cout << "loop closed in destructor" << std::endl;
	}
	else
	{
		std::cout << "loop closure in destructor failed: " << err.message() << std::endl;			
	}
}

async::timer::ptr
loop_uv::create_timer( std::error_code& err, async::timer::handler hf )
{
	err.clear();

	timer_uv::ptr result;

	if ( ! m_uv_loop ) // TODO: propagate this test to other operations
	{
		err = make_error_code( async::errc::loop_closed );
		goto exit;
	}
	result = std::make_shared< timer_uv >( m_uv_loop, err, hf );
	result->init( result );
	if ( err ) goto exit;

exit:
	return result;
}

void
loop_uv::run( std::error_code& err )
{
	err.clear();
	if ( ! m_uv_loop )
	{
		err = make_error_code( async::errc::loop_closed );
		goto exit;
	}
	else
	{
		auto status = uv_run( m_uv_loop, UV_RUN_DEFAULT );
		UV_ERROR_CHECK( status, err, exit );
	}

exit:
	return;
}

void
loop_uv::stop( std::error_code& err )
{
	err.clear();
	if ( ! m_uv_loop )
	{
		err = make_error_code( async::errc::loop_closed );
	}
	uv_stop(  m_uv_loop );
exit:
	return;
}

void
loop_uv::on_walk( uv_handle_t* handle, void* )
{
	if ( handle )
	{
		auto handle_type = uv_handle_get_type( handle );
		switch( handle_type )
		{
			case uv_handle_type::UV_TIMER:
			{
				if ( ! uv_is_closing( handle ) )
				{
					std::cout << "closing timer: on loop closure" << std::endl;
					uv_close( handle, timer_uv::on_timer_close );
				}
			}
			break;
			case uv_handle_type::UV_TCP:
			{
				if ( ! uv_is_closing( handle ) )
				{
					std::cout << "closing tcp stream: on loop closure" << std::endl;
					uv_close( handle, tcp_base_uv::on_close );
				}
			}
			break;
			default:
			{
				std::cout << "closing loop handles, unexpected handle type: " << handle_type << std::endl;
			}
			break;
		}
	}
}

void
loop_uv::close( std::error_code& err ) // probably should NOT be called from any handler
{
	std::cout << "starting loop close" << std::endl;
	err.clear();
	int status = 0;

	if ( ! m_uv_loop )
	{
		err = make_error_code( async::errc::loop_closed );
		goto exit;
	}

	status = uv_loop_close( m_uv_loop );
	if ( status == UV_EBUSY )
	{
		uv_walk( m_uv_loop, on_walk, nullptr );
		status = uv_run( m_uv_loop, UV_RUN_DEFAULT ); // is this cool?
		UV_ERROR_CHECK( status, err, exit );
		status = uv_loop_close( m_uv_loop );
		UV_ERROR_CHECK( status, err, exit );
	}
	else if ( status < 0 )
	{
		UV_ERROR_CHECK( status, err, exit );
	}
	if ( ! m_is_default_loop )
	{
		delete m_uv_loop;
	}
	m_uv_loop = nullptr;
exit:
	return;
}

#if ( BUILD_TCP )

async::tcp::listener::ptr
loop_uv::create_tcp_listener( async::ip::endpoint const& ep, std::error_code& err, async::tcp::listener::connection_handler&& handler )
{
	err.clear();
	auto listener = std::make_shared< tcp_listener_uv >( ep, std::move( handler ) );
	listener->init( m_uv_loop, listener, err );
	return listener;
}

async::tcp::listener::ptr
loop_uv::create_tcp_listener( async::ip::endpoint const& ep, std::error_code& err, async::tcp::listener::connection_handler const& handler )
{
	err.clear();
	auto listener = std::make_shared< tcp_listener_uv >( ep, handler );
	listener->init( m_uv_loop, listener, err );
	return listener;
}
	
async::tcp::channel::ptr
loop_uv::connect_tcp_channel( async::ip::endpoint const& ep, std::error_code& err, async::tcp::channel::connect_handler&& handler )
{
	err.clear();
	auto cp = std::make_shared< tcp_channel_uv >();
	cp->init( m_uv_loop, cp, err );
	if ( err ) goto exit;
	cp->connect( ep, err, std::move( handler ) );
exit:
	return cp;
}

async::tcp::channel::ptr
loop_uv::connect_tcp_channel( async::ip::endpoint const& ep, std::error_code& err, async::tcp::channel::connect_handler const& handler )
{
	return nullptr;
}

#endif


void
loop_uv::resolve( std::string const& hostname, std::error_code& err, resolve_handler&& handler )
{
	err.clear();

	if ( ! m_uv_loop )
	{
		err = make_error_code( async::errc::loop_closed );
		goto exit;
	}

	{
		resolve_req_uv* req  = new resolve_req_uv{ hostname, std::move( handler ) };
		req->start( m_uv_loop, err );
	}

exit:
	return;
}

void
loop_uv::resolve( std::string const& hostname, std::error_code& err, resolve_handler const& handler )
{
	err.clear();

	if ( ! m_uv_loop )
	{
		err = make_error_code( async::errc::loop_closed );
		goto exit;
	}

	{
		resolve_req_uv* req  = new resolve_req_uv{ hostname, handler };
		req->start( m_uv_loop, err );
	}

exit:
	return;
}

loop_uv::ptr
loop_data::get_loop_ptr()
{
	return m_impl_wptr.lock();
}

loop_uv::ptr
loop_uv::create_from_default()
{
	static std::weak_ptr< loop_uv > default_loop;
	if ( default_loop.expired() )
	{
		auto default_loop_shared = std::make_shared< loop_uv >( use_default_loop{} );
		default_loop_shared->init( default_loop_shared );
		default_loop = default_loop_shared;
		return default_loop_shared;
	}
	else
	{
		return default_loop.lock();
	}
}

async::loop::ptr
async::loop::get_default()
{
	return loop_uv::create_from_default();
}

async::loop::ptr
async::loop::create()
{
	auto lp = std::make_shared< loop_uv >();
	lp->init( lp );
	return lp;
}
