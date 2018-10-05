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

#include <logicmill/async/stream.h>
#include <doctest.h>
#include <iostream>

using namespace logicmill;
using namespace async;
using namespace stream;

namespace stream_test
{
	using payload_type = std::shared_ptr< bstream::mutable_buffer >;

	class pump : public data_source< payload_type >
	{
	public:

		bool paused = false;
		bool resumed = false;
		bool got_receipt = false;

		void push( payload_type pload )
		{
			m_last_pushed = pload;
			emit( data_event< payload_type >{}, pload );
		}

		virtual void
		control( control_state state ) override
		{
			if ( state == control_state::pause )
			{
				paused = true;
				resumed = false;
			}
			else if ( state == control_state::resume )
			{
				resumed = true;
				paused = false;
			}
		}

		virtual void
		received( payload_type bp, std::error_code const& err ) override
		{
			if ( bp == m_last_pushed )
			{
				got_receipt = true;
			}
		}

		payload_type	m_last_pushed;

	};

	class drain : public data_sink< payload_type >
	{
	public:
		using control_emitter::emit;
		using receipt_emitter::emit;

		virtual void
		recv_data( payload_type pload ) override
		{
			std::error_code ecode;
			m_last_drained = pload;
			emit( receipt_event< payload_type >{}, pload, ecode ); 
		}

		payload_type	m_last_drained;

	};

	class echo : public duplex< payload_type >
	{
	public:
		using ptr = std::shared_ptr< echo >;

		using data_sink< payload_type>::receipt_emitter::emit;
		using data_sink< payload_type>::control_emitter::emit;
		using data_sink< payload_type>::receipt_emitter::add_listener;
		using data_sink< payload_type>::control_emitter::add_listener;
		using data_sink< payload_type>::data_handler::handle;
		using data_source< payload_type >::control_handler::handle;
		using data_source< payload_type >::receipt_handler::handle;
		using data_source< payload_type >::data_emitter::add_listener;

		virtual void
		recv_data( payload_type pload_in ) override
		{
			m_last_echoed = pload_in;
			data_sink< payload_type >::receipt_emitter::emit( receipt_event< payload_type >{}, pload_in, std::error_code{} );
			data_source< payload_type >::emit( data_event< payload_type >{}, pload_in );
		}

		virtual void
		control( control_state state ) override
		{
			/* data_sink< payload_type >:: */ emit( control_event{}, state );
		}

		virtual void
		received( payload_type pload, std::error_code const& err ) override
		{
			CHECK( pload ==  m_last_echoed );
		}

		payload_type m_last_echoed;

	};

	class echo_driver : public duplex< payload_type >
	{
	public:
		using ptr = std::shared_ptr< echo_driver >;

		using data_sink< payload_type>::receipt_emitter::emit;
		using data_sink< payload_type>::control_emitter::emit;

		bool buffer_echoed = false;
		bool got_receipt = false;

		payload_type m_payload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

		// std::string content{ "some buffer content" };
		// m_payload->putn( 0, content.data(), content.size() );

		void send()
		{
			std::error_code err;
			data_source< payload_type >::emit( data_event< payload_type >{}, m_payload );
		}

		virtual void
		recv_data( payload_type pload ) override
		{
			emit( receipt_event< payload_type >{}, pload, std::error_code{} );

			CHECK( m_payload == pload );

			buffer_echoed = true;
		}

		virtual void
		control( control_state state ) override
		{
//			data_sink::emit( control_event{}, state, std::error_code{} );
		}

		virtual void
		received( payload_type pload, std::error_code const& err ) override
		{
			CHECK( pload == m_payload );
			got_receipt = true;
		}

	};
}


TEST_CASE( "logicmill/async/stream/smoke/data_source/data_sink" )
{
	using namespace stream_test;

	std::error_code err;

	payload_type pload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

	auto pmp = std::make_shared< pump >();
	auto drn = std::make_shared< drain >();

	connect< payload_type >( pmp, drn, err );

	pmp->push( pload );
//	pmp->emit( data_event< payload_type >{}, pload );

	CHECK( drn->m_last_drained == pload );
	CHECK( pmp->got_receipt );

	drn->emit( control_event{}, control_state::pause  );

	CHECK( pmp->paused );
	CHECK( ! pmp->resumed );

	drn->emit( control_event{}, control_state::resume  );

	CHECK( ! pmp->paused );
	CHECK( pmp->resumed );

	pmp->disconnect();
	drn->disconnect();

}
#if 1

TEST_CASE( "logicmill/async/stream/smoke/echo" )
{
	using namespace stream_test;

	std::error_code ecode;

	auto pload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

	auto ep = std::make_shared< echo >();
	auto pmp = std::make_shared< pump >();

	bool got_echo_receipt = false;
	bool got_buffer_echo = false;

	// auto rl_id = ep->add_listener( receipt_event< payload_type >{},
	// [ & ] ( payload_type pp, std::error_code const& err )
	// {
	// 	CHECK( pp == pload );
	// 	got_echo_receipt = true;
	// } );

	auto dl_id = ep->add_listener( data_event< payload_type >{}, 
	[ & ] ( payload_type pp )
	{
		CHECK( pp == pload );
		got_buffer_echo = true;
		ep->handle( receipt_event< payload_type >{}, pp, std::error_code{} );

	} );

	connect< payload_type >( pmp, ep, ecode );

	pmp->push( pload );	
//	pmp->emit( data_event< payload_type >{}, pload );

	CHECK( pmp->got_receipt );
	CHECK( got_buffer_echo );

	pmp->disconnect();
	ep->disconnect();
}
#endif

