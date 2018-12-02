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
	using test_payload_type = std::shared_ptr< bstream::mutable_buffer >;

	class pump : public data_out< test_payload_type, pump >
	{
	public:

		using source_base< data_event< test_payload_type > >::send;
		using source_base< cancel_event >::send;

		bool paused = false;
		bool resumed = false;
		bool got_receipt = false;

		test_payload_type pload_sent = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

		void push()
		{
			send< data_event< test_payload_type > >( pload_sent, 0, [=] ( id_type id, std::error_code const& err )
			{
				CHECK( ! err );
				CHECK( id == 0 );
				got_receipt = true;
			});
		}

		void cancel()
		{
			send< cancel_event >( 0 );
		}

		void
		on(control_event, control_state state)
		{
			if (state == control_state::pause)
			{
				paused = true;
				resumed = false;
			}
			else if (state == control_state::resume)
			{
				resumed = true;
				paused = false;
			}
		}
	};

	class drain : public data_in< test_payload_type, drain >
	{
	public:

		bool canceled = false;

		test_payload_type pload_received = nullptr;

		void
		on( data_event< test_payload_type >,  test_payload_type pload, id_type id, receipt rcpt )
		{
			std::error_code ecode;
			pload_received = pload;
			rcpt( id, ecode );
		}

		void
		on( cancel_event, id_type id )
		{
			canceled = true;
		}

	};

#if 1

	class echo : public data_out< test_payload_type, echo >, public data_in< test_payload_type, echo >
	{
	public:

		using data_source_base = data_out< test_payload_type, echo >::source_base< data_event< test_payload_type > >;
		using cancel_source_base = data_out< test_payload_type, echo >::source_base< cancel_event >;
		using control_source_base = data_in< test_payload_type, echo >::source_base< control_event >;

		using data_source_base::send;
		using cancel_source_base::send;
		using control_source_base::send;

		using data_out< test_payload_type, echo >::get_connector;
		using data_in< test_payload_type, echo >::get_connector;

		void
		on( data_event< test_payload_type>,  test_payload_type pload_in, id_type id, receipt rcpt )
		{
			m_last_echoed = pload_in;
			send< data_event< test_payload_type > >( pload_in, id, rcpt );
		}

		void
		on( cancel_event, id_type id )
		{
			send< cancel_event >( id );
		}

		void
		on( control_event, control_state state )
		{
			send< control_event >( state );
		}

		test_payload_type m_last_echoed;

	};

	class echoplex : public duplex< test_payload_type, echoplex >
	{
	public:

		void
		on( data_event< test_payload_type>,  test_payload_type pload_in, id_type id, receipt rcpt )
		{
			m_last_echoed = pload_in;
			send< data_event< test_payload_type > >( pload_in, id, rcpt );
		}

		void
		on( cancel_event, id_type id )
		{
			send< cancel_event >( id );
		}

		void
		on( control_event, control_state state )
		{
			send< control_event >( state );
		}

		test_payload_type m_last_echoed;

	};

	class echo_driver : public duplex< test_payload_type, echo_driver >
	{
	public:
		test_payload_type m_payload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

		bool buffer_echoed = false;
		bool got_receipt = false;

		void push()
		{
			std::error_code err;
			send< data_event< test_payload_type > >( m_payload, 0,
			[=] ( id_type id, std::error_code const& err )
			{
				CHECK( ! err );
				got_receipt = true;
			} );
		}

		void
		on( data_event< test_payload_type >, test_payload_type pload, id_type id, receipt rcpt )
		{
			rcpt( id, std::error_code{} );
			CHECK( m_payload == pload );
			buffer_echoed = true;
		}

		void
		on( cancel_event, id_type id )
		{}

		void
		on( control_event, control_state state )
		{}
	};
