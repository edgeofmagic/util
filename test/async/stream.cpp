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

static void
clear_error( std::error_code& err )
{
	static std::error_code no_error;
	err = no_error;
}

namespace stream_test
{
	class pump : public async::stream::data_source
	{
	public:

		bool paused = false;
		bool resumed = false;
		bool got_receipt = false;

		virtual void
		control( async::stream::control_state state, std::error_code const& err ) override
		{
			if ( state == async::stream::control_state::pause )
			{
				paused = true;
				resumed = false;
			}
			else if ( state == async::stream::control_state::resume )
			{
				resumed = true;
				paused = false;
			}
		}

		virtual void
		received( async::stream::receipt::ptr rp, std::error_code const& err ) override
		{
			got_receipt = true;
		}

	};

	class drain : public async::stream::data_sink
	{
	public:
		const_buffer buff;
		
		virtual void
		recv_data( const_buffer buf, async::stream::receipt::ptr rp, std::error_code const& err ) override
		{
			std::error_code ecode;
			buff = buf;
			emit( async::stream::receipt_event{}, rp, ecode ); 
		}

	};

	class echo : public async::stream::duplex
	{
	public:
		using ptr = std::shared_ptr< echo >;

		async::stream::receipt::ptr my_receipt;

		virtual void
		recv_data( const_buffer buf, async::stream::receipt::ptr rp, std::error_code const& err ) override
		{
			data_sink::emit( async::stream::receipt_event{}, rp, std::error_code{} );

			my_receipt = create_receipt();

			data_source::emit( async::stream::data_event{}, buf, my_receipt, std::error_code{} );
		}

		virtual void
		control( async::stream::control_state state, std::error_code const& err ) override
		{
			data_sink::emit( async::stream::control_event{}, state, std::error_code{} );
		}

		virtual void
		received( async::stream::receipt::ptr rp, std::error_code const& err ) override
		{
			CHECK( rp == my_receipt );
		}

	};

	class echo_driver : public async::stream::duplex
	{
	public:
		using ptr = std::shared_ptr< echo_driver >;

		bool buffer_echoed = false;
		bool got_receipt = false;

		async::stream::receipt::ptr my_receipt = data_source::create_receipt();

		const_buffer b{ mutable_buffer{ "some buffer content" } };

		void send()
		{
			std::error_code err;
			data_source::emit( async::stream::data_event{}, b, my_receipt, err );
		}

		virtual void
		recv_data( const_buffer buf, async::stream::receipt::ptr rp, std::error_code const& err ) override
		{
			data_sink::emit( async::stream::receipt_event{}, rp, std::error_code{} );

			CHECK( buf == b );

			buffer_echoed = true;
		}

		virtual void
		control( async::stream::control_state state, std::error_code const& err ) override
		{
//			data_sink::emit( async::stream::control_event{}, state, std::error_code{} );
		}

		virtual void
		received( async::stream::receipt::ptr rp, std::error_code const& err ) override
		{
			CHECK( rp == my_receipt );
			got_receipt = true;
		}

	};
}

using namespace stream_test;

TEST_CASE( "logicmill/async/stream/smoke/data_source/data_sink" )
{
	std::error_code err;

	const_buffer b{ mutable_buffer{ "some buffer content" } };

	auto pmp = std::make_shared< pump >();
	auto rp = pmp->create_receipt();
	auto dp = std::make_shared< drain >();

	async::stream::connect( pmp, dp, err );

	pmp->emit( async::stream::data_event{}, b, rp, err );

	CHECK( dp->buff == b );
	CHECK( pmp->got_receipt );

	dp->emit( async::stream::control_event{}, async::stream::control_state::pause, err );

	CHECK( pmp->paused );
	CHECK( ! pmp->resumed );

	dp->emit( async::stream::control_event{}, async::stream::control_state::resume, err );

	CHECK( ! pmp->paused );
	CHECK( pmp->resumed );

	pmp->disconnect();
	dp->disconnect();

}

TEST_CASE( "logicmill/async/stream/smoke/echo" )
{
	std::error_code ecode;

	const_buffer b{ mutable_buffer{ "some buffer content" } };

	auto ep = std::make_shared< echo >();
	auto pmp = std::make_shared< pump >();

	auto echo_receipt = pmp->create_receipt();
	bool got_echo_receipt = false;
	bool got_buffer_echo = false;

	auto rl_id = ep->add_listener( async::stream::receipt_event{},
	[ & ] ( async::stream::receipt::ptr rp, std::error_code const& err )
	{
		CHECK( rp == echo_receipt );
		got_echo_receipt = true;
	} );

	auto dl_id = ep->add_listener( async::stream::data_event{}, 
	[ & ] ( const_buffer buf, async::stream::receipt::ptr rp, std::error_code const& err )
	{
		CHECK( buf == b );
		got_buffer_echo = true;
		ep->handle( async::stream::receipt_event{}, rp, std::error_code{} );

	} );

	async::stream::connect( pmp, ep, ecode );
	
	pmp->emit( async::stream::data_event{}, b, echo_receipt, std::error_code{} );

	CHECK( got_echo_receipt );
	CHECK( got_buffer_echo );

	pmp->disconnect();
	ep->disconnect();
}


TEST_CASE( "logicmill/async/stream/smoke/echo_driver" )
{
	std::error_code ecode;

	auto edp = std::make_shared< echo_driver >();
	auto ep = std::make_shared< echo >();

	async::stream::stack( edp, ep, ecode );
	
	CHECK( ! edp->buffer_echoed );
	CHECK( ! edp->got_receipt );

	edp->send();

	CHECK( edp->buffer_echoed );
	CHECK( edp->got_receipt );

	edp->disconnect();
	ep->disconnect();

}