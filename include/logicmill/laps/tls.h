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

#ifndef LOGICMILL_LAPS_TLS_H
#define LOGICMILL_LAPS_TLS_H

#include <botan/auto_rng.h>
#include <botan/data_src.h>
#include <botan/pkcs8.h>
#include <botan/tls_callbacks.h>
#include <botan/tls_client.h>
#include <botan/tls_policy.h>
#include <botan/tls_server.h>
#include <botan/tls_session_manager.h>
#include <fstream>
#include <list>
#include <logicmill/async/event_flow.h>
#include <logicmill/async/loop.h>
#include <logicmill/async/options.h>
#include <logicmill/laps/types.h>
#include <logicmill/util/shared_ptr.h>

namespace logicmill
{
namespace laps
{
namespace tls
{

// TODO: catch exceptions and translated to error codes

enum class errc
{
	ok                     = 0,
	invalid_argument       = 1,
	unsupported_argument   = 2,
	invalid_state          = 3,
	lookup_error           = 4,
	internal_error         = 5,
	invalid_key_length     = 6,
	invalid_iv_length      = 7,
	prng_unseeded          = 8,
	policy_violation       = 9,
	algorithm_not_found    = 10,
	no_provider_found      = 11,
	provider_not_found     = 12,
	invalid_algorithm_name = 13,
	encoding_error         = 14,
	decoding_error         = 15,
	integrity_failure      = 16,
	invalid_oid            = 17,
	stream_io_error        = 18,
	self_test_failure      = 19,
	not_implemented        = 20,
	unknown                = 21
};

std::error_category const&
error_category() noexcept;

std::error_condition
make_error_condition(errc e);

std::error_code
make_error_code(errc e);

class options
{
public:
	options&
	trusted_cert_auth_dir_path(std::string const& dir)
	{
		m_cert_auth_dir_path = dir;
		return *this;
	}

	std::string const&
	trusted_cert_auth_dir_path() const
	{
		return m_cert_auth_dir_path;
	}

	options&
	certificate_chain_filename(std::string const& cert_filename)
	{
		m_cert_chain_filename = cert_filename;
		return *this;
	}

	std::string const&
	certificate_chain_filename() const
	{
		return m_cert_chain_filename;
	}

	options&
	certificate_chain_filenames(std::vector<std::string> const& filenames)
	{
		m_cert_chain_filenames = filenames;
		return *this;
	}

	options&
	certificate_chain_filenames(std::vector<std::string>&& filenames)
	{
		m_cert_chain_filenames = std::move(filenames);
		return *this;
	}

	std::vector<std::string> const&
	certificate_chain_filenames() const
	{
		return m_cert_chain_filenames;
	}

	options&
	private_key_filename(std::string const& key_filename)
	{
		m_key_filename = key_filename;
		return *this;
	}

	std::string const&
	private_key_filename() const
	{
		return m_key_filename;
	}

	options&
	private_key_passwd(std::string const& key_passwd)
	{
		m_key_passwd = key_passwd;
		return *this;
	}

	std::string const&
	private_key_passwd() const
	{
		return m_key_passwd;
	}

	options&
	policy_overrides(std::vector<std::pair<std::string, std::string>> const& overrides)
	{
		m_policy_overrides = overrides;
		return *this;
	}

	options&
	policy_overrides(std::vector<std::pair<std::string, std::string>>&& overrides)
	{
		m_policy_overrides = std::move(overrides);
		return *this;
	}

