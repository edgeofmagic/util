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

class killer : public async::emitter<kill_event>
{};

class victim : public async::listener<kill_event, victim>
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
	std::enable_if_t<std::is_same<T, kill_event>::value, sink<T>>
	get_sink()
	{
		return sink<kill_event>{kill_functor(*this)};
	}

	template<class T, class Emitter>
	std::enable_if_t<std::is_same<T, kill_event>::value && async::has_get_source_method<Emitter, T>::value>
	fit(Emitter& e)
	{
		e.template get_source<T>().fit(get_sink<T>());
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
	std::enable_if_t<std::is_same<T, kill_event>::value, sink<T>>
	get_sink()
	{
		return sink<kill_event>{[=](std::string const& s) { this->die(s); }};
	}

	template<class T, class Emitter>
	std::enable_if_t<std::is_same<T, kill_event>::value && async::has_get_source_method<Emitter, T>::value>
	fit(Emitter& e)
	{
		e.template get_source<T>().fit(get_sink<T>());
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
	using emitter<beep_event>::send;
	using emitter<ring_event>::send;

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
	using emitter<squeal_event>::send;
	using emitter<honk_event>::send;

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

	using emitter<squeal_event>::send;
	using emitter<honk_event>::send;

	using emitter<beep_event>::send;
	using emitter<ring_event>::send;

	using foo_base::get_connector;
	using oof_base::get_connector;
	using foo_base::mate;
	using oof_base::mate;

	combo_a(combo_b& b);


	bool                 beep_flag;
	int                  beep_i;
	bool                 ring_flag;
	int                  ring_i;
	std::string          str;
	char                 ch;
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
		beep_i    = i;
	}

	void
	on(ring_event, bool b, int i)
	{
		ring_flag = b;
		ring_i    = i;
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
		ch  = c;
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

	using emitter<squeal_event>::send;
	using emitter<honk_event>::send;

	using emitter<beep_event>::send;
	using emitter<ring_event>::send;

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

TEST_CASE("logicmill::async::event [ smoke ] { functor }")
{

	functor_victim nicole;

	killer oh_jay;

	// oh_jay.get_source<kill_event>().fit(nicole.get_sink<kill_event>());
	// fit<kill_event>(oh_jay, nicole);
	// oh_jay.fit<kill_event>(nicole);
	nicole.fit<kill_event>(oh_jay);

	oh_jay.send<kill_event>("Die, bitch!");


	CHECK(nicole.is_dead());
	CHECK(nicole.message() == "Die, bitch!");
}

TEST_CASE("logicmill::async::event [ smoke ] { naked emitter }")
{

	functor_victim nicole;

	emitter<kill_event> oh_jay;

	// oh_jay.get_source<kill_event>().fit(nicole.get_sink<kill_event>());
	// fit<kill_event>(oh_jay, nicole);
	// oh_jay.fit<kill_event>(nicole);
	oh_jay.fit<kill_event>(nicole);

	oh_jay.send<kill_event>("Die, bitch!");

	CHECK(nicole.is_dead());
	CHECK(nicole.message() == "Die, bitch!");
}

TEST_CASE("logicmill::async::event::sink [ smoke ] { lambda move }")
{
	lambda_victim nicole;

	killer oh_jay;

	// oh_jay.get_source<kill_event>().fit(nicole.get_sink<kill_event>());
	nicole.fit<kill_event>(oh_jay);

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

	using emitter<squeal_event>::send;
	using emitter<honk_event>::send;

	using emitter<beep_event>::send;
	using emitter<ring_event>::send;

	// using foo_base::get_connector;
	// using oof_base::get_connector;
	// using foo_base::mate;
	// using oof_base::mate;

	bool                 beep_flag;
	int                  beep_i;
	bool                 ring_flag;
	int                  ring_i;
	std::string          str;
	char                 ch;
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
		beep_i    = i;
	}

	void
	on(ring_event, bool b, int i)
	{
		ring_flag = b;
		ring_i    = i;
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
		ch  = c;
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

	using emitter<squeal_event>::send;
	using emitter<honk_event>::send;

	using emitter<beep_event>::send;
	using emitter<ring_event>::send;

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

}    // namespace event_test

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
	top    t;
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

#endif
