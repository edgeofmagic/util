#include <doctest.h>
#include <logicmill/armi/armi.h>
#include <logicmill/armi/adapters/async/client_adapter.h>
#include <logicmill/armi/adapters/async/server_adapter.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/bstream/stdlib/pair.h>
#include <logicmill/bstream/stdlib/vector.h>

#define END_LOOP(loop_ptr, delay_ms)                                                                                   \
	{                                                                                                                  \
		std::error_code _err;                                                                                          \
		auto            loop_exit_timer = loop_ptr->create_timer(_err, [](async::timer::ptr tp) {                      \
            std::error_code err;                                                                            \
            tp->loop()->stop(err);                                                                          \
            CHECK(!err);                                                                                    \
        });                                                                                                 \
		CHECK(!_err);                                                                                                  \
		loop_exit_timer->start(std::chrono::milliseconds{delay_ms}, _err);                                             \
		CHECK(!_err);                                                                                                  \
	}


#define DELAYED_ACTION_BEGIN(_loop_ptr)                                                                                \
	{                                                                                                                  \
		std::error_code _err;                                                                                          \
	auto _action_timer = _loop_ptr->create_timer(_err, [&](async::timer::ptr tp)

#define DELAYED_ACTION_END(_delay_ms)                                                                                  \
	);                                                                                                                 \
	CHECK(!_err);                                                                                                      \
	_action_timer->start(std::chrono::milliseconds{_delay_ms}, _err);                                                  \
	CHECK(!_err);                                                                                                      \
	}

using namespace logicmill;

namespace foo
{

enum class errc
{
	ok = 0,
	end_of_the_world_as_we_know_it,
	sun_exploded,
	aliens_invaded,
};

class foo_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override
	{
		return "foo";
	}

	virtual std::string
	message(int ev) const noexcept override
	{
		switch (static_cast<foo::errc>(ev))
		{
			case foo::errc::ok:
				return "success";
			case foo::errc::end_of_the_world_as_we_know_it:
				return "end of the world as we know it";
			case foo::errc::sun_exploded:
				return "sun exploded";
			case foo::errc::aliens_invaded:
				return "aliens invaded";
			default:
				return "unknown foo error";
		}
	}
};

inline std::error_category const&
error_category() noexcept
{
	static foo::foo_category_impl instance;
	return instance;
}

inline std::error_condition
make_error_condition(errc e)
{
	return std::error_condition(static_cast<int>(e), foo::error_category());
}

inline std::error_code
make_error_code(errc e)
{
	return std::error_code(static_cast<int>(e), foo::error_category());
}

class bar
{
public:
	util::promise<int>
	increment(int n)
	{
		util::promise<int> p;
		p.resolve(n + 1);
		increment_called = true;
		return p;
	}

	util::promise<void>
	freak_out()
	{
		util::promise<void> p;
		p.reject(make_error_code(foo::errc::sun_exploded));
		freak_out_called = true;
		return p;
	}

	bool increment_called{false};
	bool freak_out_called{false};
};

class boo
{
public:
	boo(async::loop::ptr lp) : loop{lp} {}

	util::promise<int>
	decrement(int n)
	{
		std::error_code   err;
		util::promise<int> p;
		async::timer::ptr timer = loop->create_timer(err, [=](async::timer::ptr tp) mutable {
			decrement_called = true;
			p.resolve(n - 1);
		});
		CHECK(!err);
		timer->start(std::chrono::milliseconds{3000}, err);
		CHECK(!err);
		return p;
	}

	async::loop::ptr loop;
	bool             decrement_called{false};
};

class bfail
{
public:
	util::promise<double>
	bonk(std::string const& s)
	{
		util::promise<double> p;
		try
		{
			double result = std::stod(s);
			p.resolve(result);
		}
		catch (std::invalid_argument const& e)
		{
			p.reject(make_error_code(std::errc::invalid_argument));
		}
		return p;
	}
};

}    // namespace foo

template<>
struct std::is_error_condition_enum<foo::errc> : public true_type
{};

using async::ip::endpoint;
using async::ip::address;


namespace rfoo
{

class foo_stream_context : public bstream::context<>
{
public:
	foo_stream_context()
		: bstream::context<>{
				  bstream::context_options{}.error_categories({&armi::error_category(), &foo::error_category()})}
	{}
	static bstream::context_base::ptr const&
	get()
	{
		static const bstream::context_base::ptr instance{MAKE_SHARED<foo_stream_context>()};
		return instance;
	}
};

ARMI_CONTEXT(bar_remote, foo::bar, foo_stream_context, increment, freak_out);
ARMI_CONTEXT(boo_remote, foo::boo, foo_stream_context, decrement);
ARMI_CONTEXT(bfail_remote, foo::bfail, foo_stream_context, bonk);


}    // namespace rfoo

