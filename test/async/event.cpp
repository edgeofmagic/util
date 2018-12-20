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

#include <doctest.h>
#include <iostream>
#include <logicmill/async/event.h>

using namespace logicmill;
using namespace async;

#if 1
namespace event_test
{
enum class actions
{
	start,
	stop,
	kill
};

using kill_event  = async::event<actions, actions::kill, std::string const&>;
using start_event = async::event<actions, actions::start, int>;
using stop_event  = async::event<actions, actions::stop, bool>;

class killer : public async::source_base<kill_event>
{};

class victim : public async::sink_base<kill_event, victim>
{
public:
	void
	on(kill_event, std::string const& msg)
	{
		m_is_dead = true;
		m_message = msg;
	}

	bool
	is_dead() const
	{
		return m_is_dead;
	}

	std::string const&
	message() const
	{
		return m_message;
	}

private:
	bool        m_is_dead;
	std::string m_message;
};

class functor_victim
{
public:
	functor_victim() : m_is_dead{false}, m_message{} {}

	void
	die(std::string const& message)
	{
		m_is_dead = true;
		m_message = message;
	}

	bool
	is_dead() const
	{
		return m_is_dead;
	}

	std::string const&
	message() const
	{
		return m_message;
	}

	struct kill_functor
	{
		kill_functor(functor_victim& ksrc) : m_victim{ksrc} {}

		functor_victim& m_victim;

		void
		operator()(std::string const& s)
		{
			m_victim.die(s);
		}
	};

	template<class T>
	std::enable_if_t<std::is_same<T,kill_event>::value,sink<T>>
	get_sink()
	{
		return sink<kill_event>{kill_functor(*this)};
	}

	bool        m_is_dead;
	std::string m_message;
};

class lambda_victim
{
public:
	lambda_victim() : m_is_dead{false}, m_message{} {}

	void
	die(std::string const& message)
	{
		m_is_dead = true;
		m_message = message;
	}

	bool
	is_dead() const
	{
		return m_is_dead;
	}

	std::string const&
	message() const
	{
		return m_message;
	}

	template<class T>
	std::enable_if_t<std::is_same<T,kill_event>::value,sink<T>>
	get_sink()
	{
		return sink<kill_event>{[=](std::string const& s) { this->die(s); }};

	}

	bool        m_is_dead;
	std::string m_message;
};

using snuff = connector<source<kill_event>, sink<kill_event>>;

class murder : public connectable<snuff, murder>
{
public:
	void
	on(kill_event, std::string const& msg)
	{
		m_is_dead = true;
		m_message = msg;
	}

	bool
	is_dead() const
	{
		return m_is_dead;
	}

	std::string const&
	message() const
	{
		return m_message;
	}

private:
	bool        m_is_dead = false;
	std::string m_message;
};

using ffuns = complement<snuff>::type;

class redrum : public connectable<complement<snuff>::type, redrum>
{
public:
	void
	on(kill_event, std::string const& msg)
	{
		m_is_dead = true;
		m_message = msg;
	}

	bool
	is_dead() const
	{
		return m_is_dead;
	}

	std::string const&
	message() const
	{
		return m_message;
	}

private:
	bool        m_is_dead = false;
	std::string m_message;
};

enum class noise
{
	beep,
	ring,
	squeal,
	honk
};

using beep_event   = event<noise, noise::beep, bool, int>;
using ring_event   = event<noise, noise::ring, bool, int>;
using squeal_event = event<noise, noise::squeal, std::string const&, char>;
using honk_event   = event<noise, noise::honk, std::shared_ptr<int>>;

using foo_con = connector<source<beep_event>, source<ring_event>, sink<squeal_event>, sink<honk_event>>;

class foo : public connectable<foo_con, foo>
{
public:
	using source_base<beep_event>::send;
	using source_base<ring_event>::send;

	void
	on(squeal_event, std::string const& s, char c)
	{
		squeal_string = s;
		squeal_char   = c;
	}

	void
	on(honk_event, std::shared_ptr<int> p)
	{
		honk_ptr = p;
	}

	std::string squeal_string;
	char        squeal_char = ' ';

	std::shared_ptr<int> honk_ptr = nullptr;
};

using oof_con = complement<foo_con>::type;

class oof : public connectable<complement<foo_con>::type, oof>
{
public:
	using source_base<squeal_event>::send;
	using source_base<honk_event>::send;

	void
	on(beep_event, bool b, int i)
	{
		beep_flag = b;
		beep_num  = i;
	}

	void
	on(ring_event, bool b, int i)
	{
		ring_flag = b;
		ring_num  = i;
	}

	bool beep_flag = false;
	int  beep_num  = 0;
	bool ring_flag = false;
	int  ring_num  = 0;
};

class combo_b;

class combo_a : public connectable<foo_con, combo_a>, public connectable<oof_con, combo_a>
{
public:


	using foo_base = connectable<foo_con, combo_a>;
	using oof_base = connectable<oof_con, combo_a>;

	using source_base<squeal_event>::send;
	using source_base<honk_event>::send;

