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

#include <logicmill/async/event.h>
#include <doctest.h>
#include <iostream>

using namespace logicmill;

namespace event_test
{
	enum class actions
	{
		start,
		stop,
		kill
	};

	using kill_event = async::event< actions, actions::kill, std::string const& >;
	using start_event = async::event< actions, actions::start, int >;
	using stop_event = async::event< actions, actions::stop, bool >;

	class kill_emitter : public async::emitter< kill_event, async::cardinality::simplex >
	{};

	class kill_handler : public async::handler_spec< kill_event, kill_handler >
	{
	public:
		using base = async::handler_spec< kill_event, kill_handler >;
		using ptr = std::shared_ptr< kill_handler >;

		kill_handler()
		: 
		base{  &kill_handler::murder }
		{}

		void murder( std::string const& message )
		{
			std::cout << "redrum: " << message << std::endl;
			kill_received = true;
		}

		bool kill_received = false;
	};	

	struct kill_functor
	{

		kill_functor( bool& f ) : flag{ f } { flag = false; }
		bool& flag;
		void operator()( std::string const& s )
		{
			std::cout << "kill_functor: " << s << std::endl;
			flag = true;
		}
	};
}

using namespace event_test;

TEST_CASE( "logicmill/async/event/smoke/listener/lambda" )
{
	bool kill_received = false;

	kill_event::listener kill_listener = [&] ( std::string const& message )
	{
		std::cout << "kill_listener: " << message << std::endl;
		kill_received = true;
	};

	kill_listener( std::string{ "weasels ripped my flesh" } );

	CHECK( kill_received );
}

TEST_CASE( "logicmill/async/event/smoke/listener/functor/move" )
{
	bool kill_received = false;

	kill_functor kf{ kill_received };

	CHECK( ! kill_received );

	kill_emitter ke;

	auto id = ke.add_listener( kill_event{}, std::move( kf ) );
	
	ke.emit( kill_event{}, "don't you eat that yellow snow" );

	CHECK( kill_received );
}

TEST_CASE( "logicmill/async/event/smoke/listener/functor/copy" )
{
	bool kill_received = false;

	kill_functor kf{ kill_received };

	CHECK( ! kill_received );

	kill_emitter ke;

	auto id = ke.add_listener( kill_event{}, kf );
	
	ke.emit( kill_event{}, "here it goes, the circular motion--rub it!" );

	CHECK( kill_received );
}

TEST_CASE( "logicmill/async/event/smoke/handler" )
{
	kill_handler k;

	CHECK( ! k.kill_received );

	k.handle( kill_event{}, std::string{ "zoot allures" } );

	CHECK( k.kill_received );
}

TEST_CASE( "logicmill/async/event/smoke/emitter/listener" )
{
	bool kill_received = false;

	kill_emitter ke;

	auto id = ke.add_listener( kill_event{}, [&] ( std::string const& message )
	{
		std::cout << "kill_listener: " << message << std::endl;
		kill_received = true;
	} );
	
	ke.emit( kill_event{}, "shut up and play yer guitar");

	CHECK( kill_received );
}

TEST_CASE( "logicmill/async/event/smoke/emitter/handler" )
{
	kill_emitter ke;
	kill_handler::ptr kp = std::make_shared< kill_handler >();
	CHECK( ! kp->kill_received );

	auto id = ke.add_listener( kill_event{}, kp );
	
	ke.emit( kill_event{}, "watch out where the huskies go");

	CHECK( kp->kill_received );
}