namespace armi_test
{

enum class errc
{
	ok = 0,
	end_of_the_world_as_we_know_it,
	sun_exploded,
	aliens_invaded,
};

}

namespace std
{
template<>
struct is_error_condition_enum<armi_test::errc> : public true_type
{};
}

namespace armi_test
{

class armi_test_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override
	{
		return "armi test";
	}

	virtual std::string
	message(int ev) const noexcept override
	{
		switch (static_cast<armi_test::errc>(ev))
		{
			case armi_test::errc::ok:
				return "success";
			case armi_test::errc::end_of_the_world_as_we_know_it:
				return "end of the world as we know it";
			case armi_test::errc::sun_exploded:
				return "sun exploded";
			case armi_test::errc::aliens_invaded:
				return "aliens invaded";
			default:
				return "unknown armi_test error";
		}
	}
};

inline std::error_category const&
error_category() noexcept
{
	static armi_test::armi_test_category_impl instance;
	return instance;
}

inline std::error_condition
make_error_condition(errc e)
{
	return std::error_condition(static_cast<int>(e), armi_test::error_category());
}

inline std::error_code
make_error_code(errc e)
{
	return std::error_code(static_cast<int>(e), armi_test::error_category());
}

class test_stream_context : public bstream::context<>
{
public:
	test_stream_context()
		: bstream::context<>{
				  bstream::context_options{}.error_categories({&armi::error_category(), &armi_test::error_category()})}
	{}
	static bstream::context_base::ptr const&
	get()
	{
		static const bstream::context_base::ptr instance{MAKE_SHARED<test_stream_context>()};
		return instance;
	}
};



class test_fixture;

class target
{
public:
	target(test_fixture& fixture) : m_fixture{fixture} {}

	util::promise<std::string>
	form_6_fail()
	{
		util::promise<std::string> result;
		result.reject(make_error_code(armi_test::errc::sun_exploded));
		return result;
	}

	util::promise<std::string>
	form_6_pass()
	{
		util::promise<std::string> result;
		result.resolve("shamma lamma ding dong");
		return result;
	}

	util::promise<std::string>
	form_7_fail(std::string const& s, int n)
	{
		util::promise<std::string> result;
		result.reject(make_error_code(armi_test::errc::sun_exploded));
		return result;
	}

	util::promise<std::string>
	form_7_pass(std::string const& s, int n)
	{
		util::promise<std::string> result;
		result.resolve(s + std::to_string(n));
		return result;
	}

	util::promise<void>
	form_8_fail()
	{
		util::promise<void> result;
		result.reject(make_error_code(armi_test::errc::sun_exploded));
		return result;
	}

	util::promise<void>
	form_8_pass()
	{
		util::promise<void> result;
		result.resolve();
		return result;
	}
	util::promise<void>
	form_9_fail(std::string const& s, int n)
	{
		util::promise<void> result;
		result.reject(make_error_code(armi_test::errc::sun_exploded));
		return result;
	}

	util::promise<void>
	form_9_pass(std::string const& s, int n)
	{
		util::promise<void> result;
		result.resolve();
		return result;
	}

private:
	test_fixture& m_fixture;
};

ARMI_CONTEXT(
		test_remote,
		target,
		test_stream_context,
		form_6_fail,
		form_6_pass,
		form_7_fail,
		form_7_pass,
		form_8_fail,
		form_8_pass,
		form_9_fail,
		form_9_pass);

class test_fixture
{
public:
	using ref_type = test_remote::client_context_type::client_channel;

	test_fixture()
		: m_loop{async::loop::create()}, m_target{std::make_shared<target>(*this)}, m_client{m_loop}, m_server{m_loop}
	{
		m_server.on_request([&](armi::channel_id_type channel_id) {
					m_server_request_handler_visited = true;
					return m_target;
				})
				.on_channel_connect(
						[=](armi::channel_id_type channel_id) { m_server_channel_connect_handler_visited = true; })
				.on_accept_error([=](std::error_code err) { m_server_accept_error_handler_visited = true; })
				.on_channel_error([=](armi::channel_id_type channel_id, std::error_code err) {
					m_server_channel_error_handler_visited = true;
				})
				.on_channel_close(
						[=](armi::channel_id_type channel_id) { m_server_channel_close_handler_visited = true; })
				.on_server_close([=]() { m_server_close_handler_visited = true; });
	}