	using source_base<beep_event>::send;
	using source_base<ring_event>::send;

	using foo_base::get_connector;
	using oof_base::get_connector;
	using foo_base::mate;
	using oof_base::mate;

	combo_a(combo_b& b);        


	bool beep_flag;
	int beep_i;
	bool ring_flag;
	int ring_i;
	std::string str;
	char ch;
	std::shared_ptr<int> intp;

	void
	send_beep()
	{
		send<beep_event>(false, 1);
	}

	void
	send_ring()
	{
		send<ring_event>(true, 2);
	}

	void
	on(beep_event, bool b, int i)
	{
		beep_flag = b;
		beep_i = i;
	}

	void
	on(ring_event, bool b, int i)
	{
		ring_flag = b;
		ring_i = i;
	}

	void
	send_squeal()
	{
		send<squeal_event>("hello", 'Z');
	}

	void
	on(squeal_event, std::string const& s, char c)
	{
		str = s;
		ch = c;
	} 

	void
	send_honk()
	{
		send<honk_event>(std::make_shared<int>(3));
	}

	void
	on(honk_event, std::shared_ptr<int> ip)
	{
		intp = ip;
	}

};

class combo_b : public connectable<foo_con, combo_b>, public connectable<oof_con, combo_b>
{
public:

	using foo_base = connectable<foo_con, combo_b>;
	using oof_base = connectable<oof_con, combo_b>;

	using source_base<squeal_event>::send;
	using source_base<honk_event>::send;

	using source_base<beep_event>::send;
	using source_base<ring_event>::send;

	using foo_base::get_connector;
	using oof_base::get_connector;
	using foo_base::mate;
	using oof_base::mate;

	void
	on(beep_event, bool b, int i)
	{
		send<beep_event>(b, i);
	}

	void
	on(ring_event, bool b, int i)
	{
		send<ring_event>(b, i);
	}

	void
	on(squeal_event, std::string const& s, char c)
	{
		send<squeal_event>(s, c);
	} 

	void
	on(honk_event, std::shared_ptr<int> ip)
	{
		send<honk_event>(ip);
	}

};

}    // namespace event_test

event_test::combo_a::combo_a(combo_b& b)
{
	get_connector<foo_con>().mate(b.get_connector<oof_con>());
	get_connector<oof_con>().mate(b.get_connector<foo_con>());
}


using namespace event_test;

TEST_CASE("logicmill::async::event [ smoke ] { lambda }")
{

	functor_victim nicole;

	killer oh_jay;

	oh_jay.get_source<kill_event>().fit(nicole.get_sink<kill_event>());


	oh_jay.send<kill_event>("Die, bitch!");


	CHECK(nicole.is_dead());
	CHECK(nicole.message() == "Die, bitch!");
}

TEST_CASE("logicmill::async::event::sink [ smoke ] { functor move }")
{
	lambda_victim nicole;

	killer oh_jay;

	oh_jay.get_source<kill_event>().fit(nicole.get_sink<kill_event>());

	oh_jay.send<kill_event>("Die, bitch!");


	CHECK(nicole.is_dead());
	CHECK(nicole.message() == "Die, bitch!");
}

TEST_CASE("logicmill::async::event::connector [ smoke ] { 1 }")
{
	lambda_victim  nicole;
	functor_victim jfk;

	killer oh_jay;
	killer lee_harvey;

	connector<source<kill_event>, sink<kill_event>> left{oh_jay.get_source<kill_event>(), jfk.get_sink<kill_event>()};
	connector<sink<kill_event>, source<kill_event>> right{nicole.get_sink<kill_event>(),
														  lee_harvey.get_source<kill_event>()};

	left.mate(right);

	oh_jay.send<kill_event>("Die, bitch!");
	lee_harvey.send<kill_event>("Bang, bang bang!");


	CHECK(nicole.is_dead());
	CHECK(nicole.message() == "Die, bitch!");

	CHECK(jfk.is_dead());
	CHECK(jfk.message() == "Bang, bang bang!");
}

TEST_CASE("logicmill::async::event::connector [ smoke ] { 2 }")
{
	victim nicole;

	killer oh_jay;

	oh_jay.get_source<kill_event>().fit(nicole.get_sink<kill_event>());

	oh_jay.send<kill_event>("Die, bitch!");


	CHECK(nicole.is_dead());
	CHECK(nicole.message() == "Die, bitch!");
}


TEST_CASE("logicmill::async::event::connectable [ smoke ] { simple }")
{
	murder top;
	redrum bottom;

	top.mate(bottom);

	//	top.get_connector< snuff >().mate( bottom.get_connector< ffuns >() );

	top.send<kill_event>("Die, bitch!");
	bottom.send<kill_event>("Bang, bang bang!");


	CHECK(top.is_dead());
	CHECK(top.message() == "Bang, bang bang!");

	CHECK(bottom.is_dead());
	CHECK(bottom.message() == "Die, bitch!");
}

