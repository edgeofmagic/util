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

#ifndef LOGICMILL_ARMI_ADAPTER_ASYNC_SERVER_H
#define LOGICMILL_ARMI_ADAPTER_ASYNC_SERVER_H

#include <functional>
#include <logicmill/armi/adapters/async/channel_manager.h>
#include <logicmill/armi/server_context.h>
#include <logicmill/armi/transport.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/bstream/imbstream.h>
#include <logicmill/util/promise.h>

namespace logicmill
{
namespace async
{

class server_adapter_base : public armi::transport::server, public channel_manager
{
public:
	using channel_error_handler = std::function<void(armi::channel_id_type, std::error_code)>;
	using accept_error_handler  = std::function<void(std::error_code err)>;
	using server_close_handler  = std::function<void()>;
	using channel_close_handler = std::function<void(armi::channel_id_type)>;
	using connection_handler    = std::function<void(armi::channel_id_type)>;

protected:
	armi::server_context_base& m_context_base;
	async::loop::ptr           m_loop;
	async::acceptor::ptr       m_acceptor;
	bool                       m_is_server_closing;
	channel_close_handler      m_on_channel_close;
	channel_error_handler      m_on_channel_error;
	accept_error_handler       m_on_accept_error;
	server_close_handler       m_on_server_close;
	connection_handler         m_on_channel_connect;

public:
	server_adapter_base(async::loop::ptr lp, armi::server_context_base& context)
		: m_context_base{context}, m_loop{lp}, m_is_server_closing{false}
	{}

	virtual ~server_adapter_base() {}

	server_adapter_base&
	on_channel_close(channel_close_handler handler)
	{
		m_on_channel_close = std::move(handler);
		return *this;
	}

	server_adapter_base&
	on_channel_error(channel_error_handler handler)
	{
		m_on_channel_error = std::move(handler);
		return *this;
	}

	server_adapter_base&
	on_channel_connect(connection_handler handler)
	{
		m_on_channel_connect = std::move(handler);
		return *this;
	}

	server_adapter_base&
	on_server_close(server_close_handler handler)
	{
		m_on_server_close = std::move(handler);
		return *this;
	}

	server_adapter_base&
	on_accept_error(accept_error_handler handler)
	{
		m_on_accept_error = std::move(handler);
		return *this;
	}

	void
	close()
	{
		really_close();
	}

	virtual bool
	is_valid_channel(armi::channel_id_type channel_id) override;

	virtual void
	close(armi::channel_id_type channel_id) override
	{
		really_close(channel_id);
	}

	virtual void
	send_reply(armi::channel_id_type channel_id, util::mutable_buffer&& req) override;

protected:
	void
	accept_error(std::error_code err);

	void
	channel_error(armi::channel_id_type channel_id, std::error_code err);

	void
	default_channel_error_handler(armi::channel_id_type channel_id, std::error_code err)
	{
		close(channel_id);
	}

	void
	default_accept_error_handler(std::error_code err)
	{
		close();
	}

	virtual void
	cleanup()
			= 0;

	void
	cleanup_base();

	void
	really_close(armi::channel_id_type channel_id);

	void
	really_close();
};

template<class T>
class server_adapter : public server_adapter_base
{
public:
	using server_context_type = T;
	using target_ptr_type     = std::shared_ptr<typename server_context_type::target_type>;
	using request_handler     = std::function<target_ptr_type(armi::channel_id_type)>;

private:
	server_context_type m_context;
	request_handler     m_on_request;

public:
	server_adapter(async::loop::ptr lp) : server_adapter_base{lp, m_context}, m_context{*this} {}

	~server_adapter()
	{
		close();
	}

	virtual void
	cleanup() override
	{
		m_on_request = nullptr;
		cleanup_base();
	}

	server_adapter&
	on_request(request_handler handler)
	{
		m_on_request = std::move(handler);
		return *this;
	}

	void
	bind(async::options const& opts, std::error_code& err)
	{
		err.clear();
		async::options opts_override{opts};
		opts_override.framing(true);
		m_acceptor = m_loop->create_acceptor(
				opts_override,
				err,
				[=](async::acceptor::ptr const& sp, async::channel::ptr const& chan, std::error_code err) {
					if (err)
						accept_error(err);
					else
					{
						auto channel_id = new_channel(chan);
						if (m_on_channel_connect)
							m_on_channel_connect(channel_id);
						chan->start_read(
								err,
								[=](async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err) {
									if (err)
										channel_error(channel_id, err);
									else
									{
										if (!m_on_request)
											accept_error(make_error_code(armi::errc::no_target_provided));
										else
										{
											// TODO: check to see if channel_id is still in channel_map first
											bstream::imbstream is{std::move(buf), m_context.stream_context()};
											m_context.handle_request(channel_id, is, m_on_request(channel_id));
										}
									}
								});
					}
				});
	}
};

}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_ADAPTER_ASYNC_SERVER_H
