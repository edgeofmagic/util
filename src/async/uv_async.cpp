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

class loop_impl;

class resolver_impl : public async::resolver, public std::enable_shared_from_this< resolver_impl >
{
public:

	using ptr = std::shared_ptr< resolver_impl >;

	struct req_data
	{
		resolver_impl::ptr	m_req_impl;
	};

	resolver_impl( std::shared_ptr< loop_impl > owner, std::string const& hostname, async::resolver::handler handler )
	:
	m_owner{ owner },
	m_uv_req{ new uv_getaddrinfo_t },
	m_data{ nullptr },
	m_hostname{ hostname },
	m_handler{ handler }
	{}

	void 
	start();

	void 
	start( std::error_code& err );

	virtual ~resolver_impl()
	{
		std::cout << "in resolver_impl destructor" << std::endl;
		assert( m_uv_req == nullptr );
	}

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

		uv_freeaddrinfo( result );

		auto p = uv_req_get_data( reinterpret_cast< uv_req_t* >( req ) );
		auto data = reinterpret_cast< resolver_impl::req_data* >( p );
		auto req_impl = data->m_req_impl;

		data->m_req_impl->m_handler( data->m_req_impl, std::move( addresses ), err );

		assert( ( void* )req == ( void* )( req_impl->m_uv_req ) );
		data->m_req_impl->wipe();
		data->m_req_impl.reset(); // kill self-reference
		delete req;
	}

	void
	wipe()
	{
		m_hostname.clear();
		m_owner.reset();
		m_handler = nullptr;
		m_uv_req = nullptr;
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

	std::shared_ptr< loop_impl >		m_owner;
	uv_getaddrinfo_t*					m_uv_req;
	req_data							m_data;
	std::string							m_hostname;
	async::resolver::handler			m_handler;

};

class timer_impl : public async::timer, public std::enable_shared_from_this< timer_impl >
{
public:

	using ptr = std::shared_ptr< timer_impl >;

	struct timer_data
	{
		timer_impl::ptr		m_timer_impl;
	};

	timer_impl( std::shared_ptr< loop_impl > owner, async::timer::handler handler )
	:
	m_owner{ owner },
	m_uv_timer{ nullptr },
	m_data{ nullptr },
	m_handler{ handler }
	{}

	virtual ~timer_impl()
	{
		std::cout << "in timer_impl destructor" << std::endl;
		assert( m_uv_timer == nullptr );
	}

	void
	init_uv_timer( std::error_code& err );

	void
	init_uv_timer();

	virtual void
	start( std::chrono::milliseconds timeout ) override
	{
		if ( ! m_owner ) // on_close clears the owner
		{
			throw std::system_error{ make_error_code( async::errc::timer_closed ) };
		}

		if ( ! m_uv_timer )
		{
			init_uv_timer();
		}

		if ( is_active() )
		{
			throw std::system_error{ make_error_code( std::errc::operation_in_progress ) };
		}

		auto status = uv_timer_start( m_uv_timer, on_timer_expire, timeout.count(), 0 );
		UV_ERROR_THROW( status );
	}

	virtual void
	start( std::chrono::milliseconds timeout, std::error_code& err ) override
	{
		int status = 0;
		err.clear();

		if ( ! m_owner ) // on_close clears the owner
		{
			err = make_error_code( async::errc::timer_closed );
			goto exit;
		}
		
		if ( ! m_uv_timer )
		{
			init_uv_timer( err );
			if ( err ) goto exit;
		}

		if ( is_active() )
		{
			err = make_error_code( std::errc::operation_in_progress );
			goto exit;
		}

		status = uv_timer_start( m_uv_timer, on_timer_expire, timeout.count(), 0 );
		UV_ERROR_CHECK( status, err, exit );

	exit:
		return;
	}