#endif

	template< class T >
	class pass_thru;

	template< class Payload >
	class pass_thru_surface : public duplex< Payload, pass_thru_surface< Payload > >
	{
	public:

		friend class pass_thru< Payload >;

		template< class P, class R >
		void
		on( data_event< Payload >, P&& pload, id_type id, R&& rcpt )
		{
			m_opposite-> template send< data_event< Payload > >( std::forward< P >( pload ), id, std::forward< R >( rcpt ) );
		}

		void
		on( cancel_event, id_type id )
		{
			m_opposite-> template send< cancel_event >( id );
		}

		void
		on( control_event, control_state state )
		{
			m_opposite-> template send< control_event >( state );
		}

	private:

		void mirror( pass_thru_surface* opposite )
		{
			m_opposite = opposite;
		}

		pass_thru_surface() : m_opposite{ nullptr } {}

		pass_thru_surface*	m_opposite;
	};

	template< class Payload >
	class pass_thru
	{
	public:
		pass_thru()
		{
			m_top.mirror( &m_bottom );
			m_bottom.mirror( &m_top );
		}

		pass_thru_surface< Payload >& get_top()
		{
			return m_top;
		}

		pass_thru_surface< Payload >& get_bottom()
		{
			return m_bottom;
		}

	private:
		pass_thru_surface< Payload >	m_top;
		pass_thru_surface< Payload >	m_bottom;
	};
}

TEST_CASE( "logicmill/async/stream/smoke/data_connectables" )
{
	using namespace stream_test;

	std::error_code err;

	test_payload_type pload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

	pump pmp;
	drain drn;

	pmp.get_connector< data_out_connector< test_payload_type > >().mate( drn.get_connector< data_in_connector< test_payload_type > >() );

	pmp.push();

	CHECK( drn.pload_received == pmp.pload_sent );
	CHECK( pmp.got_receipt );

	drn.send< control_event>( control_state::pause  );

	CHECK( pmp.paused );
	CHECK( ! pmp.resumed );

	drn.send< control_event >( control_state::resume  );

	CHECK( ! pmp.paused );
	CHECK( pmp.resumed );

	pmp.cancel();

	CHECK( drn.canceled );

}

#if 1

TEST_CASE( "logicmill/async/stream/smoke/echo" )
{
	using namespace stream_test;

	std::error_code ecode;

	auto pload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

	echo ep;
	pump pmp;
	drain drn;

	bool got_echo_receipt = false;
	bool got_buffer_echo = false;

	pmp.get_connector< data_out_connector< test_payload_type > >().mate( ep.get_connector< data_in_connector< test_payload_type > >() );
	drn.get_connector< data_in_connector< test_payload_type > >().mate( ep.get_connector< data_out_connector< test_payload_type > >() );
	
	pmp.push();	

	CHECK( drn.pload_received == pmp.pload_sent );
	CHECK( pmp.got_receipt );

	drn.send< control_event>( control_state::pause  );

	CHECK( pmp.paused );
	CHECK( ! pmp.resumed );

	drn.send< control_event >( control_state::resume  );

	CHECK( ! pmp.paused );
	CHECK( pmp.resumed );

	pmp.cancel();

	CHECK( drn.canceled );
}

TEST_CASE( "logicmill/async/stream/smoke/echoplex" )
{
	using namespace stream_test;

	std::error_code ecode;

	auto pload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

	echoplex ep;
	pump pmp;
	drain drn;

	bool got_echo_receipt = false;
	bool got_buffer_echo = false;

	pmp.mate( drn );

	pmp.push();

	CHECK( drn.pload_received == pmp.pload_sent );
	CHECK( pmp.got_receipt );

	drn.send< control_event>( control_state::pause  );

	CHECK( pmp.paused );
	CHECK( ! pmp.resumed );

	drn.send< control_event >( control_state::resume  );

	CHECK( ! pmp.paused );
	CHECK( pmp.resumed );

	pmp.cancel();

	CHECK( drn.canceled );
}

#endif

TEST_CASE( "logicmill/async/stream/smoke/stack" )
{
	using namespace stream_test;

	std::error_code ecode;

	echo_driver edp;
	echoplex ep;

	edp.stack( ep );
	
	CHECK( ! edp.buffer_echoed );
	CHECK( ! edp.got_receipt );

	edp.push();

	CHECK( edp.buffer_echoed );
	CHECK( edp.got_receipt );
}

TEST_CASE( "logicmill/async/stream/smoke/pass_thru" )
{
	using namespace stream_test;

	std::error_code ecode;

	echo_driver edp;
	pass_thru< test_payload_type > pt;
	echoplex ep;

	edp.stack( pt.get_top() );
	pt.get_bottom().stack( ep );
	
	CHECK( ! edp.buffer_echoed );
	CHECK( ! edp.got_receipt );

	edp.push();

	CHECK( edp.buffer_echoed );
	CHECK( edp.got_receipt );
}

#if 1

namespace stream_test_1
{
	using test_payload_type = std::unique_ptr< std::string > ;