TEST_CASE("logicmill::async::event::connectable [ smoke ] { complex }")
{
	foo f;
	oof o;

	f.get_connector<foo_con>().mate(o.get_connector<oof_con>());

	f.send<beep_event>(true, 27);
	f.send<ring_event>(true, 108);


	CHECK(o.beep_flag == true);
	CHECK(o.beep_num == 27);
	CHECK(o.ring_flag == true);
	CHECK(o.ring_num == 108);

	o.send<squeal_event>("zoot", 'x');
	o.send<honk_event>(std::make_shared<int>(42));

	CHECK(f.squeal_string == "zoot");
	CHECK(f.squeal_char == 'x');
	CHECK(*f.honk_ptr == 42);
}
namespace event_test
{

using top_surface = surface<foo_con, oof_con>;

using bottom_surface = surface<oof_con, foo_con>;

class top : public stackable<top_surface, top>
{
public:

	using foo_base = connectable<foo_con, top>;
	using oof_base = connectable<oof_con, top>;

	using source_base<squeal_event>::send;
	using source_base<honk_event>::send;

	using source_base<beep_event>::send;
	using source_base<ring_event>::send;

	// using foo_base::get_connector;
	// using oof_base::get_connector;
	// using foo_base::mate;
	// using oof_base::mate;

	bool beep_flag;
	int beep_i;
	bool ring_flag;
	int ring_i;
	std::string str;
	char ch;
	std::shared_ptr<int> intp;

	void
	send_beep()
	{
		send<beep_event>(false, 1);
	}

	void
	send_ring()
	{
		send<ring_event>(true, 2);
	}

	void
	on(beep_event, bool b, int i)
	{
		beep_flag = b;
		beep_i = i;
	}

	void
	on(ring_event, bool b, int i)
	{
		ring_flag = b;
		ring_i = i;
	}

	void
	send_squeal()
	{
		send<squeal_event>("hello", 'Z');
	}

	void
	on(squeal_event, std::string const& s, char c)
	{
		str = s;
		ch = c;
	} 

	void
	send_honk()
	{
		send<honk_event>(std::make_shared<int>(3));
	}

	void
	on(honk_event, std::shared_ptr<int> ip)
	{
		intp = ip;
	}

};

class bottom : public stackable<bottom_surface, bottom>
{
public:

	using foo_base = connectable<foo_con, top>;
	using oof_base = connectable<oof_con, top>;

	using source_base<squeal_event>::send;
	using source_base<honk_event>::send;

	using source_base<beep_event>::send;
	using source_base<ring_event>::send;

	// using foo_base::get_connector;
	// using oof_base::get_connector;
	// using foo_base::mate;
	// using oof_base::mate;

	void
	on(beep_event, bool b, int i)
	{
		send<beep_event>(b, i);
	}

	void
	on(ring_event, bool b, int i)
	{
		send<ring_event>(b, i);
	}

	void
	on(squeal_event, std::string const& s, char c)
	{
		send<squeal_event>(s, c);
	} 

	void
	on(honk_event, std::shared_ptr<int> ip)
	{
		send<honk_event>(ip);
	}

};

}

TEST_CASE("logicmill::async::event::connectable [ smoke ] { double wrap }")
{
	combo_b b;
	combo_a a(b);

	a.send_beep();
	a.send_ring();
	a.send_squeal();
	a.send_honk();


	CHECK(a.beep_flag == false);
	CHECK(a.beep_i == 1);
	CHECK(a.ring_flag == true);
	CHECK(a.ring_i == 2);
	CHECK(a.str == "hello");
	CHECK(a.ch == 'Z');
	CHECK(*a.intp == 3);

	b.send<beep_event>(true, 27);
	CHECK(a.beep_flag == true);
	CHECK(a.beep_i == 27);  

}

TEST_CASE("logicmill::async::event::surface [ smoke ] { basic }")
{
	top t;
	bottom b;

	// t.get_surface<top_surface>().stack(b.get_surface<bottom_surface>());

	t.stack(b);

	t.send_beep();
	t.send_ring();
	t.send_squeal();
	t.send_honk();


	CHECK(t.beep_flag == false);
	CHECK(t.beep_i == 1);
	CHECK(t.ring_flag == true);
	CHECK(t.ring_i == 2);
	CHECK(t.str == "hello");
	CHECK(t.ch == 'Z');
	CHECK(*t.intp == 3);
}

#if 0
	class killer : public kill_event::handler< killer >
	{
	public:
		using base = kill_event::handler< killer >;

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

	class killer : public kill_event::emitter
	{
	};

	class action_handler : 
		public kill_event::handler< action_handler >,
		public start_event::handler< action_handler >,
		public stop_event::handler< action_handler >
	{
	public:
		action_handler() 
		: 
		kill_event::handler< action_handler >{ &action_handler::kill },
		start_event::handler< action_handler >{ &action_handler::start },
		stop_event::handler< action_handler >{ &action_handler::stop }
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

#define USE_EMITTER_BASE(_event_name_)                                                                                 \
	using _event_name_::emitter::add_handler;                                                                          \
	using _event_name_::emitter::add_listener;                                                                         \
	using _event_name_::emitter::emit;                                                                                 \
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

	event_test::killer provoker;

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

#endif
