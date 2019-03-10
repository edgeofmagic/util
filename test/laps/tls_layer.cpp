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
#include <logicmill/async/loop.h>
#include <logicmill/laps/channel_anchor.h>
#include <logicmill/laps/tls.h>
#include <logicmill/laps/driver.h>
#include <logicmill/util/buffer.h>

using namespace logicmill;
using namespace async;
using namespace event_flow;
using namespace laps;

class client_credentials : public Botan::Credentials_Manager
{
	std::unique_ptr<Botan::Certificate_Store_In_Memory> m_certstore;

	public:
		client_credentials() : m_certstore{std::make_unique<Botan::Certificate_Store_In_Memory>("testdata/trusted")} {}

		std::vector<Botan::Certificate_Store*>
		trusted_certificate_authorities(const std::string& type, const std::string& context) override
		{
			std::cout << "client_credentials::trusted_certificate_authorities called, type: " << type
					  << ", context: " << context << std::endl;
			return {m_certstore.get()};
	}

	std::vector<Botan::X509_Certificate>
	cert_chain(const std::vector<std::string>& cert_key_types,
			   const std::string& type,
			   const std::string& context) override
	{
		return std::vector<Botan::X509_Certificate>();
	}

	Botan::Private_Key*
	private_key_for(const Botan::X509_Certificate& cert, const std::string& type, const std::string& context) override
	{
		return nullptr;
	}
};

class server_credentials : public Botan::Credentials_Manager
{
	public:
	server_credentials()
	{
		std::ifstream keyin{"testdata/edge.key"};
		Botan::DataSource_Stream keysource{keyin};
		m_key = Botan::PKCS8::load_key(keysource, "k8tylied");
		if (m_key) { std::cout << "key loaded" << std::endl; }
		else
		{
			std::cout << "key load failed" << std::endl;
		}


		std::ifstream certin{"testdata/chain.crt"};
		Botan::DataSource_Stream certsrc{certin};

		while(!certsrc.end_of_data())
		{
			try
			{
				Botan::X509_Certificate cert(certsrc);
				m_chain.emplace_back(cert);
			}
			catch(const Botan::Decoding_Error& e)
			{
				std::cerr << e.what() << '\n';
			}
			std::cout << "server cert chain has " << m_chain.size() << " entries" << std::endl;
		}


	}

	std::vector<Botan::Certificate_Store*>
	trusted_certificate_authorities(const std::string& type, const std::string& context) override
	{
		// if client authentication is required, this function
		// shall return a list of certificates of CAs we trust
		// for tls client certificates, otherwise return an empty list
		return std::vector<Botan::Certificate_Store*>();
	}

	std::vector<Botan::X509_Certificate>
	cert_chain(const std::vector<std::string>& cert_key_types,
			   const std::string& type,
			   const std::string& context) override
	{
		std::vector<Botan::X509_Certificate> result;
		std::cout << "server_credentials::cert_chain called, type: " << type
				  << ", context: " << context << std::endl;
		for (auto& key_type : cert_key_types)
		{
			std::cout << "cert_key_type: " << key_type << std::endl;
			if (key_type == "RSA")
			{
				result = m_chain;
			}
		}
		// return the certificate chain being sent to the tls client
		// e.g., the certificate file "botan.randombit.net.crt"
		return result;
	}

	Botan::Private_Key*
	private_key_for(const Botan::X509_Certificate& cert, const std::string& type, const std::string& context) override
	{
		std::cout << "server_credentials::private_key_for called, type: " << type
				  << ", context: " << context << std::endl;
		std::cout << "cert: " << cert.to_string() << std::endl;
		// return the private key associated with the leaf certificate,
		// in this case the one associated with "botan.randombit.net.crt"
		return m_key.get();
	}

	private:
	std::unique_ptr<Botan::Private_Key> m_key;
	std::vector<Botan::X509_Certificate> m_chain;
};


class server_callbacks : public Botan::TLS::Callbacks
{
	using data_handler = std::function<void(util::mutable_buffer&& buf)>;
	using ready_handler = std::function<void()>;

	public:
		server_callbacks(channel::ptr chan, data_handler dh, ready_handler rh)
			: m_channel{chan}, m_data_handler{dh}, m_ready_handler{rh}
		{}

