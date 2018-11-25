#include "loop_uv.h"

#if ( BUILD_RESOLVER )

#include "resolve_request_uv.h"

using namespace logicmill;

resolve_request_uv::resolve_request_uv( std::string const& hostname,
												  logicmill::async::resolve_request::handler handler)
	: m_hostname{hostname}, m_handler{handler}
{}

resolve_request_uv::~resolve_request_uv()
{
	std::cout << "in resolve_request_uv destructor" << std::endl;
}

void 
resolve_request_uv::on_resolve( uv_getaddrinfo_t* req, int status, struct addrinfo* result ) 
{
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

	auto data = reinterpret_cast< resolve_request_data* >( uv_req_get_data( reinterpret_cast< uv_req_t* >( req ) ) );

	data->m_impl_ptr->m_handler( data->m_impl_ptr, std::move( addresses ), err );

	assert( req == reinterpret_cast< uv_getaddrinfo_t* >( &( data->m_impl_ptr->m_uv_req ) ) );
	data->m_impl_ptr->wipe();
	data->m_impl_ptr.reset(); // kill self-reference
}

void
resolve_request_uv::wipe()
{
	m_handler = nullptr;
}

std::shared_ptr< logicmill::async::loop >
resolve_request_uv::owner()
{
	return reinterpret_cast< loop_data* >( reinterpret_cast< uv_getaddrinfo_t* >( &m_uv_req )->loop->data )->get_loop_ptr();
}

void
resolve_request_uv::cancel()
{
	uv_cancel( reinterpret_cast< uv_req_t* >( &m_uv_req ) );
}

std::string const&
resolve_request_uv::hostname() const
{
	return m_hostname;
}

void 
resolve_request_uv::start( uv_loop_t* lp, resolve_request_uv::ptr const& self, std::error_code& err )
{
	err.clear();
	m_data.m_impl_ptr = self;
	uv_req_set_data( reinterpret_cast< uv_req_t* >( &m_uv_req ), &m_data );
	auto status = uv_getaddrinfo( lp, &m_uv_req, on_resolve, m_hostname.c_str(), nullptr, nullptr );
	UV_ERROR_CHECK( status, err, exit );	

exit:
	return;
}

#endif /* ( BUILD_RESOLVER ) */