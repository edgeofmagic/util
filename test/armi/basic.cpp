#include <doctest.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/armi/armi.h>


using namespace logicmill;

namespace foo
{
using increment_reply = std::function<void(std::error_code, int n_plus_1)>;
using error_reply = std::function< void (std::error_code) >;

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
	message( int ev ) const noexcept override
	{
		switch ( static_cast< foo::errc > (ev) )
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
    return std::error_condition( static_cast< int >( e ), foo::error_category() );
}

inline std::error_code
make_error_code(errc e)
{
    return std::error_code( static_cast< int >( e ), foo::error_category() );
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

}    // namespace foo


namespace std
{

template<>
struct is_error_condition_enum<foo::errc> : public true_type
{};

}    // namespace std




namespace rfoo
{
ARMI_INTERFACES(foo::bar)
ARMI_INTERFACE(foo::bar, increment, freak_out)
ARMI_CONTEXT()
}    // namespace rfoo

namespace nfoo
{
	ARMI_CONTEXT_A(funk, foo::bar)
	ARMI_INTERFACE_A(funk, foo::bar, increment, freak_out)
}


TEST_CASE("logicmill::armi [ smoke ] { basic functionality }")
{

	bool loop_stop_timer_handler_visited{false};
	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	rfoo::context().loop(lp);

	auto bar_impl = std::make_shared<foo::bar>();

	rfoo::server().register_impl(bar_impl);

	auto loop_exit_timer = lp->create_timer(err, [=,&loop_stop_timer_handler_visited](async::timer::ptr tp) {
		std::error_code err;
		tp->loop()->stop(err);
		loop_stop_timer_handler_visited = true;
		CHECK(!err);
	});
	loop_exit_timer->start(std::chrono::milliseconds{5000}, err);

	CHECK(!err);

	async::ip::endpoint bind_ep{async::ip::address::v4_any(), 7001};
	rfoo::server().bind(async::options{bind_ep}, err, 
	[=](std::error_code err)
	{
		std::cout << "listener error handler: " << err.message() << std::endl;
		CHECK(!err);
		rfoo::server().close([=]() { std::cout << "server close handler called on listener error" << std::endl; });
	},
	[=](async::channel::ptr const& chan, std::error_code err)
	{
		std::cout << "channel error handler: " << err.message() << std::endl;
		CHECK(!err);
	});

	CHECK(!err);

	auto client_connect_timer = lp->create_timer(err, [&client_connect_timer_handler_visited,&client_connect_handler_visited](async::timer::ptr tp)
	{
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};
		async::options connect_opt{connect_ep};
		std::error_code err;
		rfoo::client().connect(async::options{connect_ep}, err, [=,&client_connect_handler_visited](std::error_code err) {
			CHECK(!err);
			rfoo::client().proxy<foo::bar>()->increment(
					[=](std::error_code err, int result) {
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

	lp->run(err);

	CHECK(!err);

	CHECK(loop_stop_timer_handler_visited);
	CHECK(client_connect_timer_handler_visited);
	CHECK(client_connect_handler_visited);

	CHECK(bar_impl->increment_called);

	lp->close(err);
	CHECK(!err);
}


TEST_CASE("logicmill::armi [ smoke ] { error handling }")
{

	bool loop_stop_timer_handler_visited{false};
	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();


	static const bstream::context<> foo_stream_context({&armi::error_category(), &foo::error_category()});

	rfoo::context_type foo_context{lp, foo_stream_context};


	// foo_context.loop(lp);

	auto bar_impl = std::make_shared<foo::bar>();

	foo_context.server().register_impl(bar_impl);

	auto loop_exit_timer = lp->create_timer(err, [=,&loop_stop_timer_handler_visited](async::timer::ptr tp) {
		std::error_code err;
		tp->loop()->stop(err);
		loop_stop_timer_handler_visited = true;
		CHECK(!err);
	});
	loop_exit_timer->start(std::chrono::milliseconds{5000}, err);

	CHECK(!err);

	async::ip::endpoint bind_ep{async::ip::address::v4_any(), 7001};
	foo_context.server().bind(async::options{bind_ep}, err, 
	[=,&foo_context](std::error_code err)
	{
		std::cout << "listener error handler: " << err.message() << std::endl;
		CHECK(!err);
		foo_context.server().close([=]() { std::cout << "server close handler called on listener error" << std::endl; });
	},
	[=](async::channel::ptr const& chan, std::error_code err)
	{
		std::cout << "channel error handler: " << err.message() << std::endl;
		CHECK(!err);
	});

	CHECK(!err);

	auto client_connect_timer = lp->create_timer(err, [&foo_context,&client_connect_timer_handler_visited,&client_connect_handler_visited](async::timer::ptr tp)
	{
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};
		async::options connect_opt{connect_ep};
		std::error_code err;
		foo_context.client().connect(async::options{connect_ep}, err, [=,&foo_context,&client_connect_handler_visited](std::error_code err) {
			CHECK(!err);
			foo_context.client().proxy<foo::bar>()->freak_out(
					[=](std::error_code err) {
						CHECK(err == foo::errc::sun_exploded);
					});
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

	CHECK(loop_stop_timer_handler_visited);
	CHECK(client_connect_timer_handler_visited);
	CHECK(client_connect_handler_visited);

	CHECK(bar_impl->freak_out_called);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::armi [ smoke ] { bad error category }")
{

	bool loop_stop_timer_handler_visited{false};
	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();


	static const bstream::context<> foo_stream_context({&armi::error_category()});

	rfoo::context_type foo_context{lp, foo_stream_context};


	foo_context.loop(lp);

	auto bar_impl = std::make_shared<foo::bar>();

	foo_context.server().register_impl(bar_impl);

	auto loop_exit_timer = lp->create_timer(err, [=,&loop_stop_timer_handler_visited](async::timer::ptr tp) {
		std::error_code err;
		tp->loop()->stop(err);
		loop_stop_timer_handler_visited = true;
		CHECK(!err);
	});
	loop_exit_timer->start(std::chrono::milliseconds{5000}, err);

	CHECK(!err);

	async::ip::endpoint bind_ep{async::ip::address::v4_any(), 7001};
	foo_context.server().bind(async::options{bind_ep}, err, 
	[=,&foo_context](std::error_code err)
	{
		std::cout << "listener error handler: " << err.message() << std::endl;
		CHECK(!err);
		foo_context.server().close([=]() { std::cout << "server close handler called on listener error" << std::endl; });
	},
	[=](async::channel::ptr const& chan, std::error_code err)
	{
		std::cout << "channel error handler: " << err.message() << std::endl;
		CHECK(!err);
	});

	CHECK(!err);

	auto client_connect_timer = lp->create_timer(err, [&foo_context,&client_connect_timer_handler_visited,&client_connect_handler_visited](async::timer::ptr tp)
	{
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};
		async::options connect_opt{connect_ep};
		std::error_code err;
		foo_context.client().connect(async::options{connect_ep}, err, [=,&foo_context,&client_connect_handler_visited](std::error_code err) {
			CHECK(!err);
			foo_context.client().proxy<foo::bar>()->freak_out(
					[=](std::error_code err) {
						CHECK(err == bstream::errc::invalid_err_category);
						// CHECK(err == foo::errc::sun_exploded);
						
					});
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

	CHECK(loop_stop_timer_handler_visited);
	CHECK(client_connect_timer_handler_visited);
	CHECK(client_connect_handler_visited);

	CHECK(bar_impl->freak_out_called);

	lp->close(err);
	CHECK(!err);
}


TEST_CASE("logicmill::armi [ smoke ] { error handling, new class containment }")
{

	bool loop_stop_timer_handler_visited{false};
	bool client_connect_timer_handler_visited{false};
	bool client_connect_handler_visited{false};

	std::error_code  err;
	async::loop::ptr lp = async::loop::create();

	static const bstream::context<> foo_stream_context({&armi::error_category(), &foo::error_category()});

	nfoo::funk funky{lp, foo_stream_context};

	// foo_context.loop(lp);

	auto bar_impl = std::make_shared<foo::bar>();

	funky.server().register_impl(bar_impl);

	auto loop_exit_timer = lp->create_timer(err, [=,&loop_stop_timer_handler_visited](async::timer::ptr tp) {
		std::error_code err;
		tp->loop()->stop(err);
		loop_stop_timer_handler_visited = true;
		CHECK(!err);
	});
	loop_exit_timer->start(std::chrono::milliseconds{5000}, err);

	CHECK(!err);

	async::ip::endpoint bind_ep{async::ip::address::v4_any(), 7001};
	funky.server().bind(async::options{bind_ep}, err, 
	[=,&funky](std::error_code err)
	{
		std::cout << "listener error handler: " << err.message() << std::endl;
		CHECK(!err);
		funky.server().close([=]() { std::cout << "server close handler called on listener error" << std::endl; });
	},
	[=](async::channel::ptr const& chan, std::error_code err)
	{
		std::cout << "channel error handler: " << err.message() << std::endl;
		CHECK(!err);
	});

	CHECK(!err);

	auto client_connect_timer = lp->create_timer(err, [&funky,&client_connect_timer_handler_visited,&client_connect_handler_visited](async::timer::ptr tp)
	{
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};
		async::options connect_opt{connect_ep};
		std::error_code err;
		funky.client().connect(async::options{connect_ep}, err, [=,&funky,&client_connect_handler_visited](std::error_code err) {
			CHECK(!err);
			funky.client().proxy<foo::bar>()->freak_out(
					[=](std::error_code err) {
						CHECK(err == foo::errc::sun_exploded);
					});
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

	CHECK(loop_stop_timer_handler_visited);
	CHECK(client_connect_timer_handler_visited);
	CHECK(client_connect_handler_visited);

	CHECK(bar_impl->freak_out_called);

	lp->close(err);
	CHECK(!err);
}