	void
	run()
	{
		std::error_code err;
		m_loop->schedule(std::chrono::milliseconds{500}, [](async::loop::ptr const& lp) {
			std::error_code err;
			lp->stop(err);
			CHECK(!err);
		});

		m_server.bind(async::options{endpoint{address::v4_any(), 7001}}, err);
		CHECK(!err);

		m_client.connect(
				async::options{endpoint{address::v4_loopback(), 7001}}, err, [=](ref_type t, std::error_code err) {
					CHECK(t.is_valid());

					t->form_6_fail().then([=](std::string s)
					{
						CHECK(false);
					},
					[=](std::error_code err)
					{
						CHECK(err == make_error_code(armi_test::errc::sun_exploded));
						m_target_form_6_fail_visited = true;
					});
					t->form_6_pass().then([=](std::string s)
					{
						CHECK(s == "shamma lamma ding dong");
						m_target_form_6_pass_visited = true;
					},
					[=](std::error_code err)
					{
						CHECK(false);
					});

					t->form_7_fail("zoot", 3).then([=](std::string s)
					{
						CHECK(false);
					},
					[=](std::error_code err)
					{
						CHECK(err == make_error_code(armi_test::errc::sun_exploded));
						m_target_form_7_fail_visited = true;
					});

					t->form_7_pass("seven: ", 7).then([=](std::string s)
					{
						CHECK(s == "seven: 7");
						m_target_form_7_pass_visited = true;
					},
					[=](std::error_code err)
					{
						CHECK(false);
					});

					t->form_8_fail().then([=]()
					{
						CHECK(false);
					},
					[=](std::error_code err)
					{
						CHECK(err == make_error_code(armi_test::errc::sun_exploded));
						m_target_form_8_fail_visited = true;
					});
					t->form_8_pass().then([=]()
					{
						m_target_form_8_pass_visited = true;
					},
					[=](std::error_code err)
					{
						CHECK(false);
					});

					t->form_9_fail("zoot", 3).then([=]()
					{
						CHECK(false);
					},
					[=](std::error_code err)
					{
						CHECK(err == make_error_code(armi_test::errc::sun_exploded));
						m_target_form_9_fail_visited = true;
					});

					t->form_9_pass("seven: ", 7).then([=]()
					{
						m_target_form_9_pass_visited = true;
					},
					[=](std::error_code err)
					{
						CHECK(false);
					});

					m_client_connect_handler_visited = true;

				});
		CHECK(!err);

		m_loop->run(err);
		CHECK(!err);

		m_client.close();    // optional, but cleaner
		m_server.close();    // optional, but cleaner

		CHECK(m_client_connect_handler_visited);
		CHECK(m_server_request_handler_visited);
		CHECK(m_server_channel_connect_handler_visited);
		CHECK(!m_server_accept_error_handler_visited);
		CHECK(!m_server_channel_error_handler_visited);
		CHECK(m_server_channel_close_handler_visited);
		CHECK(m_server_close_handler_visited);
		CHECK(m_target_form_6_fail_visited);
		CHECK(m_target_form_6_pass_visited);
		CHECK(m_target_form_7_fail_visited);
		CHECK(m_target_form_7_pass_visited);
		CHECK(m_target_form_8_fail_visited);
		CHECK(m_target_form_8_pass_visited);
		CHECK(m_target_form_9_fail_visited);
		CHECK(m_target_form_9_pass_visited);

		CHECK(!err);
	}

	async::loop::ptr                                        m_loop;
	std::shared_ptr<target>                                 m_target;
	async::client_adapter<test_remote::client_context_type> m_client;
	async::server_adapter<test_remote::server_context_type> m_server;

	bool m_client_connect_handler_visited{false};
	bool m_server_request_handler_visited{false};
	bool m_server_channel_connect_handler_visited{false};
	bool m_server_accept_error_handler_visited{false};
	bool m_server_channel_error_handler_visited{false};
	bool m_server_channel_close_handler_visited{false};
	bool m_server_close_handler_visited{false};
	bool m_target_form_6_fail_visited{false};
	bool m_target_form_6_pass_visited{false};
	bool m_target_form_7_fail_visited{false};
	bool m_target_form_7_pass_visited{false};
	bool m_target_form_8_fail_visited{false};
	bool m_target_form_8_pass_visited{false};
	bool m_target_form_9_fail_visited{false};
	bool m_target_form_9_pass_visited{false};
};
}    // namespace armi_test

TEST_CASE("logicmill::armi [ smoke ] { fixture test }")
{
	armi_test::test_fixture tfix;
	tfix.run();
}