	class pump : public data_out< test_payload_type, pump >
	{
	public:
		using source_base< data_event< test_payload_type > >::send;
		using source_base< cancel_event >::send;

		bool paused = false;
		bool resumed = false;
		bool got_receipt = false;


		void push( test_payload_type&& pload )
		{
			m_value_last_pushed = *pload;
			send< data_event< test_payload_type > >( std::move( pload ), 0, [=] ( id_type id, std::error_code const& err )
			{
				CHECK( ! err );
				got_receipt = true;
			});
		}

		void
		on( control_event, control_state state )
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

		std::string		m_value_last_pushed;
	};

	class drain : public data_in< test_payload_type, drain >
	{
	public:

		template< class P, class R >
		void
		on( data_event< test_payload_type >, P&& pload, id_type id, R&& rcpt )
		{
			m_last_value_drained = *pload;
			rcpt( id, std::error_code{} );
		}

		void
		on( cancel_event, id_type id )
		{

		}

		std::string		m_last_value_drained;
	};

#if 0
	class echo : public duplex< test_payload_type >
	{
	public:
		using ptr = std::shared_ptr< echo >;

		using data_receiver< test_payload_type>::receipt_emitter::emit;
		using data_receiver< test_payload_type>::control_emitter::emit;
		using data_receiver< test_payload_type>::receipt_emitter::add_listener;
		using data_receiver< test_payload_type>::control_emitter::add_listener;
		using data_receiver< test_payload_type>::data_handler::handle;
		using data_sender< test_payload_type >::control_handler::handle;
		using data_sender< test_payload_type >::receipt_handler::handle;
		using data_sender< test_payload_type >::data_emitter::add_listener;

		virtual void
		recv_data( test_payload_type pload_in ) override
		{
			m_last_echoed = pload_in;
			data_receiver< test_payload_type >::receipt_emitter::emit( receipt_event< test_payload_type >{}, pload_in, std::error_code{} );
			data_sender< test_payload_type >::emit( data_event< test_payload_type >{}, pload_in );
		}

		virtual void
		control( control_state state ) override
		{
			/* data_receiver< test_payload_type >:: */ emit( control_event{}, state );
		}

		virtual void
		received( test_payload_type pload, std::error_code const& err ) override
		{
			CHECK( pload ==  m_last_echoed );
		}

		test_payload_type m_last_echoed;

	};

	class echo_driver : public duplex< test_payload_type >
	{
	public:
		using ptr = std::shared_ptr< echo_driver >;

		using data_receiver< test_payload_type>::receipt_emitter::emit;
		using data_receiver< test_payload_type>::control_emitter::emit;

		bool buffer_echoed = false;
		bool got_receipt = false;

		test_payload_type m_payload = std::make_shared< bstream::mutable_buffer >( "some buffer content" );

		// std::string content{ "some buffer content" };
		// m_payload->putn( 0, content.data(), content.size() );

		void send()
		{
			std::error_code err;
			data_sender< test_payload_type >::emit( data_event< test_payload_type >{}, m_payload );
		}

		virtual void
		recv_data( test_payload_type pload ) override
		{
			emit( receipt_event< test_payload_type >{}, pload, std::error_code{} );

			CHECK( m_payload == pload );

			buffer_echoed = true;
		}

		virtual void
		control( control_state state ) override
		{
//			data_receiver::emit( control_event{}, state, std::error_code{} );
		}

		virtual void
		received( test_payload_type pload, std::error_code const& err ) override
		{
			CHECK( pload == m_payload );
			got_receipt = true;
		}

	};

#endif 
}

TEST_CASE( "logicmill/async/stream/smoke/data_sender/data_sink_rvalue" )
{
	using namespace stream_test_1;

	std::error_code err;

	test_payload_type pload = std::make_unique< std::string >( "some buffer content" );

	pump pmp;
	drain drn;

	// connect< test_payload_type, pump, drain >( pmp, drn, err );

	pmp.mate( drn );
//	connect( pmp, drn, err );

	pmp.push( std::move( pload ) );

	CHECK( drn.m_last_value_drained == "some buffer content" );
	CHECK( pmp.got_receipt );

	drn.send< control_event >( control_state::pause  );

	CHECK( pmp.paused );
	CHECK( ! pmp.resumed );

	drn.send< control_event >( control_state::resume  );

	CHECK( ! pmp.paused );
	CHECK( pmp.resumed );
}

#endif