#if 0
	class killer : public kill_event::handler_spec< killer >
	{
	public:
		using base = kill_event::handler_spec< killer >;

		killer()
		: 
		base{  &killer::murder }
		{}

		void murder( std::string const& message )
		{
			std::cout << "redrum: " << message << std::endl;
			kill_received = true;
		}

		bool kill_received = false;
	};

	class kill_emitter : public kill_event::emitter
	{
	};

	class action_handler : 
		public kill_event::handler_spec< action_handler >,
		public start_event::handler_spec< action_handler >,
		public stop_event::handler_spec< action_handler >
	{
	public:
		action_handler() 
		: 
		kill_event::handler_spec< action_handler >{ &action_handler::kill },
		start_event::handler_spec< action_handler >{ &action_handler::start },
		stop_event::handler_spec< action_handler >{ &action_handler::stop }
		{}

		void kill( std::string const& message )
		{
			kill_received = true;
			std::cout << "action_handler::kill: " << message << std::endl;
		}

		void start( int i )
		{
			start_received = true;
			std::cout << "action_handler::start: " << i << std::endl;
		}

		void stop( bool b )
		{
			stop_received = true;
			std::cout << "action_handler::stop: " << std::boolalpha << b << std::endl;
		}

		bool kill_received = false;
		bool start_received = false;
		bool stop_received = false;

		bool all_events_received() const
		{
			return kill_received && start_received && stop_received;
		}

		bool no_events_received() const
		{
			return ! ( kill_received || start_received || stop_received );
		}

	};

#define USE_EMITTER_BASE( _event_name_ )			\
	using _event_name_::emitter::add_handler;		\
	using _event_name_::emitter::add_listener;		\
	using _event_name_::emitter::emit;				\
/**/

	class action_emitter : public async::multi_emitter< event_test::kill_event, event_test::start_event, event_test::stop_event >
	{
	public:

		void emit_stuff()
		{
			emit( event_test::kill_event{}, "make a jazz noise here" );
			emit( event_test::start_event{}, 42);
			emit( event_test::stop_event{}, false );
		}

	};



}

TEST_CASE( "logicmill/async/event/smoke/handler" )
{
	event_test::killer k;

	CHECK( ! k.kill_received );

	k.handle( event_test::kill_event{}, std::string{ "zoot allures" } );

	CHECK( k.kill_received );
}

TEST_CASE( "logicmill/async/event/smoke/listener" )
{
	bool kill_received = false;

	event_test::kill_event::listener kill_listener = [&] ( std::string const& message )
	{
		std::cout << "kill_listener: " << message << std::endl;
		kill_received = true;
	};

	kill_listener( std::string{ "weasels ripped my flesh" } );

	CHECK( kill_received );
}

TEST_CASE( "logicmill/async/event/smoke/emitter" )
{
	bool listener_received_kill = false;

	event_test::kill_event::listener kill_listener = [&] ( std::string const& message )
	{
		listener_received_kill = true;
		std::cout << "kill_listener: " << message << std::endl;
	};

	event_test::kill_emitter provoker;

	auto kp = std::make_shared< event_test::killer >();

	CHECK( provoker.receiver_count( event_test::kill_event{} ) == 0 );
	CHECK( provoker.handler_count( event_test::kill_event{} ) == 0 );
	CHECK( provoker.listener_count( event_test::kill_event{} ) == 0 );

	auto id = provoker.add_listener( event_test::kill_event{}, kill_listener );

	CHECK( provoker.receiver_count( event_test::kill_event{} ) == 1 );
	CHECK( provoker.handler_count( event_test::kill_event{} ) == 0 );
	CHECK( provoker.listener_count( event_test::kill_event{} ) == 1 );

	provoker.add_handler( event_test::kill_event{}, kp );

	CHECK( provoker.receiver_count( event_test::kill_event{} ) == 2 );
	CHECK( provoker.handler_count( event_test::kill_event{} ) == 1 );
	CHECK( provoker.listener_count( event_test::kill_event{} ) == 1 );

	CHECK( ! kp->kill_received );
	CHECK( ! listener_received_kill );

	provoker.emit( event_test::kill_event{}, "shut up and play yer guitar" );

	CHECK( listener_received_kill );
	CHECK( kp->kill_received );

	CHECK( provoker.remove_handler( event_test::kill_event{}, kp ) );

	CHECK( provoker.receiver_count( event_test::kill_event{} ) == 1 );
	CHECK( provoker.handler_count( event_test::kill_event{} ) == 0 );
	CHECK( provoker.listener_count( event_test::kill_event{} ) == 1 );

	CHECK( provoker.remove_listener( event_test::kill_event{}, id ) );

	CHECK( provoker.receiver_count( event_test::kill_event{} ) == 0 );
	CHECK( provoker.handler_count( event_test::kill_event{} ) == 0 );
	CHECK( provoker.listener_count( event_test::kill_event{} ) == 0 );

}