		void
		tls_emit_data(const uint8_t data[], size_t size) override
		{
			// util::mutable_buffer dbuf{data, size};
			// std::cout << "server sending " << dbuf.size() << " bytes:" << std::endl;
			// dbuf.dump(std::cout);
			std::error_code write_err;
			m_channel->write(
					util::mutable_buffer{data, size},
					write_err,
					[&](channel::ptr const& chan, util::mutable_buffer&& buf, std::error_code err) {
						CHECK(!err);
						// CHECK(buf.as_string() == "reply to first payload");
						// acceptor_write_handler_did_execute = true;
					});
			CHECK(!write_err);
	}

	void
	tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size) override
	{
		// process full TLS record received by tls client, e.g.,
		// by passing it to the application
		m_data_handler(util::mutable_buffer{data, size});
		// std::cout << "server received message: " << std::string{reinterpret_cast<const char*>(data), size} << std::endl;
	}

	void
	tls_alert(Botan::TLS::Alert alert) override
	{
		if (alert.is_valid())
		{
			std::cout << "server received alert: " << alert.type_string();
			if (alert.is_fatal())
			{
				std::cout << " [fatal]";
			}
			std::cout << std::endl;
		}
		else
		{
			std::cout << "client received invalid alert" << std::endl;
		}
	}

	bool
	tls_session_established(const Botan::TLS::Session& session) override
	{
		std::cout << "server::tls_session_established called" << std::endl;
		m_ready_handler();
		return false;
	}
private:
	channel::ptr m_channel;
	data_handler m_data_handler;
	ready_handler m_ready_handler;
};



class clnt_context : public tls::context_base
{
public:	

	clnt_context() : m_rng{}, m_session_mgr{m_rng}, m_creds{}, m_policy{""}
	{
		m_policy.set("require_cert_revocation_info", "false");
		m_policy.set("signature_methods", "RSA");
	}

	virtual Botan::RNG&
	rng() override
	{
		return m_rng;
	}

	virtual Botan::Credentials_Manager&
	credentials() override
	{
		return m_creds;
	}

	virtual Botan::TLS::Session_Manager&
	session_manager() override
	{
		return m_session_mgr;
	}

	virtual Botan::TLS::Policy&
	policy() override
	{
		return m_policy;	
	}

private:
	Botan::AutoSeeded_RNG m_rng;
	Botan::TLS::Session_Manager_In_Memory m_session_mgr;
	client_credentials m_creds;
	Botan::TLS::Text_Policy m_policy;
};


class srvr_context : public tls::context_base
{
public:	

	srvr_context() : m_rng{}, m_session_mgr{m_rng}, m_creds{}, m_policy{}
	{
	}

	virtual Botan::RNG&
	rng() override
	{
		return m_rng;
	}

	virtual Botan::Credentials_Manager&
	credentials() override
	{
		return m_creds;
	}

	virtual Botan::TLS::Session_Manager&
	session_manager() override
	{
		return m_session_mgr;
	}

	virtual Botan::TLS::Policy&
	policy() override
	{
		return m_policy;	
	}

private:
	Botan::AutoSeeded_RNG m_rng;
	Botan::TLS::Session_Manager_In_Memory m_session_mgr;
	server_credentials m_creds;
	Botan::TLS::Strict_Policy m_policy;
};

#if 1
TEST_CASE("logicmill::laps::tls::options [ smoke ] { accessors and mutators }")
{
	tls::options client_opts;
	client_opts
		.trusted_cert_auth_dir_path("testdata/trusted");

	tls::options server_opts;
	server_opts
		.certificate_chain_filename("testdata/chain.crt")
		.private_key_filename("testdata/edge.key")
		.private_key_passwd("k8tylied");

	CHECK(client_opts.trusted_cert_auth_dir_path() == "testdata/trusted");
	CHECK(server_opts.private_key_filename() == "testdata/edge.key");
	CHECK(server_opts.private_key_passwd() == "k8tylied");
}

