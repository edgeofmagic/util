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

#include <logicmill/async/loop.h>
#include <logicmill/bstream/buffer.h>
#include <doctest.h>
#include <iostream>

using namespace logicmill;
using namespace async;
using namespace tcp;

TEST_CASE( "logicmill::async::tcp::listener [ smoke ] { basic functionality }" )
{
	std::error_code err;
	auto lp = loop::create();
	async::ip::endpoint listen_ep{ async::ip::address::v4_any(), 7001 };
	auto lstnr = lp->create_tcp_listener( listen_ep, err, [&] ( listener::ptr const& ls, channel::ptr const& chan, std::error_code const& ec )
	{
		if ( ec )
		{
			std::cout << "error in listener connection handler: " << ec.message() << std::endl;
		}
		else
		{
			std::cout << "listener connection handler received connection" << std::endl;
		}
		CHECK( ! ec );
		chan->close();
		ls->close();

	} );

	CHECK( ! err );

	auto connect_timer = lp->create_timer( err, [&] ( async::timer::ptr timer_ptr )
	{

		std::error_code ec;
		async::ip::endpoint connect_ep{ async::ip::address::v4_loopback(), 7001 };

		std::cout << "in timer handler, connecting to listener" << std::endl;

		lp->connect_tcp_channel( connect_ep, ec, [&] ( channel::ptr const& chan, std::error_code const& erc )
		{
			if ( erc )
			{
				std::cout << "error in channel connect handler: " << erc.message() << std::endl;
			}
			else
			{
				std::cout << "channel connect handler connection succeeded" << std::endl;
			}
			chan->close();
		} );

		CHECK( ! ec );
	});

	CHECK( ! err );

	connect_timer->start( std::chrono::milliseconds{ 1000 }, err );

	CHECK( ! err );

	CHECK( connect_timer->is_pending() );

	connect_timer.reset();

	lp->run( err );

	CHECK( ! err );

	lp->close( err );

	CHECK( ! err );

}

TEST_CASE( "logicmill::async::tcp::listener [ smoke ] { error on bad address }" )
{
	std::error_code err;
	auto lp = loop::create();
	async::ip::endpoint listen_ep{ async::ip::address{ "11.42.53.5" }, 7001 };
	auto lstnr = lp->create_tcp_listener( listen_ep, err, [&] ( listener::ptr const& ls, channel::ptr const& chan, std::error_code const& ec )
	{
		if ( ec )
		{
			std::cout << "error in listener connection handler: " << ec.message() << std::endl;
		}
		else
		{
			std::cout << "listener connection handler received connection" << std::endl;
		}
		CHECK( ! ec );
		chan->close();
		ls->close();

	} );

	REQUIRE( err );
	REQUIRE( err == std::errc::address_not_available );

	std::cout << "create listener failed as expected" << std::endl;

	if ( err )
	{
		lstnr->close();
	}

	std::cout << "listener closed" << std::endl;

	auto connect_timer = lp->create_timer( err, [&] ( async::timer::ptr timer_ptr )
	{

		std::error_code ec;
		async::ip::endpoint connect_ep{ async::ip::address::v4_loopback(), 7001 };

		std::cout << "in timer handler, connecting to listener" << std::endl;

		lp->connect_tcp_channel( connect_ep, ec, [&] ( channel::ptr const& chan, std::error_code const& erc )
		{
			if ( ! chan )
			{
				std::cout << "in channel connect handler, channel pointer is null" << std::endl;
			}
			else
			{
				std::cout << "in channel connect handler, channel pointer is non-null" << std::endl;
			}

			if ( erc )
			{
				std::cout << "error in channel connect handler: " << erc.message() << std::endl;
			}
			else
			{
				std::cout << "channel connect handler connection succeeded" << std::endl;
			}
			chan->close();
		} );

		CHECK( ! ec );
	});

	CHECK( ! err );

	connect_timer->start( std::chrono::milliseconds{ 1000 }, err );

	CHECK( ! err );

	CHECK( connect_timer->is_pending() );

	connect_timer.reset();

	lp->run( err );

	CHECK( ! err );

	lp->close( err );

	CHECK( ! err );

}

TEST_CASE( "logicmill::async::tcp::listener [ smoke ] { connect read write }" )
{
	std::error_code err;
	auto lp = loop::create();
	async::ip::endpoint listen_ep{ async::ip::address::v4_any(), 7001 };
	auto lstnr = lp->create_tcp_listener( listen_ep, err, [&] ( listener::ptr const& ls, channel::ptr const& chan, std::error_code const& ec )
	{
		if ( ec )
		{
			std::cout << "error in listener connection handler: " << ec.message() << std::endl;
		}
		else
		{
			std::cout << "listener connection handler received connection" << std::endl;
		}
		CHECK( ! ec );
		chan->start_read( [&] ( channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code const& err )
		{
			CHECK( ! err );
			auto sv = buf.as_string();
			CHECK( sv == "first test payload" );
			std::cout << "received payload in read handler" << std::endl;

			bstream::mutable_buffer reply{ "reply to first payload" };
			cp->write( bstream::mutable_buffer{ "reply to first payload" }, 
			[&] ( channel::ptr const& chan, write_request::ptr const& req, std::error_code const& err )
			{
				CHECK( ! err );
				std::cout << "wrote reply, written buffer contains '" << req->buffers()[0].as_string() << "'" << std::endl;
			} );

		} );
		// chan->close();
		// ls->close();

	} );

	CHECK( ! err );

	auto connect_timer = lp->create_timer( err, [&] ( async::timer::ptr timer_ptr )
	{

		std::error_code ec;
		async::ip::endpoint connect_ep{ async::ip::address::v4_loopback(), 7001 };

		std::cout << "in timer handler, connecting to listener" << std::endl;

		lp->connect_tcp_channel( connect_ep, ec, [&] ( channel::ptr const& chan, std::error_code const& erc )
		{
			if ( erc )
			{
				std::cout << "error in channel connect handler: " << erc.message() << std::endl;
			}
			else
			{
				std::cout << "channel connect handler connection succeeded" << std::endl;

				chan->start_read( [&] ( channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code const& err )
				{
					CHECK( ! err );
					auto sv = buf.as_string();
					CHECK( sv == "reply to first payload" );
					std::cout << "received reply in client read handler" << std::endl;
				} );

				std::string contents{ "first test payload" };
				bstream::mutable_buffer mbuf{ contents.size() };
				mbuf.putn( 0, contents.data(), contents.size() );
				mbuf.size( contents.size() );
				CHECK(mbuf.size() == contents.size() );
				chan->write( std::move( mbuf ), [&] ( channel::ptr const& chan, write_request::ptr const& req, std::error_code const& err )
				{
					CHECK( ! err );
					std::cout << "write handler called, written buffer contains '" << req->buffers()[0].as_string() << "'" << std::endl;
				} );
			}
		} );

		CHECK( ! ec );
	});

	CHECK( ! err );

	auto shutdown_timer = lp->create_timer( err, [&] ( async::timer::ptr timer_ptr )
	{
		std::error_code ec;
		lp->stop( ec );
		CHECK( ! ec );
	} );

	CHECK( ! err );

	connect_timer->start( std::chrono::milliseconds{ 1000 }, err );

	CHECK( ! err );

	shutdown_timer->start( std::chrono::milliseconds{ 3000 }, err );

	CHECK( ! err );

	CHECK( connect_timer->is_pending() );

	connect_timer.reset();

	lp->run( err );

	CHECK( ! err );

	lp->close( err );

	CHECK( ! err );

}