	std::vector<std::pair<std::string, std::string>> const&
	policy_overrides() const
	{
		return m_policy_overrides;
	}

private:
	std::string                                      m_key_filename;
	std::string                                      m_key_passwd;
	std::string                                      m_cert_auth_dir_path;
	std::string                                      m_cert_chain_filename;
	std::vector<std::string>                         m_cert_chain_filenames;
	std::vector<std::pair<std::string, std::string>> m_policy_overrides;
};

class credentials_manager : public Botan::Credentials_Manager
{
public:
	credentials_manager(options const& opts)
	{
		if (!opts.trusted_cert_auth_dir_path().empty())
		{
			std::cout << "credentials_manager creating certificate store from directory "
					  << opts.trusted_cert_auth_dir_path() << std::endl;
			m_cert_store = std::make_unique<Botan::Certificate_Store_In_Memory>(opts.trusted_cert_auth_dir_path());
		}

		if (!opts.certificate_chain_filename().empty())
		{
			std::cout << "credentials_manager creating certificate chain from file "
					  << opts.certificate_chain_filename() << std::endl;
			load_cert_chain_from_file(opts.certificate_chain_filename());
		}
		else if (opts.certificate_chain_filenames().size() > 0)
		{
			for (auto& filename : opts.certificate_chain_filenames())
			{
				m_cert_chain.emplace_back(Botan::X509_Certificate{filename});
			}
		}

		if (!opts.private_key_filename().empty() && !opts.private_key_passwd().empty())
		{
			std::cout << "credentials_manager loading private key from file " << opts.private_key_filename()
					  << " with password " << opts.private_key_passwd() << std::endl;
			std::ifstream            keyin{opts.private_key_filename()};
			Botan::DataSource_Stream keysource{keyin};
			m_private_key = Botan::PKCS8::load_key(keysource, opts.private_key_passwd());
		}
	}

	std::vector<Botan::Certificate_Store*>
	trusted_certificate_authorities(const std::string& type, const std::string& context) override
	{
		std::vector<Botan::Certificate_Store*> result;
		if (m_cert_store)
		{
			result.push_back(m_cert_store.get());
		}
		return result;
	}

	std::vector<Botan::X509_Certificate>
	cert_chain(const std::vector<std::string>& cert_key_types, const std::string& type, const std::string& context)
			override
	{
		std::vector<Botan::X509_Certificate> result;
		std::cout << "cert_chain called, type: " << type << ", context: " << context << std::endl;
		for (auto& key_type : cert_key_types)
		{
			std::cout << "cert_key_type: " << key_type << std::endl;
			if (key_type == "RSA")
			{
				result = m_cert_chain;
			}
		}
		return result;
	}

	Botan::Private_Key*
	private_key_for(const Botan::X509_Certificate& cert, const std::string& type, const std::string& context) override
	{
		Botan::Private_Key* result{nullptr};

		if (m_private_key && m_cert_chain.size() > 0 && m_cert_chain.front() == cert)
		{
			result = m_private_key.get();
		}

		return result;
	}

private:
	void
	load_cert_chain_from_file(std::string const& filename)
	{
		std::ifstream            in{filename};
		Botan::DataSource_Stream src{in};

		while (!src.end_of_data())
		{
			try
			{
				Botan::X509_Certificate cert{src};
				m_cert_chain.emplace_back(std::move(cert));
			}
			catch (const Botan::Decoding_Error& e)
			{}
		}
	}

	std::unique_ptr<Botan::Certificate_Store_In_Memory> m_cert_store;
	std::vector<Botan::X509_Certificate>                m_cert_chain;
	std::unique_ptr<Botan::Private_Key>                 m_private_key;
};

class context_base
{
public:
	virtual Botan::RNG&
	rng() = 0;

	virtual Botan::Credentials_Manager&
	credentials()
			= 0;

	virtual Botan::TLS::Session_Manager&
	session_manager()
			= 0;

	virtual Botan::TLS::Policy&
	policy() = 0;
};

class context : public context_base
{
public:
	using ptr = util::shared_ptr<context>;

	context(options const& opts) : m_cred_mgr{opts}, m_session_mgr{m_rng}, m_policy{""}
	{
		if (opts.policy_overrides().size() > 0)
		{
			for (auto& keyval : opts.policy_overrides())
			{
				m_policy.set(keyval.first, keyval.second);
			}
		}
	}

	Botan::RNG&
	rng() override
	{
		return m_rng;
	}

	Botan::Credentials_Manager&
	credentials() override
	{
		return m_cred_mgr;
	}