#if 0

TEST_CASE( "logicmill/async/stream/smoke/echo_driver" )
{
	using namespace stream_test;

	std::error_code ecode;

	auto edp = std::make_shared< echo_driver >();
	auto ep = std::make_shared< echo >();

	stack( edp, ep, ecode );
	
	CHECK( ! edp->buffer_echoed );
	CHECK( ! edp->got_receipt );

	edp->send();

	CHECK( edp->buffer_echoed );
	CHECK( edp->got_receipt );

	edp->disconnect();
	ep->disconnect();

}

#endif

#if 0


namespace stream_test_1
{
	using payload_type = std::unique_ptr< std::string >&& ;

	class pump : public data_source< payload_type >
	{
	public:

		bool paused = false;
		bool resumed = false;
		bool got_receipt = false;

		void push( payload_type pload )
		{
			m_value_last_pushed = *pload;
			emit( data_event< payload_type >{}, std::move( pload ) );
		}

		virtual void
		control( control_state state ) override
		{
			if ( state == control_state::pause )
			{
				paused = true;
				resumed = false;
			}
			else if ( state == control_state::resume )
			{
				resumed = true;
				paused = false;
			}
		}

		virtual void
		received( payload_type pload, std::error_code const& err ) override
		{
			CHECK( *pload == m_value_last_pushed );
			got_receipt = true;
		}

		std::string		m_value_last_pushed;
	};

	class drain : public data_sink< payload_type >
	{
	public:
		using control_emitter::emit;
		using receipt_emitter::emit;

		virtual void
		recv_data( payload_type pload ) override
		{
			std::error_code ecode;
			m_last_value_drained = *pload;
			emit( receipt_event< payload_type >{}, std::move( pload ), ecode ); 
		}

		std::string		m_last_value_drained;

	};

#if 0
	class echo : public duplex< payload_type >
	{
	public:
		using ptr = std::shared_ptr< echo >;

		using data_sink< payload_type>::receipt_emitter::emit;
		using data_sink< payload_type>::control_emitter::emit;
		using data_sink< payload_type>::receipt_emitter::add_listener;
		using data_sink< payload_type>::control_emitter::add_listener;
		using data_sink< payload_type>::data_handler::handle;
		using data_source< payload_type >::control_handler::handle;
		using data_source< payload_type >::receipt_handler::handle;
		using data_source< payload_type >::data_emitter::add_listener;

		virtual void
		recv_data( payload_type pload_in ) override
		{
			m_last_echoed = pload_in;
			data_sink< payload_type >::receipt_emitter::emit( receipt_event< payload_type >{}, pload_in, std::error_code{} );
			data_source< payload_type >::emit( data_event< payload_type >{}, pload_in );
		}

		virtual void
		control( control_state state ) override
		{
			/* data_sink< payload_type >:: */ emit( control_event{}, state );
		}

		virtual void
		received( payload_type pload, std::error_code const& err ) override
		{
			CHECK( pload ==  m_last_echoed );
		}

		payload_type m_last_echoed;

	};

	class echo_driver : public duplex< payload_type >
	{
	public:
		using ptr = std::shared_ptr< echo_driver >;

		using data_sink< payload_type>::receipt_emitter::emit;
		using data_sink< payload_type>::control_emitter::emit;

		bool buffer_echoed = false;
		bool got_receipt = false;

		payload_type m_payload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

		// std::string content{ "some buffer content" };
		// m_payload->putn( 0, content.data(), content.size() );

		void send()
		{
			std::error_code err;
			data_source< payload_type >::emit( data_event< payload_type >{}, m_payload );
		}

		virtual void
		recv_data( payload_type pload ) override
		{
			emit( receipt_event< payload_type >{}, pload, std::error_code{} );

			CHECK( m_payload == pload );

			buffer_echoed = true;
		}

		virtual void
		control( control_state state ) override
		{
//			data_sink::emit( control_event{}, state, std::error_code{} );
		}

		virtual void
		received( payload_type pload, std::error_code const& err ) override
		{
			CHECK( pload == m_payload );
			got_receipt = true;
		}

	};

#endif 

}


TEST_CASE( "logicmill/async/stream/smoke/data_source/data_sink_rvalue" )
{
	using namespace stream_test_1;

	std::error_code err;

	payload_type pload = std::make_unique< std::string >( "some buffer content" );

	auto pmp = std::make_shared< pump >();
	auto drn = std::make_shared< drain >();

	connect< payload_type >( pmp, drn, err );

	pmp->push( std::move( pload ) );
//	pmp->emit( data_event< payload_type >{}, pload );

	CHECK( drn->m_last_value_drained == "some buffer content" );
	CHECK( pmp->got_receipt );

	drn->emit( control_event{}, control_state::pause  );

	CHECK( pmp->paused );
	CHECK( ! pmp->resumed );

	drn->emit( control_event{}, control_state::resume  );

	CHECK( ! pmp->paused );
	CHECK( pmp->resumed );

	pmp->disconnect();
	drn->disconnect();

}

#endif