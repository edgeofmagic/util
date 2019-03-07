#include <doctest.h>
#include <logicmill/armi/armi.h>
#include <logicmill/armi/async_adapter.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>

using namespace logicmill;
using namespace util;

namespace example
{
class up_down_counter
{
public:
	up_down_counter(int initial_value = 0) : m_value{initial_value} {}

	promise<int>
	increment()
	{
		promise<int> result;
		result.resolve(++m_value);
		return result;
	}

	promise<int>
	decrement()
	{
		promise<int> result;
		result.resolve(--m_value);
		return result;
	}

	promise<int>
	get_value() const
	{
		promise<int> result;
		auto         v = m_value;
		result.resolve(v);
		return result;
	}

private:
	int m_value;
};

ARMI_CONTEXT(remote, up_down_counter, armi::default_stream_context, increment, decrement, get_value);
}    // namespace example

using namespace armi;
using namespace example;
using namespace async;

TEST_CASE("logicmill::armi [ smoke ] { example 1 }")
{
	std::error_code  err;
	async::loop::ptr lp = loop::create();
	using counter_ref   = remote::client_context_type::client_channel;
	client_adaptor<remote::client_context_type> client{lp};
	server_adaptor<remote::server_context_type> server{lp};

	auto impl = std::make_shared<up_down_counter>();

	server.on_request([=](channel_id_type id) { return impl; });

	server.bind(options{async::ip::endpoint{async::ip::address::v4_any(), 7001}}, err);

	client.connect(options{async::ip::endpoint{async::ip::address::v4_loopback(), 7001}})
	.then([=] (counter_ref cp)
	{
		cp->increment()
				.then([=](int result) {
					std::cout << result << std::endl;
					cp->decrement().then([=](int result)
					{
						std::cout << result << std::endl;
						lp->stop();
					});
				});
	}, [=](std::error_code err)
	{
		std::cout << err.message() << std::endl;
	});

	lp->run();
}

TEST_CASE("logicmill::armi [ smoke ] { example 2 }")
{
	std::error_code  err;
	async::loop::ptr lp = loop::create();
	using counter_ref   = remote::client_context_type::client_channel;
	client_adaptor<remote::client_context_type> client{lp};
	server_adaptor<remote::server_context_type> server{lp};

	auto impl = std::make_shared<up_down_counter>();

	server.on_request([=](channel_id_type id) { return impl; });

	server.bind(options{async::ip::endpoint{async::ip::address::v4_any(), 7001}}, err);

	client.connect(options{async::ip::endpoint{async::ip::address::v4_loopback(), 7001}})
	.then([=] (counter_ref cp)
	{
		cp->increment()
				.then([=](int n) 
				{
					std::cout << n << std::endl;
					return cp->decrement();
				})
				.then([=](int n) 
				{
					std::cout << n << std::endl;
					return cp->increment();
				})
				.then([=](int n)
				{
					std::cout << n << std::endl;
					lp->stop();
				});

	}, [=](std::error_code err)
	{
		std::cout << err.message() << std::endl;
	});

	lp->run();
}
