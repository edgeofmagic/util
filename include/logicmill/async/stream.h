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

#ifndef LOGICMILL_ASYNC_STREAM_H
#define LOGICMILL_ASYNC_STREAM_H

#include <memory>
#include <functional>
#include <system_error>
#include <chrono>
#include <logicmill/buffer.h>
#include <logicmill/async/event.h>
#include <logicmill/async/error.h>

namespace logicmill
{
namespace async
{
namespace stream
{

enum class stream_event
{
	data,
	control,
	received
};

enum class control_state
{
	pause,
	resume,
	shutdown
};

class receipt
{
public:
	using ptr = std::shared_ptr< receipt >;
	using cancel_listener = std::function< void ( std::error_code const& err ) >;

	receipt()
	:
	m_listener_told{ false },
	m_is_canceled{ false },
	m_cancel_error{},
	m_cancel_listener{ nullptr }
	{}

	virtual void really_cancel() {}

	bool is_canceled() const
	{
		return m_is_canceled;
	}

	void cancel( std::error_code const& err )
	{
		if ( ! m_is_canceled )
		{
			m_cancel_error = err;
			m_is_canceled = true;
			if ( m_cancel_listener )
			{
				m_cancel_listener( err );
				m_listener_told = true;
				m_cancel_listener = nullptr;
			}
			really_cancel();
		}
	}

	void on_cancel( cancel_listener lstnr )
	{
		if ( m_is_canceled )
		{
			if ( ! m_listener_told )
			{
				lstnr( m_cancel_err );
				m_listener_told = true;
			}
		}
		else
		{
			assert( ! m_listener_told );
			m_cancel_listener = lstnr;
		}
	}

private:
	bool							m_listener_told;
	bool							m_is_canceled;
	std::error_code					m_cancel_err;
	cancel_listener					m_cancel_listener;
};

using data_event = event_definition< stream_event, stream_event::data, const_buffer, receipt::ptr, std::error_code const& >;
using control_event = event_definition< stream_event, stream_event::control, control_state, std::error_code const& >;
using receipt_event = event_definition< stream_event, stream_event::received, receipt::ptr, std::error_code const& >;

class data_sink : public data_event::handler_spec< data_sink >, public control_event::emitter, public receipt_event::emitter
{
public:
	using ptr = std::shared_ptr< data_sink >;
	using sink_base = data_event::handler_spec< data_sink >;

	LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( control_event );
	LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( receipt_event );

	data_sink()
	:
	sink_base{ &data_sink::recv_data }
	{}

	virtual void
	recv_data( const_buffer buf, receipt::ptr rp, std::error_code const& err ) = 0;

	virtual void disconnect() override
	{
		control_event::emitter::disconnect();
		receipt_event::emitter::disconnect();
	}

};

class data_source : 
	public control_event::handler_spec< data_source >, 
	public receipt_event::handler_spec< data_source >, 
	public data_event::emitter
{
public:
	using ptr = std::shared_ptr< data_source >;

	data_source()
	:
	control_event::handler_spec< data_source >{ &data_source::control },
	receipt_event::handler_spec< data_source >{ &data_source::received }
	{}

	virtual void
	control( control_state state, std::error_code const& err ) = 0;

	virtual void
	received( receipt::ptr rp, std::error_code const& err ) = 0;

	virtual receipt::ptr
	create_receipt()
	{
		return std::make_shared< receipt >();
	}

	virtual void disconnect() override
	{
		data_event::emitter::disconnect();
	}

};

void connect( data_source::ptr src, data_sink::ptr sink, std::error_code& err )
{
	clear_error( err );
	src->add_handler( data_event{}, sink );
	sink->add_handler( control_event{}, src );
	sink->add_handler( receipt_event{}, src );
}



/*
class duplex : 
	public multi_emitter< data_event, control_event >, 
	public data_event::handler_spec< duplex >, 
	public control_event::handler_spec< duplex >
{


};
*/

class duplex : public data_source, public data_sink
{
public:
	using ptr = std::shared_ptr< duplex >;

	LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( control_event );
	LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( receipt_event );
	LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( data_event );
	LOGICMILL_ASYNC_EVENT_USE_HANDLER_BASE( control_event );
	LOGICMILL_ASYNC_EVENT_USE_HANDLER_BASE( receipt_event );
	LOGICMILL_ASYNC_EVENT_USE_HANDLER_BASE( data_event );

	virtual void disconnect() override
	{
		data_source::disconnect();
		data_sink::disconnect();
	}
};


void stack( duplex::ptr a, duplex::ptr b, std::error_code& err )
{
	clear_error( err );
	connect( a, b, err );
	connect( b, a, err );
}

class stackable
{
public:

	virtual ~stackable() {}

	virtual duplex::ptr top() = 0;

	virtual duplex::ptr top() = 0;

	virtual void disconnect() = 0;

};

class pass_thru : public stackable
{
public:

private:

	class bottom_surface;

	class top_surface : public duplex
	{
	public:

		struct _receipt : public receipt
		{
			_receipt( const_buffer const& b )
			:
			receipt{}, 
			buf{ b }
			{}

			_receipt()
			:
			receipt{},
			buf{}
			{}

			virtual void really_cancel() override
			{
				buf = const_buffer{};
			}

			const_buffer	buf;
		};

		

		virtual void
		recv_data( const_buffer buf, async::stream::receipt::ptr rp, std::error_code const& err ) override
		{
			static const std::error_code success;

			if ( err )
			{
				auto rcpt = std::make_shared< _receipt >();
			}
			if ( m_paused )
			{
				data_sink::emit( receipt_event{}, rp, make_error_code( std::errc::device_or_resource_busy ) );
			}
			else
			{
				auto rcpt = create_receipt();
				data_source::emit( data_event{}, buf, rcpt, std::error_code{})
				data_sink::emit( async::stream::receipt_event{}, rp, std::error_code{} );

			}

			my_receipt = create_receipt();

			duplex::ptr bottom{ m_bottom };
			m_bottom->emit( async::stream::data_event{}, buf, my_receipt, std::error_code{} );
		}

		virtual void
		control( async::stream::control_state state, std::error_code const& err ) override
		{
			data_sink::emit( async::stream::control_event{}, state, std::error_code{} );
		}

		virtual void
		received( async::stream::receipt::ptr rp, std::error_code const& err ) override
		{
			auto found = m_outstanding_receipts.erase( rp );
			assert( found );
		}		

	private:

		bool									m_paused;
		std::deque< receipt::ptr >				m_outstanding_receipts;
		std::weak_ptr< duplex > 				m_bottom;
	};

	std::shared_ptr< top_surface >				m_top;
	std::shared_ptr< bottom_surface >			m_bottom;
};

#if 0
class readable
{
public:
	using ptr = str::shared_ptr< readable >;

	virtual ~readable() {}

	using read_handler = std::function< void ( readable::ptr stream, buffer&& buf, std::error_code const& err ) >;

	virtual void
	start_read( buffer::size_type size, std::error_code& err, read_handler handler ) = 0;

	virtual void
	stop_read( std::error_code& err ) = 0;

	virtual void
	close() = 0;

	virtual void
	close( std::error_code& err ) = 0;
};

class writable
{
public:
	using ptr = std::shared_ptr< writable >;

	using buffer_list = std::deque< mutable_buffer >;

	virtual ~writable() {}

	using write_multi_handler = std::function< void ( writable::ptr stream, buffer_list&& buffers, std::error_code const& err ) >;
	using write_single_handler = std::function< void ( writable::ptr stream, mutable_buffer&& buffers, std::error_code const& err ) >;

	virtual void
	write( buffer_list&& buffers, write_multi_handler handler ) = 0;

	virtual void
	write( buffer_list&& buffers, std::error_code& err, write_multi_handler handler ) = 0;

	virtual void
	write( mutable_buffer&& buf, write_single_handler ) = 0;

	virtual void
	write( mutable_buffer&& buf, std::error_code& err, write_single_handler ) = 0;

	virtual void
	close() = 0;

	virtual void
	close( std::error_code& err ) = 0;

	virtual void
	shutdown() = 0;

	virtual void
	shutdown( std::error_code& err ) = 0;
};

#endif

} // namespace stream
} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_STREAM_H