#if 1
TEST_CASE("logicmill::armi [ smoke ] { basic functionality }")
{
	bool client_connect_handler_visited{false};
	bool increment_resolve_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();
	using bar_ref       = rfoo::bar_remote::client_context_type::client_channel;
	async::client_adapter<rfoo::bar_remote::client_context_type> client{lp};
	async::server_adapter<rfoo::bar_remote::server_context_type> server{lp};
	auto                                                         impl = std::make_shared<foo::bar>();

	server

			.on_request([&](armi::channel_id_type channel_id) {
				std::cout << "request handler called with channel id " << channel_id << std::endl;
				return impl;
			})
			.on_channel_connect([=](armi::channel_id_type channel_id) {
				std::cout << "channel connection handler called with channel id " << channel_id << std::endl;
			})
			.on_accept_error([=](std::error_code err) {
				std::cout << "accept error handler called with error code " << err.message() << std::endl;
			})
			.on_channel_error([=](armi::channel_id_type channel_id, std::error_code err) {
				std::cout << "accept error handler called with channel id " << channel_id << ", error code "
						  << err.message() << std::endl;
			})
			.on_channel_close([=](armi::channel_id_type channel_id) {
				std::cout << "channel close handler called with channel id " << channel_id << std::endl;
			})
			.on_server_close([=]() { std::cout << "server close handler called" << std::endl; });

	END_LOOP(lp, 500);

	server.bind(async::options{endpoint{address::v4_any(), 7001}}, err);

	CHECK(!err);

	client.connect(async::options{endpoint{address::v4_loopback(), 7001}}, err, [&](bar_ref b, std::error_code err) {
		CHECK(b.is_valid());
		b->increment(27).then(
				[&](int result) {
					CHECK(result == 28);
					increment_resolve_visited = true;
				},
				[](std::error_code err)
				{
					CHECK(false);
				});
		client_connect_handler_visited = true;
	});
	CHECK(!err);

	lp->run(err);
	CHECK(!err);

	std::cout << "loop run finished" << std::endl;

	client.close();    // optional, but cleaner
	server.close();    // optional, but cleaner

	CHECK(client_connect_handler_visited);
	CHECK(increment_resolve_visited);
	CHECK(impl->increment_called);

	// lp->close(err);
	CHECK(!err);
}

#endif

#if 1
TEST_CASE("logicmill::armi [ smoke ] { fail reply without error }")
{
	bool client_connect_handler_visited{false};
	bool fail_handler_visited{false};
	bool reply_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();
	using bfail_ref     = rfoo::bfail_remote::client_context_type::client_channel;
	async::client_adapter<rfoo::bfail_remote::client_context_type> client{lp};
	async::server_adapter<rfoo::bfail_remote::server_context_type> server{lp};
	auto                                                           impl = std::make_shared<foo::bfail>();

	server

			.on_request([&](armi::channel_id_type channel_id) {
				std::cout << "request handler called with channel id " << channel_id << std::endl;
				return impl;
			})
			.on_channel_connect([=](armi::channel_id_type channel_id) {
				std::cout << "channel connection handler called with channel id " << channel_id << std::endl;
			})
			.on_accept_error([=](std::error_code err) {
				std::cout << "accept error handler called with error code " << err.message() << std::endl;
			})
			.on_channel_error([=](armi::channel_id_type channel_id, std::error_code err) {
				std::cout << "accept error handler called with channel id " << channel_id << ", error code "
						  << err.message() << std::endl;
			})
			.on_channel_close([=](armi::channel_id_type channel_id) {
				std::cout << "channel close handler called with channel id " << channel_id << std::endl;
			})
			.on_server_close([=]() { std::cout << "server close handler called" << std::endl; });

	END_LOOP(lp, 500);

	server.bind(async::options{endpoint{address::v4_any(), 7001}}, err);

	CHECK(!err);

	client.connect(async::options{endpoint{address::v4_loopback(), 7001}}, err, [&](bfail_ref b, std::error_code err) {
		CHECK(b.is_valid());
		b->bonk("3.5").then(
				[&](double d) {
					std::cout << "in bonk callback, arg is " << d << std::endl;
					CHECK(d == 3.5);
					reply_handler_visited = true;
				},
				[&](std::error_code err) {
					CHECK(err);
					std::cout << "in bonk fail reply, err is " << err.message() << std::endl;
					fail_handler_visited = true;
				});
		client_connect_handler_visited = true;
	});
	CHECK(!err);

	lp->run(err);
	CHECK(!err);

	std::cout << "loop run finished" << std::endl;

	client.close();    // optional, but cleaner
	server.close();    // optional, but cleaner

	CHECK(client_connect_handler_visited);
	CHECK(reply_handler_visited);
	CHECK(!fail_handler_visited);
	// CHECK(impl->increment_called);

	// lp->close(err);
	CHECK(!err);
}

#endif

