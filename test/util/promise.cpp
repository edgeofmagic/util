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

#define TEST_ASYNC 1

#include <doctest.h>
#include <iostream>
#include <util/promise.h>
#include <unordered_map>

#if (TEST_ASYNC)
	#include <util/promise_timer.h>
	#include "asio_adapter.h"
	using async_io = asio_adapter::async_adapter;

	#define STOP_LOOP(_loop_, _millisecs_)                                                                                 \
		async_io::schedule(_loop_, std::chrono::milliseconds{_millisecs_}, [=](std::error_code const& err) mutable {       \
			async_io::stop_loop(_loop_);                                                                                   \
		});
#endif

	using namespace util;

	static int
	func()
	{
		return 42;
}

#if (TEST_ASYNC)

static promise< void >
make_v( async_io::loop_param_type lp, std::chrono::milliseconds delay )
{
	auto p = promise< void >();

	async_io::schedule(lp, delay, [=](std::error_code const& err) mutable
	{
		p.resolve();
	} );

	return p;
}

static promise< int >
make_p( int i, async_io::loop_param_type lp, std::chrono::milliseconds delay )
{
	auto p = promise< int >();

	async_io::schedule(lp, delay, [=](std::error_code const& err) mutable
	{
		p.resolve( std::move( i ) );
	});

	return p;
}


static promise< int >
make_e( int err_value, async_io::loop_param_type lp, std::chrono::milliseconds delay )
{
	auto p = promise< int >();

	async_io::schedule(lp, delay, [=](std::error_code const& err) mutable
	{
		p.reject( std::error_code( err_value, std::generic_category() ) );
	});

	return p;
}

#endif