	virtual void
	stop() override
	{
		if ( ! m_uv_timer )
		{
			throw std::system_error{ make_error_code( async::errc::timer_closed ) };
		}

		if ( is_active() )
		{
			auto status = uv_timer_stop( m_uv_timer );
			UV_ERROR_THROW( status );
		}
	}

	virtual void
	stop( std::error_code& err ) override
	{
		err.clear();

		if ( ! m_uv_timer )
		{
			err = make_error_code( async::errc::timer_closed );
			goto exit;
		}

		if ( is_active() )
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

	bool is_active() const
	{
		return m_uv_timer && uv_is_active( reinterpret_cast< uv_handle_t* >( m_uv_timer ) );
	}

	virtual bool
	is_pending() const override
	{
		return is_active();
	}

	void
	wipe()
	{
		m_uv_timer = nullptr;
		m_owner.reset();
		m_handler = nullptr;
	}

	static void
	on_timer_close( uv_handle_t* handle )
	{
		assert( uv_handle_get_type( handle ) == uv_handle_type::UV_TIMER );
		uv_timer_t* timer_handle = reinterpret_cast< uv_timer_t* >( handle );
		timer_data* tdata = reinterpret_cast< timer_data* >( uv_handle_get_data( handle ) );
		assert( (void*)tdata->m_timer_impl->m_uv_timer == (void*)handle );
		tdata->m_timer_impl->wipe();
		tdata->m_timer_impl.reset(); // release the self-reference
		delete timer_handle;
	}

	static void
	on_timer_expire( uv_timer_t* handle )
	{
		timer_data* tdata = reinterpret_cast< timer_data* >( uv_handle_get_data( reinterpret_cast< uv_handle_t* >( handle ) ) );
		auto t_impl = tdata->m_timer_impl;
		t_impl->m_handler( t_impl );

		if ( ! uv_is_active( reinterpret_cast< uv_handle_t* >( handle ) ) )
		{
			std::cout << "timer refcount is " << t_impl.use_count() << std::endl;
			if ( t_impl.use_count() <= 2 )
			{
				// Two references: this one (t_impl) and the self-reference in timer_data.
				// The timer is not active, so when t_impl goes out of scope, the
				// timer will become unusable ( no pointer other then the self-reference exists),
				// and it will constitute a memory leak. Close it.
				std::cout << "closing timer: low refcount" << std::endl;
				t_impl->close();

			}
		}
	}

	std::shared_ptr< loop_impl >	m_owner;
	uv_timer_t*						m_uv_timer;
	timer_data						m_data;
	async::timer::handler			m_handler;

};

class loop_impl : public async::loop, public std::enable_shared_from_this< loop_impl>
{
public:

	struct use_default_loop {};

	using ptr = std::shared_ptr< loop_impl >;

	loop_impl( loop_impl const& ) = delete;
	loop_impl( loop_impl&& ) = delete;

	loop_impl( use_default_loop flag )	// only use to construct for default loop
	:
	m_uv_loop{ uv_default_loop() },
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

	virtual ~loop_impl()
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

	virtual async::timer::ptr
	create_timer( async::timer::handler hf ) override
	{
		if ( ! m_uv_loop )
		{
			throw std::system_error{ make_error_code( async::errc::loop_closed ) };
		}
		return std::make_shared< timer_impl >( shared_from_this(), hf );
	}

	virtual async::timer::ptr
	create_timer( std::error_code& err, async::timer::handler hf ) override
	{
		err.clear();

		async::timer::ptr result;

		if ( ! m_uv_loop )
		{
			err = make_error_code( async::errc::loop_closed );
			goto exit;
		}
		result = std::make_shared< timer_impl >( shared_from_this(), hf );
		if ( err ) goto exit;

	exit:
		return result;
	}

	virtual void
	run() override
	{
		if ( ! m_uv_loop )
		{
			throw std::system_error{ make_error_code( async::errc::loop_closed ) };
		}
		auto status = uv_run( m_uv_loop, UV_RUN_DEFAULT );
		UV_ERROR_THROW( status );
	}

