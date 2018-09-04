#include <logicmill/async/loop.h>
#include <uv.h>

#define UV_ERROR_CHECK( _stat_var_, _err_var_, _label_ )					\
{																			\
	if ( _stat_var_ < 0 )													\
	{																		\
		_err_var_ = make_error_code( std::errc::operation_canceled );		\
		goto _label_;														\
	}																		\
}																			\
/**/

#define UV_ERROR_THROW( _stat_var_ )													\
{																						\
	if ( _stat_var_ < 0 )																\
	{																					\
		throw std::system_error{ make_error_code( std::errc::operation_canceled ) };	\
	}																					\
}																						\
/**/

using namespace logicmill;

static void
clear_error( std::error_code& err )
{
	static std::error_code no_error;
	err = no_error;
}

class loop_impl;

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