	Botan::TLS::Session_Manager&
	session_manager() override
	{
		return m_session_mgr;
	}

	Botan::TLS::Policy&
	policy() override
	{
		return m_policy;
	}

private:
	Botan::AutoSeeded_RNG                 m_rng;
	credentials_manager                   m_cred_mgr;
	Botan::TLS::Session_Manager_In_Memory m_session_mgr;
	Botan::TLS::Text_Policy               m_policy;
};


class client : public Botan::TLS::Callbacks
{
public:
	static constexpr bool reverse_order = boost::endian::order::native != boost::endian::order::big;

	client() : m_top{this}, m_bottom{this}, m_context{}, m_server_info{}, m_tls_client{} {}

	client(util::shared_ptr<context_base> cntxt)
		: m_top{this}, m_bottom{this}, m_context{cntxt}, m_server_info{}, m_tls_client{}
	{}

	client(util::shared_ptr<context_base> cntxt, std::string const& server_name, std::uint16_t port = 0)
		: m_top{this},
		  m_bottom{this},
		  m_context{cntxt},
		  m_server_info{std::make_unique<Botan::TLS::Server_Information>(server_name, port)},
		  m_tls_client{}
	{}

	client(client const& rhs) = delete;

	client(client&& rhs)
		: m_top{this},
		  m_bottom{this},
		  m_context{std::move(rhs.m_context)},
		  m_server_info{std::move(rhs.m_server_info)},
		  m_tls_client{std::move(rhs.m_tls_client)}
	{
		m_top.m_is_writable    = rhs.m_top.m_is_writable;
		m_top.m_is_reading     = rhs.m_top.m_is_reading;
		m_bottom.m_is_writable = rhs.m_bottom.m_is_writable;
		m_bottom.m_is_reading  = rhs.m_bottom.m_is_reading;
	}

	void
	config(util::shared_ptr<context_base> cntxt, std::string const& server_name, std::uint16_t port = 0)
	{
		m_context     = cntxt;
		m_server_info = std::make_unique<Botan::TLS::Server_Information>(server_name, port);
	}

	void
	tls_emit_data(const std::uint8_t data[], std::size_t size) override
	{
		// std::cout << "tls client sending " << size << " bytes:" << std::endl;
		// util::mutable_buffer{data, size}.dump(std::cout);
		m_bottom.emit<mutable_buffer_event>(util::mutable_buffer{data, size});
	}

	void
	tls_record_received(std::uint64_t seq_no, const std::uint8_t data[], std::size_t size) override
	{
		m_top.emit<const_buffer_event>(util::const_buffer{data, size});
	}

	void
	tls_alert(Botan::TLS::Alert alert) override
	{
		std::error_code err{static_cast<int>(alert.type()), tls::error_category()};
		m_top.emit<error_event>(err);
	}

	bool
	tls_session_established(const Botan::TLS::Session& session) override
	{
		std::cout << "client tls_session_established" << std::endl;
		m_top.propagate_start();
		return false;
	}

	friend class top;
	friend class bottom;

	class top : public flow::stackable<stream_duplex_top, top>, public face<client, top>
	{
	public:
		friend class logicmill::laps::tls::client;
		using connector_base = flow::stackable<stream_duplex_top, top>;
		using flow::emitter<const_buffer_event>::emit;
		using flow::emitter<control_event>::emit;
		using flow::emitter<error_event>::emit;
		using connector_base::get_surface;

		top(client* owner) : face<client, top>{owner} {}

		void
		on(mutable_data_event, std::deque<util::mutable_buffer>&& bufs)
		{
			for (auto& buf : bufs)
			{
				owner()->m_tls_client->send(buf.data(), buf.size());
			}
		}

		void
		on(mutable_buffer_event, util::mutable_buffer&& buf)
		{
			std::cout << "sending tls client application message: " << buf.as_string() << std::endl;
			owner()->m_tls_client->send(buf.data(), buf.size());
		}