TEST_CASE("logicmill::laps::tls::context [ smoke ] { construction }")
{
	tls::options client_opts;
	client_opts
		.trusted_cert_auth_dir_path("testdata/trusted")
		.policy_overrides({{"require_cert_revocation_info", "false"}, {"signature_methods", "RSA"}});

	tls::options server_opts;
	server_opts
		.certificate_chain_filename("testdata/chain.crt")
		.private_key_filename("testdata/edge.key")
		.private_key_passwd("k8tylied");

	CHECK(client_opts.trusted_cert_auth_dir_path() == "testdata/trusted");
	CHECK(server_opts.private_key_filename() == "testdata/edge.key");
	CHECK(server_opts.private_key_passwd() == "k8tylied");

	auto client_context = util::make_shared<tls::context>(client_opts);
	auto server_context = util::make_shared<tls::context>(server_opts);

	// auto client_context = util::make_shared<clnt_context>();
	// auto server_context = util::make_shared<srvr_context>();

	using client_stack_type = assembly<laps::channel_anchor, laps::tls::client, laps::driver>;
	using server_stack_type = assembly<laps::channel_anchor, laps::tls::server, laps::driver>;
	
	// std::shared_ptr<client_stack_type> cstackp = std::make_shared<client_stack_type>(
	// 		laps::channel_anchor{}, laps::tls::client{client_context, "edgeofmagic"}, laps::driver{});
	// std::shared_ptr<server_stack_type> sstackp = std::make_shared<server_stack_type>(
	// 		laps::channel_anchor{}, laps::tls::server{server_context}, laps::driver{});

	std::shared_ptr<client_stack_type> cstackp;
	std::shared_ptr<server_stack_type> sstackp;

	bool acceptor_connection_handler_did_execute{false};
	bool server_request_handler_did_execute{false};
	bool control_handler_did_execute{false};
	bool channel_write_handler_did_execute{false};
	bool driver_read_handler{false};

	std::error_code     err;
	auto                lp = loop::create();
	async::ip::endpoint listen_ep{async::ip::address::v4_any(), 7001};
	auto                lstnr = lp->create_acceptor(
            async::options{listen_ep},
            err,
            [&](acceptor::ptr const& ls, channel::ptr const& chan, std::error_code err) {
                CHECK(!err);

                // sstackp = std::make_shared<server_stack_type>(laps::channel_anchor{chan}, laps::driver{});
				// sstackp = std::make_shared<server_stack_type>(
				// 		laps::channel_anchor{}, laps::tls::server{server_context}, laps::driver{});
				sstackp = std::make_shared<server_stack_type>();
				sstackp->layer<1>().config(server_context); 

				sstackp->top().on_error([=](std::error_code err) {
                    std::cout << "error event in server stack driver: " << err.message() << std::endl;
                });

                sstackp->top().on_writable([&]() {
                    std::cout << "writable event in server stack driver: "<< std::endl;
 
					sstackp->top().start_read([&](util::const_buffer&& buf) {
						CHECK(buf.as_string() == "first test payload");

						sstackp->top().write(util::mutable_buffer{"reply to first payload"});

						server_request_handler_did_execute = true;
					});
                });

				sstackp->bottom().connect(chan);

                acceptor_connection_handler_did_execute = true;
            });

	CHECK(!err);

	auto connect_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};

		cstackp = std::make_shared<client_stack_type>(
				laps::channel_anchor{}, laps::tls::client{client_context, "edgeofmagic"}, laps::driver{});

		cstackp->top().on_error(
				[=](std::error_code err) { std::cout << "error event in driver: " << err.message() << std::endl; });

		cstackp->top().on_writable([&]() {
			std::cout << "writable event in client driver" << std::endl;

			lp->dispatch([&]()
			{
				cstackp->top().start_read([&](util::const_buffer&& buf) {
					CHECK(buf.as_string() == "reply to first payload");
					driver_read_handler = true;
				});

				cstackp->top().write(util::mutable_buffer{"first test payload"});
			});
			control_handler_did_execute = true;
		});

		cstackp->bottom().connect(lp, async::options{connect_ep});
	});

	auto shutdown_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		std::error_code err;
		lp->stop(err);
		CHECK(!err);
	});

	connect_timer->start(std::chrono::milliseconds{1000}, err);
	CHECK(!err);

	shutdown_timer->start(std::chrono::milliseconds{3000}, err);
	CHECK(!err);
	CHECK(connect_timer->is_pending());

	connect_timer.reset();

	lp->run(err);
	CHECK(!err);

	lp->close(err);
	CHECK(acceptor_connection_handler_did_execute);
	CHECK(server_request_handler_did_execute);
	CHECK(driver_read_handler);
	CHECK(control_handler_did_execute);
	CHECK(!err);

}
#endif