#if 1
TEST_CASE("logicmill::armi [ smoke ] { fail reply with error }")
{
	bool client_connect_handler_visited{false};
	bool fail_handler_visited{false};
	bool reply_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();
	using bfail_ref     = rfoo::bfail_remote::client_context_type::client_channel;
	async::client_adapter<rfoo::bfail_remote::client_context_type> client{lp};
	async::server_adapter<rfoo::bfail_remote::server_context_type> server{lp};
	auto                                                           impl = std::make_shared<foo::bfail>();

	server

			.on_request([&](armi::channel_id_type channel_id) {
				std::cout << "request handler called with channel id " << channel_id << std::endl;
				return impl;
			})
			.on_channel_connect([=](armi::channel_id_type channel_id) {
				std::cout << "channel connection handler called with channel id " << channel_id << std::endl;
			})
			.on_accept_error([=](std::error_code err) {
				std::cout << "accept error handler called with error code " << err.message() << std::endl;
			})
			.on_channel_error([=](armi::channel_id_type channel_id, std::error_code err) {
				std::cout << "accept error handler called with channel id " << channel_id << ", error code "
						  << err.message() << std::endl;
			})
			.on_channel_close([=](armi::channel_id_type channel_id) {
				std::cout << "channel close handler called with channel id " << channel_id << std::endl;
			})
			.on_server_close([=]() { std::cout << "server close handler called" << std::endl; });

	END_LOOP(lp, 500);

	server.bind(async::options{endpoint{address::v4_any(), 7001}}, err);

	CHECK(!err);

	client.connect(async::options{endpoint{address::v4_loopback(), 7001}}, err, [&](bfail_ref b, std::error_code err) {
		CHECK(b.is_valid());
		b->bonk("zoot").then(
				[&](double d) {
					std::cout << "in bonk callback, arg is " << d << std::endl;
					CHECK(d == 3.5);
					reply_handler_visited = true;
				},
				[&](std::error_code err) {
					CHECK(err);
					std::cout << "in bonk fail reply, err is " << err.message() << std::endl;
					fail_handler_visited = true;
				});
		client_connect_handler_visited = true;
	});
	CHECK(!err);

	lp->run(err);
	CHECK(!err);

	std::cout << "loop run finished" << std::endl;

	client.close();    // optional, but cleaner
	server.close();    // optional, but cleaner

	CHECK(client_connect_handler_visited);
	CHECK(!reply_handler_visited);
	CHECK(fail_handler_visited);
	CHECK(!err);
}

#endif

#if 0
TEST_CASE("logicmill::armi [ smoke ] { basic functionality, static connect }")
{
	bool client_connect_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();
	// rfoo::bar_remote context;
	auto server_context = rfoo::bar_remote::create_server();
	server_context->register_impl(std::make_shared<foo::bar>());

	END_LOOP(lp, 500);

	async::server_impl::ptr sp = MAKE_SHARED<async::server_impl>(server_context, lp);
	sp->on_server_error([=](async::server_impl::ptr const& srvrp, std::error_code err)
	{
		std::cout << "server error handler called: " << err.message() << std::endl;
		srvrp->close();
	});

	sp->on_channel_error([=](async::server_impl::ptr const& srvrp, async::channel::ptr const& chan, std::error_code err) {
		std::cout << "channel error handler called: " << err.message() << std::endl;
		srvrp->close(chan);
	});

	sp->on_server_close([]()
	{
		std::cout << "server close handler called" << std::endl;
	});

	sp->on_channel_close([](async::channel::ptr const& chan)
	{
		std::cout << "channel close handler called" << std::endl;
	});

	sp->bind(async::options{endpoint{address::v4_any(), 7001}}, err);
	CHECK(!err);

	auto client_context = rfoo::bar_remote::create_client();

	async::client_channel_impl::connect(
			client_context,
			lp,
			async::options{endpoint{address::v4_loopback(), 7001}},
			err,
			[&](armi::transport::client_channel::ptr chan, std::error_code err) {
				client_context->use(chan);
				client_context->proxy().increment(
						[](std::error_code err, int result) {
							std::cout << "in increment callback";
							if (err)
							{
								std::cout << ", err is " << err.message();
							}
							std::cout << std::endl;
							CHECK(!err);
							CHECK(result == 28);
						},
						27);
				client_connect_handler_visited = true;
			});

	CHECK(!err);

	lp->run(err);
	CHECK(!err);

	std::cout << "loop run finished" << std::endl;

	CHECK(client_connect_handler_visited);
	CHECK(server_context->get_impl()->increment_called);

	lp->close(err);
	CHECK(!err);
}

#endif

#if 0