		void
		on(control_event, control_state state)
		{
			if (state == control_state::start)
			{
				set_writable(true);
				owner()->m_bottom.propagate_start();
			}
			else if (state == control_state::stop)
			{
				set_writable(false);
				owner()->m_bottom.propagate_stop();
			}
		}

		void
		on(error_event, std::error_code err)
		{
			assert(false);    // shouldn't happen
							  // m_owner->m_bottom.emit<error_event>(err);
		}
	};

	class bottom : public flow::stackable<stream_duplex_bottom, bottom>, public face<client, bottom>
	{
	public:
		friend class logicmill::laps::tls::client;
		using connector_base = flow::stackable<stream_duplex_bottom, bottom>;
		using emitter<mutable_data_event>::emit;
		using emitter<mutable_buffer_event>::emit;
		using emitter<control_event>::emit;
		using emitter<error_event>::emit;
		using connector_base::get_surface;

		bottom(client* owner) : face<client, bottom>{owner} {}

		void
		on(const_buffer_event, util::const_buffer&& buf)
		{
			// std::cout << "client received " << buf.size() << " bytes:" << std::endl;
			// buf.dump(std::cout);
			owner()->m_tls_client->received_data(buf.data(), buf.size());
		}

		void
		on(control_event, control_state s)
		{
			if (s == control_state::start)
			{
				set_writable(true);
				if (!owner()->m_tls_client)
				{
					owner()->create_client();
					propagate_start();
				}
				else
				{
					owner()->m_top.propagate_start();
				}
			}
			else if (s == control_state::stop)
			{
				set_writable(false);
				owner()->m_top.propagate_stop();
			}
		}

		void
		on(error_event, std::error_code err)
		{
			owner()->m_top.emit<error_event>(err);
		}
	};

	void
	create_client()
	{
		assert(m_context);
		if (!m_server_info)
		{
			m_server_info = std::make_unique<Botan::TLS::Server_Information>();
		}
		m_tls_client = std::make_unique<Botan::TLS::Client>(
				*static_cast<Botan::TLS::Callbacks*>(this),
				m_context->session_manager(),
				m_context->credentials(),
				m_context->policy(),
				m_context->rng(),
				*m_server_info);
	}

public:
	stream_duplex_bottom&
	get_bottom()
	{
		return m_bottom.get_surface<stream_duplex_bottom>();
	}

	stream_duplex_top&
	get_top()
	{
		return m_top.get_surface<stream_duplex_top>();
	}

private:
	top                                             m_top;
	bottom                                          m_bottom;
	util::shared_ptr<context_base>                  m_context;
	std::unique_ptr<Botan::TLS::Server_Information> m_server_info;
	std::unique_ptr<Botan::TLS::Client>             m_tls_client;
};


class server : public Botan::TLS::Callbacks
{
public:
	static constexpr bool reverse_order = boost::endian::order::native != boost::endian::order::big;

	server() : m_top{this}, m_bottom{this}, m_context{}, m_tls_server{} {}

	server(util::shared_ptr<context_base> cntxt) : m_top{this}, m_bottom{this}, m_context{cntxt}, m_tls_server{} {}

	server(server const& rhs) = delete;

	server(server&& rhs)
		: m_top{this}, m_bottom{this}, m_context{std::move(rhs.m_context)}, m_tls_server{std::move(rhs.m_tls_server)}
	{
		m_top.m_is_writable    = rhs.m_top.m_is_writable;
		m_top.m_is_reading     = rhs.m_top.m_is_reading;
		m_bottom.m_is_writable = rhs.m_bottom.m_is_writable;
		m_bottom.m_is_reading  = rhs.m_bottom.m_is_reading;
	}

	void
	config(util::shared_ptr<context_base> cntxt)
	{
		m_context = cntxt;
	}

	void
	tls_emit_data(const std::uint8_t data[], std::size_t size) override
	{
		m_bottom.emit<mutable_buffer_event>(util::mutable_buffer{data, size});
	}

	void
	tls_record_received(std::uint64_t seq_no, const std::uint8_t data[], std::size_t size) override
	{
		m_top.emit<const_buffer_event>(util::const_buffer{data, size});
	}

