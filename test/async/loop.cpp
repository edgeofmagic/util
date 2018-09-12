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
#include <doctest.h>
#include <iostream>

using namespace logicmill;

class stopwatch
{
public:

	stopwatch()
	:
	m_start{ std::chrono::system_clock::now() }
	{}

	void
	start()
	{
		m_start = std::chrono::system_clock::now();
	}

	template< class T >
	T elapsed() const
	{
		auto current_time = std::chrono::system_clock::now();
		T et = std::chrono::duration_cast< T >( current_time - m_start );
		return et;
	}
	
private:
	std::chrono::time_point< std::chrono::system_clock >		m_start;
};

TEST_CASE( "logicmill/async/loop/smoke/basic" )
{
	async::loop::ptr lp = async::loop::create();

	lp->run();

	CHECK( true );
}

TEST_CASE( "logicmill/async/loop/smoke/timer" )
{
	async::loop::ptr lp = async::loop::create();

	{
		auto tp = lp->create_timer( [] ( async::timer::ptr timer_ptr )
		{
			CHECK( ! timer_ptr->is_pending() );
			std::cout << "timer expired" << std::endl;
		});

		tp->start( std::chrono::milliseconds{ 1000 } );

		CHECK( tp->is_pending() );
	}

	lp->run();

	lp->close();

	CHECK( true );
}

TEST_CASE( "logicmill/async/loop/smoke/timer_close_before_expire" )
{
	async::loop::ptr lp = async::loop::create();

	auto tp0 = lp->create_timer( [] ( async::timer::ptr timer_ptr )
	{
		std::cout << "timer 0 expired" << std::endl;
	});

	auto tp1 = lp->create_timer( [] ( async::timer::ptr timer_ptr )
	{
		std::cout << "timer 1 expired" << std::endl;
		auto my_loop = timer_ptr->owner();
		my_loop->stop();
	});

	tp0->start( std::chrono::milliseconds{ 2000 } );
	tp1->start( std::chrono::milliseconds{ 1000 } );

	lp->run();

	std::cout << "loop run completed" << std::endl;

	lp->close();

	CHECK( true );
}

TEST_CASE( "logicmill/async/loop/smoke/timer_stop_before_expire" )
{
	async::loop::ptr lp = async::loop::create();

	auto tp0 = lp->create_timer( [] ( async::timer::ptr timer_ptr )
	{
		std::cout << "timer 0 expired" << std::endl;
	});

	auto tp1 = lp->create_timer( [=] ( async::timer::ptr timer_ptr )
	{
		std::cout << "timer 1 expired" << std::endl;
		tp0->stop();
	});

	tp0->start( std::chrono::milliseconds{ 2000 } );
	tp1->start( std::chrono::milliseconds{ 1000 } );

	stopwatch sw;

	lp->run();

	auto ms = sw.elapsed< std::chrono::milliseconds >();

	std::cout << "loop run completed: " << ms.count() << " ms" << std::endl;

	lp->close();

	CHECK( true );
}

TEST_CASE( "logicmill/async/resolver/smoke/basic" )
{
	std::error_code err;

	async::loop::ptr lp = async::loop::create();
	auto res = lp->create_resolver( "google.com", err, [] ( async::resolver::ptr req, std::deque< async::ip::address >&& addresses, std::error_code const& err )
	{
		std::cout << "resolver handler called for hostname " << req->hostname() << std::endl;
		std::cout << "status is " << err.message() << std::endl;
		if ( ! err )
		{
			for ( auto& addr : addresses )
			{
				std::cout << addr.to_string() << std::endl;
			}
		}
		std::cout.flush();
	});

	if ( err )
	{
		std::cout << "create_resolver failed, err: " << err.message() << std::endl;
	}
	else
	{
		lp->run();
		std::cout << "loop run completed" << std::endl;
	}

	lp->close();
}

TEST_CASE( "logicmill/async/resolver/smoke/not_found_failure" )
{
	std::error_code err;

	async::loop::ptr lp = async::loop::create();
	auto res = lp->create_resolver( "no_such_host.com", err, 
	[] ( async::resolver::ptr req, std::deque< async::ip::address >&& addresses, std::error_code const& err )
	{
		std::cout << "resolver handler called for hostname " << req->hostname() << std::endl;
		std::cout << "status is " << err.message() << std::endl;
		if ( ! err )
		{
			for ( auto& addr : addresses )
			{
				std::cout << addr.to_string() << std::endl;
			}
		}
		std::cout.flush();
	});

	if ( err )
	{
		std::cout << "create_resolver failed, err: " << err.message() << std::endl;
	}
	else
	{
		lp->run();
		std::cout << "loop run completed" << std::endl;
	}

	lp->close();
}

#if 1
TEST_CASE( "logicmill/async/resolver/smoke/cancellation" )
{
	{
		std::error_code err;

		async::loop::ptr lp = async::loop::create();
		auto res = lp->create_resolver( "not_a_host_in_any_event.com", err, 
		[] ( async::resolver::ptr req, std::deque< async::ip::address >&& addresses, std::error_code const& err )
		{
			std::cout << "resolver handler called for hostname " << req->hostname() << std::endl;
			std::cout << "status is " << err.message() << std::endl;
			if ( ! err )
			{
				for ( auto& addr : addresses )
				{
					std::cout << addr.to_string() << std::endl;
				}
			}
			std::cout.flush();
		});

		async::timer::ptr tp = lp->create_timer( err, [=] ( async::timer::ptr timer_ptr ) // mutable
		{
			res->cancel();
			std::cout << "cancelled resolve request" << std::endl;
		});

		REQUIRE( ! err );

		tp->start( std::chrono::milliseconds{ 0 }, err );

		REQUIRE( ! err );

		lp->run();
		std::cout << "loop run completed" << std::endl;

		std::cout << "outstanding loop refcount after run: " << lp.use_count() << std::endl;

		lp->close();

	}
}
#endif