/*
TEST_CASE( "logicmill/async/event/smoke/multi_event_emitter_handler" )
{
	event_test::action_emitter actor;

	auto ahp = std::make_shared< event_test::action_handler >();

	actor.add_handler( event_test::kill_event{}, ahp );
	actor.add_handler( event_test::start_event{}, ahp );
	actor.add_handler( event_test::stop_event{}, ahp );

	actor.emit( event_test::kill_event{}, "make a jazz noise here" );
	actor.emit( event_test::start_event{}, 42);
	actor.emit( event_test::stop_event{}, false );

	CHECK( true );
}
*/

TEST_CASE( "logicmill/async/event/smoke/multi_event_emitter_handler" )
{
	async::multi_emitter< event_test::kill_event, event_test::start_event, event_test::stop_event > actor;

	auto ahp = std::make_shared< event_test::action_handler >();

	actor.add_handler( event_test::kill_event{}, ahp );
	actor.add_handler( event_test::start_event{}, ahp );
	actor.add_handler( event_test::stop_event{}, ahp );

	CHECK( ahp->no_events_received() );

	actor.emit( event_test::kill_event{}, "Why does it hurt when I pee?" );
	actor.emit( event_test::start_event{}, 127);
	actor.emit( event_test::stop_event{}, true );

	CHECK( ahp->all_events_received() );

	actor.disconnect();
}

TEST_CASE( "logicmill/async/event/smoke/multi_event_emitter_handler" )
{
	async::multi_emitter< event_test::kill_event, event_test::start_event, event_test::stop_event > actor;

	auto ahp = std::make_shared< event_test::action_handler >();

	actor.add_all_handlers( ahp );

	CHECK( ahp->no_events_received() );

	actor.emit( event_test::kill_event{}, "A pound for a brown" );
	actor.emit( event_test::start_event{}, 350);
	actor.emit( event_test::stop_event{}, true );

	CHECK( ahp->all_events_received() );

	actor.disconnect();
}


TEST_CASE( "logicmill/async/event/smoke/derived_multi_event_emitter_handler" )
{
	event_test::action_emitter actor;

	auto ahp = std::make_shared< event_test::action_handler >();

	CHECK( actor.handler_count( event_test::kill_event{} ) == 0 );
	CHECK( actor.handler_count( event_test::start_event{} ) == 0 );
	CHECK( actor.handler_count( event_test::stop_event{} ) == 0 );

	actor.add_all_handlers( ahp );

	CHECK( actor.handler_count( event_test::kill_event{} ) == 1 );
	CHECK( actor.handler_count( event_test::start_event{} ) == 1 );
	CHECK( actor.handler_count( event_test::stop_event{} ) == 1 );
	
	CHECK( actor.total_handler_count() == 3 );

	CHECK( ahp->no_events_received() );

	actor.emit_stuff();

	CHECK( ahp->all_events_received() );

	actor.remove_all_handlers( ahp );

	CHECK( actor.handler_count( event_test::kill_event{} ) == 0 );
	CHECK( actor.handler_count( event_test::start_event{} ) == 0 );
	CHECK( actor.handler_count( event_test::stop_event{} ) == 0 );

	CHECK( actor.total_handler_count() == 0 );

	actor.disconnect();

}

TEST_CASE( "logicmill/async/event/smoke/derived_multi_event_emitter_handler" )
{
	event_test::action_emitter actor;

	auto ahp = std::make_shared< event_test::action_handler >();

	actor.add_handlers( ahp, event_test::kill_event{}, event_test::start_event{}, event_test::stop_event{} );
	
	CHECK( ahp->no_events_received() );

	actor.emit_stuff();

	CHECK( ahp->all_events_received() );

	actor.disconnect();
}

#endif