	virtual void
	run( std::error_code& err ) override
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

	virtual void
	stop() override
	{
		if ( ! m_uv_loop )
		{
			throw std::system_error{ make_error_code( async::errc::loop_closed ) };
		}
		uv_stop(  m_uv_loop );
	}

	virtual void
	stop( std::error_code& err ) override
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
						std::cout << "closing timer: on loop closure" << std::endl;
						uv_close( handle, timer_impl::on_timer_close );
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

	virtual void
	close() override  // probably should NOT be called from any handler
	{
		std::cout << "starting loop close" << std::endl;
		if ( ! m_uv_loop )
		{
			throw std::system_error{ make_error_code( async::errc::loop_closed ) };
		}
		
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

	virtual void
	close( std::error_code& err ) override // probably should NOT be called from any handler
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

	virtual async::resolver::ptr
	create_resolver( std::string const& hostname, async::resolver::handler hf ) override
	{
		if ( ! m_uv_loop )
		{
			throw std::system_error{ make_error_code( async::errc::loop_closed ) };
		}

		auto result = std::make_shared< resolver_impl >( shared_from_this(), hostname, hf );
		result->start();
		return result;
	}

	virtual async::resolver::ptr
	create_resolver( std::string const& hostname, std::error_code& err, async::resolver::handler hf ) override
	{
		err.clear();
		resolver_impl::ptr result;

		if ( ! m_uv_loop )
		{
			err = make_error_code( async::errc::loop_closed );
			goto exit;
		}

		result = std::make_shared< resolver_impl >( shared_from_this(), hostname, hf );
		result->start( err );
		if ( err ) goto exit;

	exit:
		return result;
	}

	uv_loop_t*		m_uv_loop;
	bool			m_is_default_loop;
};

std::shared_ptr< async::loop >
timer_impl::owner()
{
	return m_owner;
}

void
timer_impl::init_uv_timer( std::error_code& err )
{
	assert( m_uv_timer == nullptr );

	err.clear();

	m_uv_timer = new uv_timer_t;
	auto status = uv_timer_init( m_owner->m_uv_loop, m_uv_timer );
	UV_ERROR_CHECK( status, err, exit );

	m_data.m_timer_impl = shared_from_this();

	uv_handle_set_data( reinterpret_cast< uv_handle_t* >( m_uv_timer ), &m_data );

exit:
	return;
}

void
timer_impl::init_uv_timer()
{
	assert( m_uv_timer == nullptr );

	m_uv_timer = new uv_timer_t;
	auto status = uv_timer_init( m_owner->m_uv_loop, m_uv_timer );
	UV_ERROR_THROW( status );

	if ( ! m_data.m_timer_impl )
	{
		m_data.m_timer_impl = shared_from_this();
	}

	uv_handle_set_data( reinterpret_cast< uv_handle_t* >( m_uv_timer ), &m_data );
}

async::loop::ptr
async::loop::get_default()
{
	static async::loop::ptr default_loop = std::make_shared< loop_impl >( loop_impl::use_default_loop{} );
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
resolver_impl::start()
{
	m_data.m_req_impl = shared_from_this();
	uv_req_set_data( reinterpret_cast< uv_req_t* >( m_uv_req ), &m_data );
	auto status = uv_getaddrinfo( m_owner->m_uv_loop, m_uv_req, on_resolve, m_hostname.c_str(), nullptr, nullptr );
	UV_ERROR_THROW( status );	
}

void 
resolver_impl::start( std::error_code& err )
{
	err.clear();
	m_data.m_req_impl = shared_from_this();
	uv_req_set_data( reinterpret_cast< uv_req_t* >( m_uv_req ), &m_data );
	auto status = uv_getaddrinfo( m_owner->m_uv_loop, m_uv_req, on_resolve, m_hostname.c_str(), nullptr, nullptr );
	UV_ERROR_CHECK( status, err, exit );	

exit:
	return;
}
