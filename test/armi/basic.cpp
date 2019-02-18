#include <doctest.h>
#include <logicmill/armi/armi.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/armi/async_adapter.h>

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
using increment_reply = std::function<void(std::error_code, int n_plus_1)>;
using decrement_reply = std::function<void(std::error_code, int n_minus_1)>;
using error_reply     = std::function<void(std::error_code)>;

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
	void
	increment(increment_reply reply, int n)
	{
		reply(std::error_code{}, n + 1);
		increment_called = true;
	}

	void
	freak_out(error_reply reply)
	{
		reply(make_error_code(foo::errc::sun_exploded));
		freak_out_called = true;
	}

	bool increment_called{false};
	bool freak_out_called{false};
};

class boo
{
public:
	boo(async::loop::ptr lp) : loop{lp} {}

	void
	decrement(decrement_reply reply, int n)
	{
		std::error_code   err;
		async::timer::ptr timer = loop->create_timer(err, [=](async::timer::ptr tp) {
			decrement_called = true;
			reply(std::error_code{}, n - 1);
		});
		CHECK(!err);
		timer->start(std::chrono::milliseconds{3000}, err);
		CHECK(!err);
	}

	async::loop::ptr loop;
	bool             decrement_called{false};
};

}    // namespace foo

template<>
struct std::is_error_condition_enum<foo::errc> : public true_type
{};

using async::ip::endpoint;
using async::ip::address;

namespace rfoo
{
static auto foo_stream_context = bstream::create_context<>({&armi::error_category(), &foo::error_category()});

ARMI_CONTEXT(remote, foo::bar, foo::boo);
ARMI_INTERFACE(remote, foo::bar, increment, freak_out);
ARMI_INTERFACE(remote, foo::boo, decrement);
}    // namespace rfoo

#if 1
TEST_CASE("logicmill::armi [ smoke ] { basic functionality }")
{
	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();
	rfoo::remote context;
	auto server_context = context.create_server();
	server_context->register_impl(std::make_shared<foo::bar>());

	END_LOOP(lp, 5000);

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

	auto client_context = context.create_client();

	async::client_channel_impl::ptr cp = MAKE_SHARED<async::client_channel_impl>(client_context, lp);

	auto client_connect_timer = lp->create_timer(err, [&](async::timer::ptr tp) {
		std::error_code err;
		cp->connect(
				async::options{endpoint{address::v4_loopback(), 7001}},
				err,
				[&](armi::transport::client_channel::ptr chan, std::error_code err) {
					client_context->use(chan);
					client_context->proxy<foo::bar>().increment(
							[](std::error_code err, int result) {
								std::cout << "in increment callback, err is " << err.message() << std::endl;
								CHECK(!err);
								CHECK(result == 28);
							},
							27);
					client_connect_handler_visited = true;
				});
		CHECK(!err);
		client_connect_timer_handler_visited = true;
	});

	CHECK(!err);

	client_connect_timer->start(std::chrono::milliseconds{2000}, err);
	CHECK(!err);

	std::cout << "started connect timer" << std::endl;

	lp->run(err);
	CHECK(!err);

	std::cout << "loop run finished" << std::endl;

	CHECK(client_connect_timer_handler_visited);
	CHECK(client_connect_handler_visited);
	CHECK(server_context->get_impl<foo::bar>()->increment_called);

	lp->close(err);
	CHECK(!err);
}

#endif

#if 1

TEST_CASE("logicmill::armi [ smoke ] { error handling }")
{
	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};
	bool client_close_handler_visited{false};
	bool channel_error_handler_visited{false};
	bool channel_close_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::remote context{rfoo::foo_stream_context};
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

	rfoo::remote context{lp, rfoo::foo_stream_context};

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

	rfoo::remote context{lp, rfoo::foo_stream_context};

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

	rfoo::remote context{lp, rfoo::foo_stream_context};

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

	rfoo::remote context{lp, rfoo::foo_stream_context};

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

	rfoo::remote context{lp, rfoo::foo_stream_context};

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