TEST_CASE("logicmill::armi [ smoke ] { error handling }")
{
	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};
	bool client_close_handler_visited{false};
	bool channel_error_handler_visited{false};
	bool channel_close_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::remote context{rfoo::foo_stream_context::get()};
	auto server_context = context.create_server();
	server_context->register_impl(std::make_shared<foo::bar>());

	END_LOOP(lp, 5000);

	async::server_impl::ptr sp = MAKE_SHARED<async::server_impl>(server_context, lp);
	sp->on_server_error([=](async::server_impl::ptr const& srvrp, std::error_code err)
	{
		std::cout << "server error handler called: " << err.message() << std::endl;
		srvrp->close();
	});

	sp->on_channel_error([&](async::server_impl::ptr const& srvrp, async::channel::ptr const& chan, std::error_code err) {
		std::cout << "channel error handler called: " << err.message() << std::endl;
		channel_error_handler_visited = true;
		CHECK(err == async::errc::end_of_file);
		srvrp->close(chan);
	});

	sp->on_server_close([]()
	{
		std::cout << "server close handler called" << std::endl;
	});

	sp->on_channel_close([&](async::channel::ptr const& chan)
	{
		std::cout << "channel close handler called" << std::endl;
		channel_close_handler_visited = true;
	});

	sp->bind(async::options{endpoint{address::v4_any(), 7001}}, err);
	CHECK(!err);

	auto client_context = context.create_client();

	async::client_channel_impl::ptr cp = MAKE_SHARED<async::client_channel_impl>(client_context, lp);

	auto client_connect_timer = lp->create_timer(err, [&](async::timer::ptr tp) {
		std::error_code err;
		cp->connect(
				async::options{endpoint{address::v4_loopback(), 7001}},
				err,
				[&](armi::transport::client_channel::ptr chan, std::error_code err) {
					client_context->use(chan);
					client_context->proxy<foo::bar>().freak_out(
							[](std::error_code err) { CHECK(err == foo::errc::sun_exploded); });
					client_connect_handler_visited = true;
				});
		CHECK(!err);
		client_connect_timer_handler_visited = true;
	});

	CHECK(!err);

	client_connect_timer->start(std::chrono::milliseconds{2000}, err);
	CHECK(!err);

	auto client_close_timer = lp->create_timer(err, [&](async::timer::ptr tp) {
		client_context->close([&](){
			client_close_handler_visited = true;
			std::cout << "client closed completed" << std::endl;
		});
	});

	client_close_timer->start(std::chrono::milliseconds{4000}, err);
	CHECK(!err);

	lp->run(err);
	CHECK(!err);

	CHECK(client_connect_timer_handler_visited);
	CHECK(client_connect_handler_visited);
	CHECK(channel_error_handler_visited);
	CHECK(client_close_handler_visited);
	CHECK(channel_close_handler_visited);
	CHECK(server_context->get_impl<foo::bar>()->freak_out_called);

	lp->close(err);
	CHECK(!err);
}

#endif

#if 0

