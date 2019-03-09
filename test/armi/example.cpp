#include <doctest.h>
#include <logicmill/armi/armi.h>
#include <logicmill/armi/adapters/async/client_adapter.h>
#include <logicmill/armi/adapters/async/server_adapter.h>
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

template<class ServantType>
class servant_manager
{
public:

	void
	new_servant(armi::channel_id_type channel_id)
	{
		m_servants.emplace(channel_id, std::make_shared<ServantType>());
	}

	std::shared_ptr<ServantType>
	get_servant(armi::channel_id_type channel_id)
	{
		std::shared_ptr<ServantType> result;
		auto it = m_servants.find(channel_id);
		if (it != m_servants.end())
		{
			result = it->second;
		}
		return result;
	}

	void
	remove_servant(armi::channel_id_type channel_id)
	{
		m_servants.erase(channel_id);		
	}

private:
	std::unordered_map<armi::channel_id_type, std::shared_ptr<ServantType>> m_servants;
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
	client_adapter<remote::client_context_type> client{lp};
	server_adapter<remote::server_context_type> server{lp};

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
	client_adapter<remote::client_context_type> client{lp};
	server_adapter<remote::server_context_type> server{lp};

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

TEST_CASE("logicmill::armi [ smoke ] { example 3 }")
{
	std::error_code  err;
	async::loop::ptr lp = loop::create();
	using counter_ref   = remote::client_context_type::client_channel;
	client_adapter<remote::client_context_type> client{lp};
	server_adapter<remote::server_context_type> server{lp};
	servant_manager<up_down_counter> smgr;

	server.on_request([&smgr](channel_id_type id) { return smgr.get_servant(id); })
			.on_channel_connect([&smgr](channel_id_type channel_id) { smgr.new_servant(channel_id); })
			.on_channel_error([=, &server](channel_id_type channel_id, std::error_code err) {
				std::cout << "server channel error, channel id: " << channel_id << ", error: " << err.message()
						  << std::endl;
				server.close(channel_id);
			})
			.on_channel_close([&smgr](channel_id_type channel_id) {
				std::cout << "server channel closing, channel id: " << channel_id << std::endl;
				smgr.remove_servant(channel_id);
			});

	server.bind(options{async::ip::endpoint{async::ip::address::v4_any(), 7001}}, err);

	client.connect(options{async::ip::endpoint{async::ip::address::v4_loopback(), 7001}})
	.then([=] (counter_ref cp)
	{
		cp->decrement()
				.then([=](int n) 
				{
					std::cout << n << std::endl;
					return cp->decrement();
				})
				.then([=](int n) 
				{
					std::cout << n << std::endl;
					return cp->decrement();
				})
				.then([=](int n) mutable
				{
					std::cout << n << std::endl;
					cp.close();
				});

	}, [=](std::error_code err)
	{
		std::cout << err.message() << std::endl;
	});

	client.connect(options{async::ip::endpoint{async::ip::address::v4_loopback(), 7001}})
	.then([=] (counter_ref cp)
	{
		cp->increment()
				.then([=](int n) 
				{
					std::cout << n << std::endl;
					return cp->increment();
				})
				.then([=](int n) 
				{
					std::cout << n << std::endl;
					return cp->increment();
				})
				.then([=](int n) mutable
				{
					std::cout << n << std::endl;
					cp.close();
					lp->schedule(std::chrono::milliseconds{100}, [=](async::loop::ptr const& lp)
					{
						lp->stop();
					});
				});

	}, [=](std::error_code err)
	{
		std::cout << err.message() << std::endl;
	});

	lp->run();
}