	void
	tls_alert(Botan::TLS::Alert alert) override
	{
		std::error_code err{static_cast<int>(alert.type()), tls::error_category()};
		m_top.emit<error_event>(err);
	}

	bool
	tls_session_established(const Botan::TLS::Session& session) override
	{
		m_top.propagate_start();
		return false;
	}

	friend class top;
	friend class bottom;

	class top : public flow::stackable<stream_duplex_top, top>, public face<server, top>
	{
	public:
		friend class logicmill::laps::tls::server;
		using connector_base = flow::stackable<stream_duplex_top, top>;
		using flow::emitter<const_buffer_event>::emit;
		using flow::emitter<control_event>::emit;
		using flow::emitter<error_event>::emit;
		using connector_base::get_surface;

		top(server* owner) : face<server, top>{owner} {}

		void
		on(mutable_data_event, std::deque<util::mutable_buffer>&& bufs)
		{
			for (auto& buf : bufs)
			{
				owner()->m_tls_server->send(buf.data(), buf.size());
			}
		}

		void
		on(mutable_buffer_event, util::mutable_buffer&& buf)
		{
			owner()->m_tls_server->send(buf.data(), buf.size());
		}

		void
		on(control_event, control_state state)
		{
			if (state == control_state::start)
			{
				set_writable(true);
				owner()->m_bottom.propagate_start();
			}
			else if (state == control_state::stop)
			{
				set_writable(false);
				owner()->m_bottom.propagate_stop();
			}
		}

		void
		on(error_event, std::error_code err)
		{
			assert(false);
		}
	};

	class bottom : public flow::stackable<stream_duplex_bottom, bottom>, public face<server, bottom>
	{
	public:
		friend class logicmill::laps::tls::server;
		using connector_base = flow::stackable<stream_duplex_bottom, bottom>;
		using emitter<mutable_data_event>::emit;
		using emitter<mutable_buffer_event>::emit;
		using emitter<control_event>::emit;
		using emitter<error_event>::emit;
		using connector_base::get_surface;

		bottom(server* owner) : face<server, bottom>{owner} {}

		void
		on(const_buffer_event, util::const_buffer&& buf)
		{
			// std::cout << "tls server receiving " << buf.size() << " bytes:" << std::endl;
			// buf.dump(std::cout);
			owner()->m_tls_server->received_data(buf.data(), buf.size());
		}

		void
		on(control_event, control_state s)
		{
			if (s == control_state::start)
			{
				set_writable(true);
				if (!owner()->m_tls_server)
				{
					owner()->create_server();
					propagate_start();
				}
				else
				{
					owner()->m_top.propagate_start();
				}
			}
			else if (s == control_state::stop)
			{
				set_writable(false);
				owner()->m_top.propagate_stop();
			}
		}

		void
		on(error_event, std::error_code err)
		{
			owner()->m_top.emit<error_event>(err);
		}
	};

	void
	create_server()
	{
		assert(m_context);
		m_tls_server = std::make_unique<Botan::TLS::Server>(
				*static_cast<Botan::TLS::Callbacks*>(this),
				m_context->session_manager(),
				m_context->credentials(),
				m_context->policy(),
				m_context->rng());
	}

public:
	stream_duplex_bottom&
	get_bottom()
	{
		return m_bottom.get_surface<stream_duplex_bottom>();
	}

	stream_duplex_top&
	get_top()
	{
		return m_top.get_surface<stream_duplex_top>();
	}

private:
	top                                 m_top;
	bottom                              m_bottom;
	util::shared_ptr<context_base>      m_context;
	std::unique_ptr<Botan::TLS::Server> m_tls_server;
};

}    // namespace tls
}    // namespace laps
}    // namespace logicmill

namespace std
{

template<>
struct is_error_condition_enum<logicmill::laps::tls::errc> : public true_type
{};

}    // namespace std

#endif    // LOGICMILL_LAPS_TLS_H