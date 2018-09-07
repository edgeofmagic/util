#include <uv.h>
#include <logicmill/async/loop.h>
#include <logicmill/async/error.h>
#include <netdb.h>
#include <sys/socket.h>

#define UV_ERROR_CHECK( _uv_err_, _err_var_, _label_ )				\
{																	\
	if ( _uv_err_ < 0 )												\
	{																\
		_err_var_ = map_uv_error( _uv_err_ );						\
		goto _label_;												\
	}																\
}																	\
/**/

#define UV_ERROR_THROW( _uv_err_ )									\
{																	\
	if ( _uv_err_ < 0 )												\
	{																\
		throw std::system_error{ map_uv_error( _uv_err_ ) };		\
	}																\
}																	\
/**/

using namespace logicmill;

std::error_code const&
map_uv_error( int uv_code )
{
	switch ( uv_code )
	{
		case 0:
		{
			static const std::error_code success;
			return success;
		}
		case UV_E2BIG:
		{
			static const std::error_code e = make_error_code( std::errc::argument_list_too_long );
			return e;
		}
		case UV_EACCES:
		{
			static const std::error_code e = make_error_code( std::errc::permission_denied );
			return e;
		}
		case UV_EADDRINUSE:
		{
			static const std::error_code e = make_error_code( std::errc::address_in_use );
			return e;
		}
		case UV_EADDRNOTAVAIL:
		{
			static const std::error_code e = make_error_code( std::errc::address_not_available );
			return e;
		}
		case UV_EAFNOSUPPORT:
		{
			static const std::error_code e = make_error_code( std::errc::address_family_not_supported );
			return e;
		}
		case UV_EAGAIN:
		{
			static const std::error_code e = make_error_code( std::errc::resource_unavailable_try_again );
			return e;
		}
		case UV_EAI_ADDRFAMILY:
		{
			static const std::error_code e = make_error_code( address_info::errc::address_family_not_supported );
			return e;
		}
		case UV_EAI_AGAIN:
		{
			static const std::error_code e = make_error_code( address_info::errc::temporary_failure );
			return e;
		}
		case UV_EAI_BADFLAGS:
		{
			static const std::error_code e = make_error_code( address_info::errc::invalid_value_for_ai_flags );
			return e;
		}
		case UV_EAI_BADHINTS:
		{
			static const std::error_code e = make_error_code( address_info::errc::invalid_value_for_hints );
			return e;
		}
		case UV_EAI_CANCELED:
		{
			static const std::error_code e = make_error_code( address_info::errc::request_canceled );
			return e;
		}
		case UV_EAI_FAIL:
		{
			static const std::error_code e = make_error_code( address_info::errc::non_recoverable_error );
			return e;
		}
		case UV_EAI_FAMILY:
		{
			static const std::error_code e = make_error_code( address_info::errc::ai_family_not_supported );
			return e;
		}
		case UV_EAI_MEMORY:
		{
			static const std::error_code e = make_error_code( address_info::errc::memory_allocation_failure );
			return e;
		}
		case UV_EAI_NODATA:
		{
			static const std::error_code e = make_error_code( address_info::errc::no_address_for_name );
			return e;
		}
		case UV_EAI_NONAME:
		{
			static const std::error_code e = make_error_code( address_info::errc::name_is_unknown );
			return e;
		}
		case UV_EAI_OVERFLOW:
		{
			static const std::error_code e = make_error_code( address_info::errc::argument_buffer_overflow );
			return e;
		}
		case UV_EAI_PROTOCOL:
		{
			static const std::error_code e = make_error_code( address_info::errc::unknown_protocol );
			return e;
		}
		case UV_EAI_SERVICE:
		{
			static const std::error_code e = make_error_code( address_info::errc::service_not_supported );
			return e;
		}
		case UV_EAI_SOCKTYPE:
		{
			static const std::error_code e = make_error_code( address_info::errc::ai_socktype_not_supported );
			return e;
		}
		case UV_EALREADY:
		{
			static const std::error_code e = make_error_code( std::errc::connection_already_in_progress );
			return e;
		}
		case UV_EBADF:
		{
			static const std::error_code e = make_error_code( std::errc::bad_file_descriptor );
			return e;
		}
		case UV_EBUSY:
		{
			static const std::error_code e = make_error_code( std::errc::device_or_resource_busy );
			return e;
		}
		case UV_ECANCELED:
		{
			static const std::error_code e = make_error_code( std::errc::operation_canceled );
			return e;
		}
		case UV_ECHARSET:
		{
			static const std::error_code e = make_error_code( async::errc::invalid_unicode_character );
			return e;
		}
		case UV_ECONNABORTED:
		{
			static const std::error_code e = make_error_code( std::errc::connection_aborted );
			return e;
		}
		case UV_ECONNREFUSED:
		{
			static const std::error_code e = make_error_code( std::errc::connection_refused );
			return e;
		}
		case UV_ECONNRESET:
		{
			static const std::error_code e = make_error_code( std::errc::connection_reset );
			return e;
		}
		case UV_EDESTADDRREQ:
		{
			static const std::error_code e = make_error_code( std::errc::destination_address_required );
			return e;
		}
		case UV_EEXIST:
		{
			static const std::error_code e = make_error_code( std::errc::file_exists );
			return e;
		}
		case UV_EFAULT:
		{
			static const std::error_code e = make_error_code( std::errc::bad_address );
			return e;
		}
		case UV_EFBIG:
		{
			static const std::error_code e = make_error_code( std::errc::file_too_large );
			return e;
		}
		case UV_EHOSTDOWN:
		{
			static const std::error_code e = make_error_code( async::errc::host_is_down );
			return e;
		}
		case UV_EHOSTUNREACH:
		{
			static const std::error_code e = make_error_code( std::errc::host_unreachable );
			return e;
		}
		case UV_EINTR:
		{
			static const std::error_code e = make_error_code( std::errc::interrupted );
			return e;
		}
		case UV_EINVAL:
		{
			static const std::error_code e = make_error_code( std::errc::invalid_argument );
			return e;
		}
		case UV_EIO:
		{
			static const std::error_code e = make_error_code( std::errc::io_error );
			return e;
		}
		case UV_EISCONN:
		{
			static const std::error_code e = make_error_code( std::errc::already_connected );
			return e;
		}
		case UV_EISDIR:
		{
			static const std::error_code e = make_error_code( std::errc::is_a_directory );
			return e;
		}
		case UV_ELOOP:
		{
			static const std::error_code e = make_error_code( std::errc::too_many_symbolic_link_levels );
			return e;
		}
		case UV_EMFILE:
		{
			static const std::error_code e = make_error_code( std::errc::too_many_files_open );
			return e;
		}
		case UV_EMLINK:
		{
			static const std::error_code e = make_error_code( std::errc::too_many_links );
			return e;
		}
		case UV_EMSGSIZE:
		{
			static const std::error_code e = make_error_code( std::errc::message_size );
			return e;
		}
		case UV_ENAMETOOLONG:
		{
			static const std::error_code e = make_error_code( std::errc::filename_too_long );
			return e;
		}
		case UV_ENETDOWN:
		{
			static const std::error_code e = make_error_code( std::errc::network_down );
			return e;
		}
		case UV_ENETUNREACH:
		{
			static const std::error_code e = make_error_code( std::errc::network_unreachable );
			return e;
		}
		case UV_ENFILE:
		{
			static const std::error_code e = make_error_code( std::errc::too_many_files_open_in_system );
			return e;
		}
		case UV_ENOBUFS:
		{
			static const std::error_code e = make_error_code( std::errc::no_buffer_space );
			return e;
		}
		case UV_ENODEV:
		{
			static const std::error_code e = make_error_code( std::errc::no_such_device );
			return e;
		}
		case UV_ENOENT:
		{
			static const std::error_code e = make_error_code( std::errc::no_such_file_or_directory );
			return e;
		}
		case UV_ENOMEM:
		{
			static const std::error_code e = make_error_code( std::errc::not_enough_memory );
			return e;
		}
		case UV_ENONET:
		{
			static const std::error_code e = make_error_code( async::errc::no_network_connection );
			return e;
		}
		case UV_ENOPROTOOPT:
		{
			static const std::error_code e = make_error_code( std::errc::no_protocol_option );
			return e;
		}
		case UV_ENOSPC:
		{
			static const std::error_code e = make_error_code( std::errc::no_space_on_device );
			return e;
		}
		case UV_ENOSYS:
		{
			static const std::error_code e = make_error_code( std::errc::function_not_supported );
			return e;
		}
		case UV_ENOTCONN:
		{
			static const std::error_code e = make_error_code( std::errc::not_connected );
			return e;
		}
		case UV_ENOTDIR:
		{
			static const std::error_code e = make_error_code( std::errc::not_a_directory );
			return e;
		}
		case UV_ENOTEMPTY:
		{
			static const std::error_code e = make_error_code( std::errc::directory_not_empty );
			return e;
		}
		case UV_ENOTSOCK:
		{
			static const std::error_code e = make_error_code( std::errc::not_a_socket );
			return e;
		}
		case UV_ENOTSUP:
		{
			static const std::error_code e = make_error_code( std::errc::not_supported );
			return e;
		}
		case UV_ENOTTY:
		{
			static const std::error_code e = make_error_code( std::errc::inappropriate_io_control_operation );
			return e;
		}
		case UV_ENXIO:
		{
			static const std::error_code e = make_error_code( std::errc::no_such_device_or_address );
			return e;
		}
		case UV_EOF:
		{
			static const std::error_code e = make_error_code( async::errc::end_of_file );
			return e;
		}
		case UV_EPERM:
		{
			static const std::error_code e = make_error_code( std::errc::operation_not_permitted );
			return e;
		}
		case UV_EPIPE:
		{
			static const std::error_code e = make_error_code( std::errc::broken_pipe );
			return e;
		}
		case UV_EPROTO:
		{
			static const std::error_code e = make_error_code( std::errc::protocol_error );
			return e;
		}
		case UV_EPROTONOSUPPORT:
		{
			static const std::error_code e = make_error_code( std::errc::protocol_not_supported );
			return e;
		}
		case UV_EPROTOTYPE:
		{
			static const std::error_code e = make_error_code( std::errc::wrong_protocol_type );
			return e;
		}
		case UV_ERANGE:
		{
			static const std::error_code e = make_error_code( std::errc::result_out_of_range );
			return e;
		}
		case UV_EREMOTEIO:
		{
			static const std::error_code e = make_error_code( async::errc::remote_io_error );
			return e;
		}
		case UV_EROFS:
		{
			static const std::error_code e = make_error_code( std::errc::read_only_file_system );
			return e;
		}
		case UV_ESHUTDOWN:
		{
			static const std::error_code e = make_error_code( async::errc::transport_endpoint_shutdown );
			return e;
		}
		case UV_ESPIPE:
		{
			static const std::error_code e = make_error_code( std::errc::invalid_seek );
			return e;
		}
		case UV_ESRCH:
		{
			static const std::error_code e = make_error_code( std::errc::no_such_process );
			return e;
		}
		case UV_ETIMEDOUT:
		{
			static const std::error_code e = make_error_code( std::errc::timed_out );
			return e;
		}
		case UV_ETXTBSY:
		{
			static const std::error_code e = make_error_code( std::errc::text_file_busy );
			return e;
		}
		case UV_EXDEV:
		{
			static const std::error_code e = make_error_code( std::errc::cross_device_link );
			return e;
		}
		case UV_UNKNOWN:
		default:
		{
			static const std::error_code e = make_error_code( async::errc::unknown_error );
			return e;
		}
	}
}