TEST_CASE( "nodeoze/smoke/promise" )
{
	SUBCASE( "synchronous chaining" )
	{
		auto done = std::make_shared< bool >( false );
		promise< void > p;
		
		p.then( [=]()
		{
			return 7;
		} )
		.then( [=]( int i )
		{
			CHECK( i == 7 );
			return "blob";
		} )
		.then( [=]( const std::string &s )
		{
			CHECK( s == "blob" );
			return 7.5;
		} )
		.then( [=]( double f )
		{
			CHECK( f == 7.5 );

			promise< bool > p1;

			p1.resolve( true );

			return p1;
		} )
		.then( [=]( bool b )
		{
			CHECK( b == true );
			return func();
		} )
		.then( [=]( int i )
		{
			CHECK( i == 42 );
		},
		[=]( std::error_code const& /* unused */ )
		{
			assert( 0 );
		} )
		.finally( [=]() mutable
		{
			*done = true;
		} );

		CHECK( !*done );

		p.resolve();

		CHECK( *done );
	}
	
	SUBCASE( "synchronous chaining with error hander" )
	{
		auto done = std::make_shared< bool >( false );
		promise< void > p;
		
		p.then( [=]()
		{
			return 7;
		} )
		.then( [=]( int i )
		{
			CHECK( i == 7 );

			promise< std::string > p1;

			p1.reject( std::error_code( 42, std::generic_category() ) );

			return p1;
		} )
		.then( [=]( const std::string & /* unused */ )
		{
			assert( 0 );
			return 7.5;
		} )
		.then( [=]( double /* unused */ )
		{
			assert( 0 );
			promise< bool > p1;

			p1.resolve( true );

			return p1;
		} )
		.then( [=]( bool /* unused */ )
		{
			assert( 0 );
			return func();
		} )
		.then( [=]( int /* unused */ )
		{
			assert( 0 );
		},
		[=]( std::error_code const& err )
		{
			CHECK( err.value() == 42 );
			CHECK( err.category() == std::generic_category() );
		} )
		.finally( [=]() mutable
		{
			*done = true;
		} );

		CHECK( !*done );

		p.resolve();

		CHECK( *done );
	}

	SUBCASE( "synchronous with build" )
	{
		auto done = std::make_shared< bool >( false );
		promise< void > p;
		
		p.then( [=]()
		{
			return promise<int>::build(7);
		} )
		.then( [=]( int i )
		{
			CHECK( i == 7 );
			*done = true;
		} );

		CHECK( !*done );

		p.resolve();

		CHECK( *done );
	}

#if (TEST_ASYNC)

	SUBCASE( "asynchronous chaining" )
	{
		auto lp = async_io::create_loop();

		auto count = std::make_shared< std::uint8_t >( 0 );
		promise< int > p;

		p.then( [=]( int i )
		{
			CHECK( i == 7 );
			return "blob";
		} )
		.then( [=]( const std::string &s )
		{
			CHECK( s == "blob" );
			return 7.5;
		} )
		.then( [=]( double f )
		{
			CHECK( f == 7.5 );

			promise< bool > p1;

			std::error_code err;

			async_io::dispatch(lp, [=]() mutable
			{
				p1.resolve( true );
			});
			return p1;
		} )
		.then( [=]( bool b )
		{
			CHECK( b == true );
			return func();
		} )
		.then( [=]( int i ) mutable
		{
			CHECK( i == 42 );
			( *count )++;
		},
		[=]( std::error_code const& )
		{
			assert( 0 );
		} )
		.finally( [=]() mutable
		{
			( *count )++;
		} );

		std::error_code err;

		async_io::dispatch(lp, [=]() mutable
		{
			p.resolve( 7 );
		});

		STOP_LOOP(lp, 100);
		async_io::run_loop(lp);

		CHECK( *count == 2 );
	}

	SUBCASE( "asynchronous chaining with error hander" )
	{
		auto lp = async_io::create_loop();

		auto count = std::make_shared< std::uint8_t >( 0 );
		promise< int > p;

		p.then( [=]( int i )
		{
			CHECK( i == 7 );

			promise< std::string > p1;

			async_io::dispatch(lp, [=]() mutable
			{
				p1.reject( std::error_code( 42, std::generic_category() ) );
			});
			return p1;
		} )
		.then( [=]( const std::string & /* unused */ )
		{
			assert( 0 );
			return 7.5;
		} )
		.then( [=]( double /* unused */ )
		{
			assert( 0 );
			promise< bool > p1;

			p1.resolve( true );

			return p1;
		} )
		.then( [=]( bool /* unused */ )
		{
			assert( 0 );

			return func();
		} )
		.then( [=]( int /* unused */ )
		{
			assert( 0 );
		} )
		.catcher( [=]( auto err ) mutable
		{
			CHECK( err.value() == 42 );
			CHECK( err.category() == std::generic_category() );
			( *count )++;
		} )
		.finally( [=]() mutable
		{
			( *count )++;
		} );

		async_io::dispatch(lp, [=]() mutable
		{
			p.resolve( 7 );
		});

		STOP_LOOP(lp, 100);
		async_io::run_loop(lp);

		CHECK( *count == 2 );
	}
	
	SUBCASE( "synchronous all <void>" )
	{
		auto count = std::make_shared< std::uint8_t >( 0 );

		auto lp = async_io::create_loop();

		promise< void >::all(
		{
			make_v( lp, std::chrono::milliseconds( 300 ) ),
			make_v( lp, std::chrono::milliseconds( 200 ) ),
			make_v( lp, std::chrono::milliseconds( 100 ) )
		} ).then( [=]() mutable
		{
			( *count )++;
		},
		[=]( std::error_code const&  ) mutable
		{
			assert( 0 );
		} )
		.finally( [=]() mutable
		{
			( *count )++;
		} );

		STOP_LOOP(lp, 400);

		async_io::run_loop(lp);

		CHECK( *count == 2 );
	}

	SUBCASE( "synchronous all <int>" )
	{
		auto count = std::make_shared< std::uint8_t >( 0 );

		auto lp = async_io::create_loop();

		promise< int >::all(
		{
			make_p( 10, lp, std::chrono::milliseconds( 300 ) ),
			make_p( 20, lp, std::chrono::milliseconds( 200 ) ),
			make_p( 30, lp, std::chrono::milliseconds( 100 ) )
		} ).then( [=]( auto results ) mutable
		{
			CHECK( results.size() == 3 );
			CHECK( results[ 0 ] == 10 );
			CHECK( results[ 1 ] == 20 );
			CHECK( results[ 2 ] == 30 );

			( *count )++;
		},
		[=]( std::error_code const&  ) mutable
		{
			assert( 0 );
		} )
		.finally( [=]() mutable
		{
			( *count )++;
		} );

		STOP_LOOP(lp, 400);
		async_io::run_loop(lp);

		CHECK( *count == 2 );
	}

	SUBCASE( "any with no errors" )
	{
		auto count = std::make_shared< std::uint8_t >( 0 );

		auto lp = async_io::create_loop();

		promise< int >::any(
		{
			make_p( 10, lp, std::chrono::milliseconds( 300 ) ),
			make_p( 20, lp, std::chrono::milliseconds( 100 ) ),
			make_p( 30, lp, std::chrono::milliseconds( 200 ) )
		} )
		.then( [=]( int val ) mutable
		{
			CHECK( val == 20 );
			( *count )++;
		} )
		.catcher( [=]( auto /* unused */ ) mutable
		{
			assert( 0 );
		} )
		.finally( [=]() mutable
		{
			( *count )++;
		} );

		STOP_LOOP(lp, 400);
		async_io::run_loop(lp);

		CHECK( *count == 2 );
	}

	SUBCASE( "any with all errors" )
	{
		auto done = std::make_shared< bool >( false );

		auto lp = async_io::create_loop();

		promise< int >::any(
		{
			make_e( 70, lp, std::chrono::milliseconds( 300 ) ),
			make_e( 71, lp, std::chrono::milliseconds( 200 ) ),
			make_e( 72, lp, std::chrono::milliseconds( 100 ) )
		} ).then( [=]( int /* unused */ ) mutable
		{
			assert( 0 );
		},
		[=]( std::error_code const& err ) mutable
		{
			CHECK( err.value() == 70 );
			*done = true;
		} );

		STOP_LOOP(lp, 400);
		async_io::run_loop(lp);

	}

	SUBCASE( "any with errors" )
	{
		auto done = std::make_shared< bool >( false );

		auto lp = async_io::create_loop();

		promise< int >::any(
		{
			make_e( 72, lp, std::chrono::milliseconds( 100 ) ), // TODO: should the delay be 300 ms?
			make_p( 20, lp, std::chrono::milliseconds( 200 ) ),
			make_e( 72, lp, std::chrono::milliseconds( 100 ) )
		} ).then( [=]( int val ) mutable
		{
			CHECK( val == 20 );
			*done = true;
		},
		[=]( std::error_code const& /* unused */ ) mutable
		{
			assert( 0 );
		} );

		STOP_LOOP(lp, 400);
		async_io::run_loop(lp);
	}
	
	SUBCASE( "race with no errors< void >" )
	{
		auto done = std::make_shared< bool >( false );

		auto lp = async_io::create_loop();

		promise< void >::race(
		{
			make_v( lp, std::chrono::milliseconds( 300 ) ),
			make_v( lp, std::chrono::milliseconds( 100 ) ),
			make_v( lp, std::chrono::milliseconds( 200 ) )
		} ).then( [=]() mutable
		{
			*done = true;
		},
		[=]( std::error_code const& /* unused */ ) mutable
		{
			assert( 0 );
		} );

		STOP_LOOP(lp, 400);
		async_io::run_loop(lp);
	}

	SUBCASE( "race with no errors< int >" )
	{
		auto done = std::make_shared< bool >( false );

		auto lp = async_io::create_loop();

		promise< int >::race(
		{
			make_p( 10, lp, std::chrono::milliseconds( 300 ) ),
			make_p( 20, lp, std::chrono::milliseconds( 100 ) ),
			make_p( 30, lp, std::chrono::milliseconds( 200 ) )
		} ).then( [=]( int val ) mutable
		{
			CHECK( val == 20 );
			*done = true;
		},
		[=]( std::error_code const& /* unused */ ) mutable
		{
			assert( 0 );
		} );

		STOP_LOOP(lp, 400);
		async_io::run_loop(lp);
	}

	SUBCASE( "race with errors" )
	{
		auto done = std::make_shared< bool >( false );

		auto lp = async_io::create_loop();

		promise< int >::race(
		{
			make_e( 72, lp, std::chrono::milliseconds( 300 ) ),
			make_p( 20, lp, std::chrono::milliseconds( 200 ) ),
			make_e( 75, lp, std::chrono::milliseconds( 100 ) )
		} ).then( [=]( int /* unused */ ) mutable
		{
			assert( 0 );
		},
		[=]( std::error_code const& err) mutable
		{
			CHECK( err.value() == 75 );
			*done = true;
		} );

		STOP_LOOP(lp, 400);
		async_io::run_loop(lp);
	}

#endif

	SUBCASE( "all with no promises" )
	{
		promise< int >::all( {} )
		.then( [=]( auto val ) mutable
		{
			CHECK( val.size() == 0 );
		},
		[=]( std::error_code const& err ) mutable
		{
			CHECK( err.value() == static_cast< int >( std::errc::invalid_argument ) );
		} );
	}
	
	SUBCASE( "any void with first succeed" )
	{
		auto p1 = promise< void >();
		auto p2 = promise< void >();
		auto p3 = promise< void >();

		p1.resolve();
		p2.reject( make_error_code( std::errc::not_connected ) );
		p3.reject( make_error_code( std::errc::not_connected ) );

		auto good = false;

		promise< void >::any( { p1, p2, p3 } )
		.then( [&]() mutable
		{
			good = true;
		},
		[&]( auto /* unused */ ) mutable
		{
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any int with first succeed" )
	{
		auto p1 = promise< int >();
		auto p2 = promise< int >();
		auto p3 = promise< int >();

		p1.resolve( 7 );
		p2.reject( make_error_code( std::errc::not_connected ) );
		p3.reject( make_error_code( std::errc::not_connected ) );

		auto good = false;

		promise< int >::any( { p1, p2, p3 } )
		.then( [&]( int val ) mutable
		{
			CHECK( val == 7 );
			good = true;
		},
		[&]( auto /* unused */ ) mutable
		{
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any void with second succeed" )
	{
		auto p1 = promise< void >();
		auto p2 = promise< void >();
		auto p3 = promise< void >();

		p1.reject( make_error_code( std::errc::not_connected ) );
		p2.resolve();
		p3.reject( make_error_code( std::errc::not_connected ) );

		auto good = false;

		promise< void >::any( { p1, p2, p3 } )
		.then( [&]() mutable
		{
			good = true;
		},
		[&]( auto  /* unused */ ) mutable
		{
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any int with second succeed" )
	{
		auto p1 = promise< int >();
		auto p2 = promise< int >();
		auto p3 = promise< int >();

		p1.reject( make_error_code( std::errc::not_connected ) );
		p2.resolve( 8 );
		p3.reject( make_error_code( std::errc::not_connected ) );

		auto good = false;

		promise< int >::any( { p1, p2, p3 } )
		.then( [&]( int val ) mutable
		{
			CHECK( val == 8 );
			good = true;
		},
		[&]( auto /* unused */ ) mutable
		{
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any void with third succeed" )
	{
		auto p1 = promise< void >();
		auto p2 = promise< void >();
		auto p3 = promise< void >();

		p1.reject( make_error_code( std::errc::not_connected ) );
		p2.reject( make_error_code( std::errc::not_connected ) );
		p3.resolve();

		auto good = false;

		promise< void >::any( { p1, p2, p3 } )
		.then( [&]() mutable
		{
			good = true;
		},
		[&]( auto /* unused */ ) mutable
		{
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any int with third succeed" )
	{
		auto p1 = promise< int >();
		auto p2 = promise< int >();
		auto p3 = promise< int >();

		p1.reject( make_error_code( std::errc::not_connected ) );
		p2.reject( make_error_code( std::errc::not_connected ) );
		p3.resolve( 9 );

		auto good = false;

		promise< int >::any( { p1, p2, p3 } )
		.then( [&]( int val ) mutable
		{
			CHECK( val == 9 );
			good = true;
		},
		[&]( auto /* unused */ ) mutable
		{
			good = false;
		} );

		CHECK( good );
	}

	SUBCASE( "any void with all fail" )
	{
		auto p1 = promise< void >();
		auto p2 = promise< void >();
		auto p3 = promise< void >();

		p1.reject( make_error_code( std::errc::not_connected ) );
		p2.reject( make_error_code( std::errc::not_connected ) );
		p3.reject( make_error_code( std::errc::not_connected ) );

		auto good = false;

		promise< void >::any( { p1, p2, p3 } )
		.then( [&]() mutable
		{
			CHECK( false );
		},
		[&]( auto /* unused */ ) mutable
		{
			good = true;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any int with all fail" )
	{
		auto p1 = promise< int >();
		auto p2 = promise< int >();
		auto p3 = promise< int >();

		p1.reject( make_error_code( std::errc::not_connected ) );
		p2.reject( make_error_code( std::errc::not_connected ) );
		p3.reject( make_error_code( std::errc::not_connected ) );

		auto good = false;

		promise< int >::any( { p1, p2, p3 } )
		.then( [&]( int /* unused */ ) mutable
		{
			CHECK( false );
		},
		[&]( auto /* unused */ ) mutable
		{
			good = true;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any with no promises" )
	{
		promise< int >::any( {} ).then( [=]( int /* unused */ ) mutable
		{
			assert( 0 );
		},
		[=]( std::error_code const& err ) mutable
		{
			CHECK( err.value() == static_cast< int >( std::errc::invalid_argument ) );
		} );
	}
	
	SUBCASE( "race with no promises" )
	{
		promise< int >::race( {} ).then( [=]( int /* unused */ ) mutable
		{
			assert( 0 );
		},
		[=]( std::error_code const& err ) mutable
		{
			CHECK( err.value() == static_cast< int >( std::errc::invalid_argument ) );
		} );
	}
	
	SUBCASE( "simple void" )
	{
		auto string = std::string( "blob" );
		promise< void > p;

		p.then( [=]()
		{
			return string;
		} )
		.then( [=]( const std::string &s )
		{
			CHECK( s == string );
			
			promise< void > p1;

			p1.resolve();

			return p1;
		} )
		.then( [=]()
		{
			promise< void > p1;

			p1.resolve();

			return p1;
		} )
		.then( [=]()
		{
		},
		[=]( std::error_code const&/* unused */ )
		{
			assert( 0 );
		} );

		p.resolve();
	}
	
	SUBCASE( "complex void" )
	{
		std::vector< std::string > strings = { "a", "b", "c" };
		
		promise< void > p;
		
		auto func1 = []()
		{
			auto ret = promise< void >();
			
			ret.resolve();
			
			return ret;
		};
		
		auto func2 = []()
		{
			auto strings	= std::vector< std::string >( { "a", "b", "c" } );
			auto ret		= promise< std::vector< std::string > >();
			
			ret.resolve( strings );
			
			return ret;
		};
			

		func1()
		.then( [=]()
		{
			return func2();
		} )
		.then( [=]( std::vector< std::string > incoming )
		{
			CHECK( incoming == strings );
			
			promise< void > p1;

			p1.resolve();

			return p1;
		} )
		.then( [=]()
		{
			promise< void > p1;

			p1.resolve();

			return p1;
		} )
		.then( [=]()
		{
		},
		[=]( std::error_code const&/* unused */ )
		{
			assert( 0 );
		} );

		p.resolve();
	}

#if (TEST_ASYNC)

	SUBCASE( "leaks" )
	{
		auto clean = std::make_shared< bool >( false );

		auto lp = async_io::create_loop();

		struct leaker
		{
			std::shared_ptr< bool > m_flag;
			
			leaker( std::shared_ptr< bool > flag )
			:
				m_flag( flag )
			{
			}
			
			~leaker()
			{
				*m_flag = true;
			}
			
			void
			func()
			{
			}
		};
		
		{
			auto ptr = std::make_shared< leaker >( clean );
			
			auto func1 = [lp]() mutable
			{
				auto ret = promise< void >();
				
				async_io::schedule(lp, std::chrono::milliseconds{10}, [=] (std::error_code const& err) mutable
				{
					ret.resolve();
				});
				return ret;
			};
			
			auto func2 = [lp]() mutable
			{
				auto ret = promise< void >();
				
				async_io::schedule(lp, std::chrono::milliseconds{10}, [=] (std::error_code const& err) mutable
				{
					ret.resolve();
				});
				return ret;
			};
			
			auto func3 = [lp]() mutable
			{
				auto ret = promise< void >();

				async_io::schedule(lp, std::chrono::milliseconds{10}, [=] (std::error_code const& err) mutable
				{
					ret.resolve();
				});
				return ret;
			};

			auto ret = promise< std::shared_ptr< leaker > >();
			
			REQUIRE( *clean == false );
			
			func1()
			.then( [=]() mutable
			{
				return func2();
			} )
			.then( [=]() mutable
			{
				return func3();
			} )
			.then( [=]() mutable
			{
				ret.resolve( ptr );
			},
			[=]( auto /* unused */ ) mutable
			{
			} );
			
			REQUIRE( !ret.is_finished() );
			REQUIRE( *clean == false );
			
			STOP_LOOP(lp, 100);
			async_io::run_loop(lp);
		}
		
		REQUIRE( *clean == true );
	}

	SUBCASE( "timeout" )
	{

		auto lp = async_io::create_loop();

		auto err	= std::error_code();
		auto func	= [lp]()
		{
			auto ret = promise< void >();


			async_io::schedule(lp, std::chrono::milliseconds{20}, [=] (std::error_code const& err) mutable
			{
				ret.resolve();
			});

			return ret;
		};

		auto done = false;

		func()
		.timeout( util::promise_timer<async_io>{std::chrono::milliseconds( 10 ), lp} )
		.then( [&]() mutable
		{
			done = true;
		},
		[&]( auto _err ) mutable
		{
			done = true;
			err = _err;
		} );

			STOP_LOOP(lp, 100);
			async_io::run_loop(lp);

		CHECK( err == std::errc::timed_out );
	}

#endif

}