TEST_CASE("logicmill::armi [ smoke ] { bad error category }")
{
	static auto stream_context = bstream::create_context<>({&armi::error_category()});

	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::remote context{lp, stream_context};

	auto bar_impl = std::make_shared<foo::bar>();

	context.server().register_impl(bar_impl);

	END_LOOP(lp, 5000);

	context.server().bind(
			async::options{endpoint{address::v4_any(), 7001}},
			err,
			[&](rfoo::remote::server_context_type& server, std::error_code err) {
				std::cout << "acceptor error handler: " << err.message() << std::endl;
				CHECK(!err);
				server.close([]() { std::cout << "server close handler called on acceptor error" << std::endl; });
			},
			[](async::channel::ptr const& chan, std::error_code err) {
				std::cout << "channel error handler: " << err.message() << std::endl;
				CHECK(!err);
			});
	CHECK(!err);

	auto client_connect_timer = lp->create_timer(err, [&](async::timer::ptr tp) {
		std::error_code err;
		context.client().connect(
				async::options{endpoint{address::v4_loopback(), 7001}},
				err,
				[&](rfoo::remote::client_context_type& client, std::error_code err) {
					CHECK(!err);
					client.proxy<foo::bar>().freak_out(
							[](std::error_code err) { CHECK(err == bstream::errc::invalid_err_category); });
					client_connect_handler_visited = true;
				});
		CHECK(!err);
		client_connect_timer_handler_visited = true;
	});
	CHECK(!err);

	client_connect_timer->start(std::chrono::milliseconds{2000}, err);
	CHECK(!err);

	lp->run(err);
	CHECK(!err);

	CHECK(client_connect_timer_handler_visited);
	CHECK(client_connect_handler_visited);
	CHECK(bar_impl->freak_out_called);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::armi [ smoke ] { close re-open client }")
{
	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};

	unsigned client_reply_handler_count{0};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::remote context{lp, rfoo::foo_stream_context::get()};

	auto bar_impl = std::make_shared<foo::bar>();

	context.server().register_impl(bar_impl);

	END_LOOP(lp, 5000);

	context.server().bind(
			async::options{endpoint{address::v4_any(), 7001}},
			err,
			[&](rfoo::remote::server_context_type& server, std::error_code err) {
				std::cout << "acceptor error handler: " << err.message() << std::endl;
				server.close([]() { std::cout << "server close handler called on acceptor error" << std::endl; });
			},
			[](async::channel::ptr const& chan, std::error_code err) {
				CHECK(err == async::errc::end_of_file);
				std::cout << "channel error handler: " << err.message() << std::endl;
				chan->close();
			});

	CHECK(!err);

	DELAYED_ACTION_BEGIN(lp)
	{
		std::error_code err;
		context.client().connect(
				async::options{endpoint{address::v4_loopback(), 7001}},
				err,
				[&](rfoo::remote::client_context_type& client, std::error_code err) {
					CHECK(!err);
					client.proxy<foo::bar>().increment(
							[=, &client_reply_handler_count](std::error_code err, int result) {
								CHECK(!err);
								CHECK(result == 43);
								++client_reply_handler_count;
							},
							42);
				});
		CHECK(!err);
	}
	DELAYED_ACTION_END(2000)

	DELAYED_ACTION_BEGIN(lp)
	{
		context.client().close([&]() {
			std::error_code err;
			context.client().connect(
					async::options{endpoint{address::v4_loopback(), 7001}},
					err,
					[&](rfoo::remote::client_context_type& client, std::error_code err) {
						CHECK(!err);
						client.proxy<foo::bar>().increment(
								[=, &client_reply_handler_count](std::error_code err, int result) {
									CHECK(!err);
									CHECK(result == 128);
									++client_reply_handler_count;
								},
								127);
					});
			CHECK(!err);
		});
	}
	DELAYED_ACTION_END(3000)

	lp->run(err);

	context.server().close(nullptr);
	CHECK(!err);
	CHECK(client_reply_handler_count == 2);
	CHECK(bar_impl->increment_called);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::armi [ smoke ] { close server before client request }")
{

	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};

	unsigned client_reply_handler_count{0};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::remote context{lp, rfoo::foo_stream_context::get()};

	auto bar_impl = std::make_shared<foo::bar>();

	context.server().register_impl(bar_impl);

	END_LOOP(lp, 5000);

	context.server().bind(
			async::options{endpoint{address::v4_any(), 7001}},
			err,
			[&](rfoo::remote::server_context_type& server, std::error_code err) {
				std::cout << "acceptor error handler: " << err.message() << std::endl;
				server.close([]() { std::cout << "server close handler called on acceptor error" << std::endl; });
			},
			[](async::channel::ptr const& chan, std::error_code err) {
				CHECK(err == async::errc::end_of_file);
				std::cout << "channel error handler: " << err.message() << std::endl;
				chan->close();
			});

	CHECK(!err);

	DELAYED_ACTION_BEGIN(lp)
	{
		std::error_code err;
		context.client().connect(
				async::options{endpoint{address::v4_loopback(), 7001}},
				err,
				[&](rfoo::remote::client_context_type& client, std::error_code err) {
					CHECK(!err);
					client.proxy<foo::bar>().increment(
							[=, &client_reply_handler_count](std::error_code err, int result) {
								CHECK(!err);
								CHECK(result == 43);
								++client_reply_handler_count;
							},
							42);
				});
		CHECK(!err);
	}
	DELAYED_ACTION_END(2000)

	DELAYED_ACTION_BEGIN(lp)
	{
		context.server().close();
	}
	DELAYED_ACTION_END(3000)

	DELAYED_ACTION_BEGIN(lp)
	{
		context.client().proxy<foo::bar>().increment(
				[=, &client_reply_handler_count](std::error_code err, int result) {
					std::cout << "client request on closed server, err is " << err.message() << std::endl;
					++client_reply_handler_count;
				},
				127);
	}
	DELAYED_ACTION_END(4000)

	lp->run(err);
	CHECK(!err);
	CHECK(client_reply_handler_count == 2);
	CHECK(bar_impl->increment_called);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::armi [ smoke ] { client closes before server sends reply }")
{
	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::remote context{lp, rfoo::foo_stream_context::get()};

	context.server().register_impl(std::make_shared<foo::boo>(lp));

	END_LOOP(lp, 5000);

	unsigned channel_error_handler_count{0};
	unsigned client_reply_handler_count{0};

	context.server().bind(
			async::options{endpoint{address::v4_any(), 7001}},
			err,
			[](rfoo::remote::server_context_type& server, std::error_code err) {
				std::cout << "acceptor error handler: " << err.message() << std::endl;
				server.close([]() { std::cout << "server close handler called on acceptor error" << std::endl; });
			},
			[&](async::channel::ptr const& chan, std::error_code err) {
				if (channel_error_handler_count == 0)
				{
					CHECK(err == async::errc::end_of_file);
					++channel_error_handler_count;
					chan->close();
				}
				else
				{
					CHECK(channel_error_handler_count == 1);
					CHECK(err == armi::errc::channel_not_connected);
					++channel_error_handler_count;
				}
			});

	CHECK(!err);

	DELAYED_ACTION_BEGIN(lp)
	{
		std::error_code err;
		context.client().connect(
				async::options{endpoint{address::v4_loopback(), 7001}},
				err,
				[&](rfoo::remote::client_context_type& client, std::error_code err) {
					CHECK(!err);
					auto& boop = client.proxy<foo::boo>();

					boop.decrement(
							[=, &client_reply_handler_count](std::error_code err, int result) {
								++client_reply_handler_count;
								CHECK(err == std::errc::operation_canceled);
							},
							1);
				});
		CHECK(!err);
	}
	DELAYED_ACTION_END(1000)

	DELAYED_ACTION_BEGIN(lp)
	{
		context.client().close();
	}
	DELAYED_ACTION_END(2000)

	lp->run(err);
	CHECK(!err);

	CHECK(channel_error_handler_count == 2);
	CHECK(client_reply_handler_count == 1);
	CHECK(context.server().get_impl<foo::boo>()->decrement_called);

	lp->close(err);
	CHECK(!err);
}

#endif

#if 0

TEST_CASE("logicmill::armi [ smoke ] { client timeout }")
{
	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::remote context{lp, rfoo::foo_stream_context::get()};

	context.server().register_impl(std::make_shared<foo::boo>(lp));

	END_LOOP(lp, 5000);

	unsigned channel_error_handler_count{0};
	unsigned client_reply_handler_count{0};

	context.server().bind(
			async::options{endpoint{address::v4_any(), 7001}},
			err,
			[](rfoo::remote::server_context_type& server, std::error_code err) {
				std::cout << "acceptor error handler: " << err.message() << std::endl;
				server.close([]() { std::cout << "server close handler called on acceptor error" << std::endl; });
			},
			[&](async::channel::ptr const& chan, std::error_code err) {
				std::cout << "channel error handler, err: " << err.message() << std::endl;
				++channel_error_handler_count;
				chan->close();
			});

	CHECK(!err);

	DELAYED_ACTION_BEGIN(lp)
	{
		std::error_code err;
		context.client().connect(
				async::options{endpoint{address::v4_loopback(), 7001}},
				err,
				[&](rfoo::remote::client_context_type& client, std::error_code err) {
					CHECK(!err);
					auto& boop = client.proxy<foo::boo>();

					boop.timeout(std::chrono::milliseconds{2000})
							.decrement(
									[=, &client_reply_handler_count](std::error_code err, int result) {
										++client_reply_handler_count;
										CHECK(err == std::errc::timed_out);
									},
									1);
				});
		CHECK(!err);
	}
	DELAYED_ACTION_END(1000)

	lp->run(err);
	CHECK(!err);

	CHECK(channel_error_handler_count == 0);
	CHECK(client_reply_handler_count == 1);
	CHECK(context.server().get_impl<foo::boo>()->decrement_called);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::armi [ smoke ] { client timeout }")
{
	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::remote context{lp, rfoo::foo_stream_context::get()};

	context.server().register_impl(std::make_shared<foo::boo>(lp));

	END_LOOP(lp, 5000);

	unsigned channel_error_handler_count{0};
	unsigned client_reply_handler_count{0};

	context.server().bind(
			async::options{endpoint{address::v4_any(), 7001}},
			err,
			[](rfoo::remote::server_context_type& server, std::error_code err) {
				std::cout << "acceptor error handler: " << err.message() << std::endl;
				server.close([]() { std::cout << "server close handler called on acceptor error" << std::endl; });
			},
			[&](async::channel::ptr const& chan, std::error_code err) {
				std::cout << "channel error handler, err: " << err.message() << std::endl;
				++channel_error_handler_count;
				chan->close();
			});

	CHECK(!err);

	DELAYED_ACTION_BEGIN(lp)
	{
		std::error_code err;
		context.client().connect(
				async::options{endpoint{address::v4_loopback(), 7001}},
				err,
				[&](rfoo::remote::client_context_type& client, std::error_code err) {
					CHECK(!err);
					auto& boop = client.proxy<foo::boo>();

					boop.timeout(std::chrono::milliseconds{2000})
							.decrement(
									[=, &client_reply_handler_count](std::error_code err, int result) {
										++client_reply_handler_count;
										CHECK(err == std::errc::timed_out);
									},
									1);
				});
		CHECK(!err);
	}
	DELAYED_ACTION_END(1000)

	lp->run(err);
	CHECK(!err);

	CHECK(channel_error_handler_count == 0);
	CHECK(client_reply_handler_count == 1);
	CHECK(context.server().get_impl<foo::boo>()->decrement_called);

	lp->close(err);
	CHECK(!err);
}

#endif