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
#include <logicmill/bstream/buffer.h>
#include <logicmill/async/event.h>
#include <logicmill/async/error.h>

namespace logicmill
{
namespace async
{
namespace stream
{

/*

source-sink interaction

If a sink accepts a message and enqueues it locally,
the sink (in essence) takes ownership of the message.

A sink may refuse to accept a message (by returning a busy error code),
the source should expect a subsequent event from the sink, either
resume or shutdown.

shutdowns may occur at any terminus, and should be propagated up or down the stack

*/


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

#if 0

class payload
{
public:

	using ptr = std::shared_ptr< payload >;
	using id_type = std::uint64_t;

	payload( std::deque< mutable_buffer >&& buffers, id_type id )
	:
	m_buffers{ std::move( buffers ) },
	m_id{ id },
	m_size{ count() },
	m_possibly_dirty{ false }
	{}

	std::deque< mutable_buffer >&
	buffers()
	{
		m_possibly_dirty = true;
		return m_buffers;
	}

	std::deque< mutable_buffer > const&
	buffers() const
	{
		return m_buffers;
	}

	id_type
	id() const
	{
		return m_id;
	}

	std::size_t
	size() const
	{
		if ( m_possibly_dirty )
		{
			m_size = count();
			m_possibly_dirty = false;
		}
		return m_size;
	}

private:

	std::size_t
	count() const
	{
		std::size_t total = 0;
		for ( auto& buf : m_buffers )
		{
			total += buf.size();
		}
		return total;
	}

	std::deque< mutable_buffer >		m_buffers;
	id_type								m_id;
	mutable std::size_t					m_size;
	mutable bool						m_possibly_dirty;
};
#endif 

#if 0
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

#endif

template< class Payload >
using data_event = event< stream_event, stream_event::data, Payload >;

using control_event = event< stream_event, stream_event::control, control_state >;

template< class Payload >
using receipt_event = event< stream_event, stream_event::received, Payload, std::error_code const& >;

template< class Payload >
class data_sink : public handler_spec< data_event< Payload >, data_sink< Payload > >,
public emitter< control_event, cardinality::simplex >,
public emitter< receipt_event< Payload >, cardinality::simplex >
{
public:
	using ptr = std::shared_ptr< data_sink >;
	using data_handler = handler_spec< data_event< Payload >, data_sink >;
	using control_emitter = emitter< control_event, cardinality::simplex >;
	using receipt_emitter = emitter< receipt_event< Payload >, cardinality::simplex >;

	using control_emitter::add_listener;
	using receipt_emitter::add_listener;


	// LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( control_event );
	// LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( receipt_event< Payload > );

	data_sink()
	:
	data_handler{ &data_sink::recv_data }
	{}

	virtual void
	recv_data( Payload pload ) = 0;

	virtual void disconnect() override
	{
		control_emitter::disconnect();
		receipt_emitter::disconnect();
	}

};

template< class Payload >
class data_source : 
	public handler_spec< control_event, data_source< Payload> >, 
	public handler_spec< receipt_event< Payload >, data_source< Payload > >, 
	public emitter< data_event< Payload >, cardinality::simplex >
{
public:
	using ptr = std::shared_ptr< data_source< Payload > >;
	using data_emitter = emitter< data_event< Payload >, cardinality::simplex >;
	using control_handler = handler_spec< control_event, data_source< Payload > >;
	using receipt_handler = handler_spec< receipt_event< Payload >, data_source< Payload > >;


	data_source()
	:
	control_handler{ &data_source::control },
	receipt_handler{ &data_source::received }
	{}

	virtual void
	control( control_state state ) = 0;

	virtual void
	received( Payload pload, std::error_code const& err ) = 0;

	virtual void disconnect() override
	{
		data_emitter::disconnect();
	}

};

template< class Payload >
void connect( typename data_source< Payload >::ptr src, typename data_sink< Payload >::ptr sink, std::error_code& err )
{
	err.clear();
	src->add_listener( data_event< Payload >{}, sink );
	sink->add_listener( control_event{}, src );
	sink->add_listener( receipt_event< Payload >{}, src );
}

template< class SourcePayload, class SinkPayload = SourcePayload >
class duplex : public data_source< SourcePayload >, public data_sink< SinkPayload >
{
public:
	using ptr = std::shared_ptr< duplex >;

	// LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( control_event );
	// LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( receipt_event< SinkPayload > );
	// LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( data_event< SourcePayload > );
	// LOGICMILL_ASYNC_EVENT_USE_HANDLER_BASE( control_event );
	// LOGICMILL_ASYNC_EVENT_USE_HANDLER_BASE( receipt_event< SourcePayload > );
	// LOGICMILL_ASYNC_EVENT_USE_HANDLER_BASE( data_event< SinkPayload > );

	virtual void disconnect() override
	{
		data_source< SourcePayload >::disconnect();
		data_sink< SinkPayload >::disconnect();
	}
};

template< class Payload >
void stack( typename duplex< Payload >::ptr a, typename duplex< Payload >::ptr b, std::error_code& err )
{
	err.clear();
	connect( a, b, err );
	connect( b, a, err );
}

template< class TopPayload, class BottomPayload = TopPayload >
class stackable
{
public:

	virtual ~stackable() {}

	virtual typename duplex< TopPayload >::ptr top() = 0;

	virtual typename duplex< BottomPayload >::ptr bottom() = 0;

	virtual void disconnect() = 0;

};

template< class Payload >
class pass_thru : public stackable< Payload >
{
private:

	class pass_thru_surface : public duplex< Payload >
	{
	public:		

		void
		opposite( std::shared_ptr< pass_thru_surface > opp )
		{
			m_opposite = opp;
		}

		virtual void
		recv_data( Payload pload ) override
		{
			typename duplex< Payload >::ptr{ m_opposite }->data_source::emit( async::stream::data_event< Payload >{}, pload );
		}

		virtual void
		control( async::stream::control_state state, std::error_code const& err ) override
		{
			typename duplex< Payload >::ptr{ m_opposite }->data_sink::emit( async::stream::control_event{}, state );
		}

		virtual void
		received( Payload pload, std::error_code const& err ) override
		{
			typename duplex< Payload >::ptr{ m_opposite }->data_sink::emit( async::stream::receipt_event< Payload >{}, pload, err );
		}		

	private:

		std::weak_ptr< pass_thru_surface >					m_opposite;
	};

public:

	pass_thru()
	:
	m_top{ std::make_shared< pass_thru_surface >() },
	m_bottom{ std::make_shared< pass_thru_surface >() }
	{
		m_top->opposite( m_bottom );
		m_bottom->opposite( m_top );
	}

	virtual typename duplex< Payload >::ptr top() override
	{
		return m_top;
	}

	virtual typename duplex< Payload >::ptr bottom() override
	{
		return m_bottom;
	}
	
	virtual void disconnect() override
	{
		m_top->disconnect();
		m_bottom->disconnect();
	}

private:

	std::shared_ptr< pass_thru_surface >					m_top;
	std::shared_ptr< pass_thru_surface >					m_bottom;
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