TEST_CASE("logicmill::laps::tls [ smoke ] { client stack connect read write }")
{
	bool acceptor_connection_handler_did_execute{false};
	bool acceptor_read_handler_did_execute{false};
	// bool acceptor_write_handler_did_execute{false};
	bool on_writable_handler_did_execute{false};
	bool driver_read_handler{false};
	bool control_handler_did_execute{false};
	// bool channel_write_handler_did_execute{false};

	// using stack_type = assembly<laps::channel_anchor, laps::driver>;
	// std::shared_ptr<stack_type> stackp;


	auto client_context = util::make_shared<clnt_context>();
	auto server_context = util::make_shared<srvr_context>();

	using client_stack_type = assembly<laps::channel_anchor, laps::tls::client, laps::driver>;

	std::shared_ptr<client_stack_type> cstackp;

	std::shared_ptr<server_callbacks> callbacks;
	std::shared_ptr<Botan::TLS::Server> srvr;


	std::error_code     err;
	auto                lp = loop::create();
	async::ip::endpoint listen_ep{async::ip::address::v4_any(), 7001};
	auto                lstnr = lp->create_acceptor(
            async::options{listen_ep},
            err,
            [&](acceptor::ptr const& ls, channel::ptr const& chan, std::error_code err) {
                CHECK(!err);

				callbacks = std::make_shared<server_callbacks>(chan, 
				[&](util::mutable_buffer&& buf)
				{
					util::mutable_buffer reply{"reply to first payload"};
					srvr->send(reply.data(), reply.size());
					acceptor_read_handler_did_execute = true;
				},
				[&]()
				{
					on_writable_handler_did_execute = true;
					std::cout << "ready handler called in server" << std::endl;
				});

				srvr = std::make_shared<Botan::TLS::Server>(
					*callbacks,
					server_context->session_manager(),
					server_context->credentials(),
					server_context->policy(),
					server_context->rng());


                std::error_code read_err;
                chan->start_read(read_err, [&](channel::ptr const& cp, util::const_buffer&& buf, std::error_code err) {
                    CHECK(!err);
                    // CHECK(buf.as_string() == "first test payload");
					// std::cout << "server received " << buf.size() << " bytes:" << std::endl;
					// buf.dump(std::cout);
					srvr->received_data(buf.data(), buf.size());

                });
                CHECK(!read_err);
                acceptor_connection_handler_did_execute = true;
            });

	CHECK(!err);

	auto connect_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};

		cstackp = std::make_shared<client_stack_type>(
				laps::channel_anchor{}, laps::tls::client{client_context, "edgeofmagic"}, laps::driver{});

		cstackp->top().on_error(
				[=](std::error_code err) { std::cout << "error event in driver: " << err.message() << std::endl; });

		cstackp->top().on_writable([&]() {
			std::cout << "writable event in client driver" << std::endl;

			lp->dispatch([&]()
			{
				cstackp->top().start_read([&](util::const_buffer&& buf) {
					CHECK(buf.as_string() == "reply to first payload");
					std::cout << "client received plaintext reply: " << buf.as_string() << std::endl;
					driver_read_handler = true;
				});

				cstackp->top().write(util::mutable_buffer{"first test payload"});
			});
			control_handler_did_execute = true;
		});

		cstackp->bottom().connect(lp, async::options{connect_ep});
	});

	auto shutdown_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		std::error_code err;
		lp->stop(err);
		CHECK(!err);
	});

	connect_timer->start(std::chrono::milliseconds{1000}, err);
	CHECK(!err);

	shutdown_timer->start(std::chrono::milliseconds{5000}, err);
	CHECK(!err);
	CHECK(connect_timer->is_pending());

	connect_timer.reset();

	lp->run(err);
	CHECK(!err);

	lp->close(err);
	CHECK(acceptor_connection_handler_did_execute);
	CHECK(acceptor_read_handler_did_execute);
	// CHECK(acceptor_write_handler_did_execute);
	CHECK(on_writable_handler_did_execute);
	CHECK(driver_read_handler);
	CHECK(control_handler_did_execute);
	CHECK(!err);
}