static void
clear_error( std::error_code& err )
{
	static std::error_code no_error;
	err = no_error;
}

class loop_impl;


class resolver_impl : public async::resolver, public std::enable_shared_from_this< resolver_impl >
{
public:

	using ptr = std::shared_ptr< resolver_impl >;

	struct req_data
	{
		resolver_impl::ptr	m_req_impl;
	};

	resolver_impl( std::shared_ptr< loop_impl > owner, async::resolver::handler handler )
	:
	m_owner{ owner },
	m_uv_req{ nullptr },
	m_data{ nullptr },
	m_hostname{},
	m_handler{ handler }
	{}

	static void on_resolve( uv_getaddrinfo_t* req, int status, struct addrinfo* result ) 
	{
		std::error_code err = map_uv_error( status );
		std::deque< async::ip::address > addresses;

		if ( ! err )
		{

			for ( auto info = result; info != nullptr; info = info->ai_next )
			{
				struct sockaddr_storage storage;

				memset( &storage, 0, sizeof( storage ) );
				memcpy( &storage, info->ai_addr, info->ai_addrlen );

				async::ip::address addr;

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

		auto p = uv_req_get_data( reinterpret_cast< uv_req_t* >( req ) );
		auto data = reinterpret_cast< resolver_impl::req_data* >( p );
		auto req_impl = data->m_req_impl;

		req_impl->m_handler( req_impl, std::move( addresses ), err );

		data->m_req_impl = nullptr; // kill self-reference
		req_impl->m_hostname.clear();
		assert( ( void* )req == ( void* )( req_impl->m_uv_req ) );
		req_impl->m_uv_req = nullptr;
		delete req;
	}

	virtual std::shared_ptr< async::loop >
	owner() override;

	virtual void
	cancel() override
	{
		if ( m_uv_req )
		{
			uv_cancel( reinterpret_cast< uv_req_t* >( m_uv_req ) );
		}
	}

	virtual std::string const&
	hostname() const override
	{
		return m_hostname;
	}

	virtual void
	resolve(  std::string const& hostname, std::error_code& err ) override;

	std::shared_ptr< loop_impl >		m_owner;
	uv_getaddrinfo_t*					m_uv_req;
	req_data							m_data;
	std::string							m_hostname;
	async::resolver::handler			m_handler;

};

class timer_impl : public async::timer
{
public:

	using ptr = std::shared_ptr< timer_impl >;

	struct timer_data
	{
		timer_impl::ptr		m_timer_impl;
	};

	timer_impl( std::shared_ptr< loop_impl > owner, async::timer::handler handler );
	
	timer_impl( std::shared_ptr< loop_impl > owner, std::error_code& err, async::timer::handler handler );

	virtual ~timer_impl()
	{
		if ( m_uv_timer &&  ! uv_is_closing( reinterpret_cast< uv_handle_t* >( m_uv_timer ) ) )
		{
			uv_close( reinterpret_cast< uv_handle_t* >( m_uv_timer ), on_timer_close );
		}
	}

	virtual void
	start( std::chrono::milliseconds timeout ) override
	{
		if ( ! m_uv_timer )
		{
			throw std::system_error{ make_error_code( std::errc::state_not_recoverable ) };
		}
		auto status = uv_timer_start( m_uv_timer, timer_cb, timeout.count(), 0 );
		UV_ERROR_THROW( status );
	}

	virtual void
	start( std::chrono::milliseconds timeout, std::error_code& err ) override
	{
		clear_error( err );
		if ( ! m_uv_timer )
		{
			err = make_error_code( std::errc::state_not_recoverable );
			goto exit;
		}
		else
		{
			auto status = uv_timer_start( m_uv_timer, timer_cb, timeout.count(), 0 );
			UV_ERROR_CHECK( status, err, exit );
		}
	exit:
		return;
	}

	virtual void
	stop() override
	{
		if ( ! m_uv_timer )
		{
			throw std::system_error{ make_error_code( std::errc::state_not_recoverable ) };
		}
		auto status = uv_timer_stop( m_uv_timer );
		UV_ERROR_THROW( status );
	}

	virtual void
	stop( std::error_code& err ) override
	{
		clear_error( err );
		if ( ! m_uv_timer )
		{
			err = make_error_code( std::errc::state_not_recoverable );
			goto exit;
		}
		else
		{
			auto status = uv_timer_stop( m_uv_timer );
			UV_ERROR_CHECK( status, err, exit );
		}
	exit:
		return;
	}

	virtual void
	close() override
	{
		if ( m_uv_timer && ! uv_is_closing( reinterpret_cast< uv_handle_t* >( m_uv_timer ) ) )
		{
			uv_close( reinterpret_cast< uv_handle_t* >( m_uv_timer ), on_timer_close );
		}
	}

	virtual std::shared_ptr< async::loop >
	owner() override;

	static void
	on_timer_close( uv_handle_t* handle )
	{
		assert( uv_handle_get_type( handle ) == uv_handle_type::UV_TIMER );
		timer_data* tdata = reinterpret_cast< timer_data* >( uv_handle_get_data( handle ) );
		assert( (void*)tdata->m_timer_impl->m_uv_timer == (void*)handle );
		tdata->m_timer_impl->m_uv_timer = nullptr;
		tdata->m_timer_impl->m_owner = nullptr;
		tdata->m_timer_impl = nullptr; // release the self-reference
		uv_timer_t* timer_handle = reinterpret_cast< uv_timer_t* >( handle );
		delete timer_handle;
	}

	void
	prep( timer_impl::ptr self )
	{
		m_data.m_timer_impl = self;
	}
	
	static void
	timer_cb( uv_timer_t* handle )
	{
		timer_data* tdata = reinterpret_cast< timer_data* >( uv_handle_get_data( reinterpret_cast< uv_handle_t* >( handle ) ) );
		auto t_impl = tdata->m_timer_impl;
		t_impl->m_handler( t_impl );
	}

	std::shared_ptr< loop_impl >	m_owner;
	uv_timer_t*						m_uv_timer;
	timer_data						m_data;
	async::timer::handler			m_handler;

};

class loop_impl : public async::loop, public std::enable_shared_from_this< loop_impl>
{
public:

	using ptr = std::shared_ptr< loop_impl >;

	loop_impl( loop_impl const& ) = delete;
	loop_impl( loop_impl&& ) = delete;

	loop_impl( uv_loop_t* lp )	// only use to construct for default loop
	:
	m_uv_loop{ lp },
	m_is_default_loop{ true }
	{}

	loop_impl&
	operator=( loop_impl const& ) = delete;

	loop_impl&
	operator=( loop_impl&& ) = delete;

	loop_impl()
	:
	m_uv_loop{ new uv_loop_t },
	m_is_default_loop{ false }
	{
		uv_loop_init( m_uv_loop );
	}

	loop_impl::ptr
	get_ptr()
	{
		return shared_from_this();
	}

	virtual ~loop_impl()
	{
		std::error_code err;
		close( err );
	}

	virtual async::timer::ptr
	create_timer( async::timer::handler hf ) override
	{
		if ( m_uv_loop )
		{
			auto status = uv_run( m_uv_loop, UV_RUN_DEFAULT );
			UV_ERROR_THROW( status );
		}
		std::error_code err;
		auto tp = std::make_shared< timer_impl >( get_ptr(), err, hf );
		if ( err )
		{
			throw std::system_error{ err };
		}
		tp->prep( tp );
		return tp;
	}

	virtual async::timer::ptr
	create_timer( std::error_code& err, async::timer::handler hf ) override
	{
		clear_error( err );
		timer_impl::ptr result;
		if ( ! m_uv_loop )
		{
			err = make_error_code(std::errc::state_not_recoverable );
			goto exit;
		}
		else
		{
			result = std::make_shared< timer_impl >( get_ptr(), err, hf );
			if ( err )
			{
				result = nullptr;
				goto exit;
			}
			else
			{
				result->prep( result );
			}
		}

	exit:
		return result;
	}

	virtual void
	run( std::error_code& err ) override
	{
		clear_error( err );
		if ( ! m_uv_loop )
		{
			err = make_error_code(std::errc::state_not_recoverable );
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

	virtual void
	run() override
	{
		if ( m_uv_loop )
		{
			auto status = uv_run( m_uv_loop, UV_RUN_DEFAULT );
			UV_ERROR_THROW( status );
		}
		else
		{
			throw std::system_error{ make_error_code(std::errc::state_not_recoverable ) };
		}
	exit:
		return;
	}

	virtual void
	stop() override
	{
		if ( m_uv_loop )
		{
			uv_stop(  m_uv_loop );
		}
		else
		{
			throw std::system_error{ make_error_code( std::errc::state_not_recoverable ) };
		}
	}

	static void
	on_walk( uv_handle_t* handle, void* )
	{
		if ( handle )
		{
			auto handle_type = uv_handle_get_type( handle );
			switch( handle_type )
			{
				case uv_handle_type::UV_TIMER:
				{
					uv_timer_t* timer_handle = reinterpret_cast< uv_timer_t* >( handle );
					if ( ! uv_is_closing( handle ) )
					{
						uv_close( handle, timer_impl::on_timer_close );
					}
				}
				break;
				default:
				break;
			}
		}
	}

	virtual void
	close() override
	{
		if ( m_uv_loop )
		{
			auto status = uv_loop_close( m_uv_loop );
			if ( status == UV_EBUSY )
			{
				uv_walk( m_uv_loop, on_walk, nullptr );
				status = uv_run( m_uv_loop, UV_RUN_DEFAULT );
				UV_ERROR_THROW( status );
				status = uv_loop_close( m_uv_loop );
				UV_ERROR_THROW( status );
			}
			else if ( status < 0 )
			{
				UV_ERROR_THROW( status );
			}
			if ( ! m_is_default_loop )
			{
				delete m_uv_loop;
			}
			m_uv_loop = nullptr;
		}
	}

	virtual void
	close( std::error_code& err ) override
	{
		clear_error( err );
		if ( m_uv_loop )
		{
			auto status = uv_loop_close( m_uv_loop );
			if ( status == UV_EBUSY )
			{
				uv_walk( m_uv_loop, on_walk, nullptr );
				status = uv_run( m_uv_loop, UV_RUN_DEFAULT );
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
		}
	exit:
		return;
	}

	virtual async::resolver::ptr
	create_resolver( std::error_code& err, async::resolver::handler hf ) override
	{
		return std::make_shared< resolver_impl >( shared_from_this(), hf );
	}

	uv_loop_t*		m_uv_loop;
	bool			m_is_default_loop;
};

timer_impl::timer_impl( std::shared_ptr< loop_impl > owner, async::timer::handler handler )
:
m_owner{ owner },
m_uv_timer{ new uv_timer_t },
m_data{ nullptr },
m_handler{ handler }
{
	auto status = uv_timer_init( m_owner->m_uv_loop, m_uv_timer );
	UV_ERROR_THROW( status );

	uv_handle_set_data( reinterpret_cast< uv_handle_t* >( m_uv_timer ), &m_data );
}

timer_impl::timer_impl( std::shared_ptr< loop_impl > owner, std::error_code& err, async::timer::handler handler )
:
m_owner{ owner },
m_uv_timer{ new uv_timer_t },
m_data{ nullptr },
m_handler{ handler }
{
	clear_error( err );
	auto status = uv_timer_init( m_owner->m_uv_loop, m_uv_timer );
	UV_ERROR_CHECK( status, err, exit );

	uv_handle_set_data( reinterpret_cast< uv_handle_t* >( m_uv_timer ), &m_data );

exit:
	return;
}

std::shared_ptr< async::loop >
timer_impl::owner()
{
	return m_owner;
}



async::loop::ptr
async::loop::get_default()
{
	static async::loop::ptr default_loop = std::make_shared< loop_impl >( uv_default_loop() );
	return default_loop;
}

async::loop::ptr
async::loop::create()
{
	return std::make_shared< loop_impl >();
}

std::shared_ptr< async::loop >
resolver_impl::owner()
{
	return m_owner;
}

void
resolver_impl::resolve(  std::string const& hostname, std::error_code& err )
{
	clear_error( err );
	if ( m_uv_req )
	{
		err = make_error_code( std::errc::operation_in_progress );
	}
	else
	{
		m_uv_req = new uv_getaddrinfo_t;
		m_hostname = hostname;
		m_data.m_req_impl = shared_from_this();
		uv_req_set_data( reinterpret_cast< uv_req_t* >( m_uv_req ), &m_data );
		uv_getaddrinfo( m_owner->m_uv_loop, m_uv_req, on_resolve, m_hostname.c_str(), nullptr, nullptr );
	}
}
