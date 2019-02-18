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

#ifndef LOGICMILL_ARMI_ASYNC_ADAPTER_H
#define LOGICMILL_ARMI_ASYNC_ADAPTER_H

// #include <chrono>
// #include <cstdint>
// #include <deque>
// #include <functional>
// #include <logicmill/armi/client_context_base.h>
// #include <logicmill/armi/server_context_base.h>
// #include <logicmill/armi/error.h>
// #include <logicmill/bstream/context.h>
// #include <logicmill/util/buffer.h>
// #include <system_error>

#include <logicmill/armi/transport.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>

namespace logicmill
{
namespace async
{

class client_channel_impl : public armi::transport::client_channel, public ENABLE_SHARED_FROM_THIS<client_channel_impl>
{
public:
	using ptr = SHARED_PTR_TYPE<client_channel_impl>;
	using connect_handler = std::function<void(armi::transport::client_channel::ptr, std::error_code)>;

	client_channel_impl(armi::client_context_base::ptr const& context, async::channel::ptr const& cp) 
	: m_context{context}, m_loop{cp->loop()}, m_channel{cp} 
	{
		start_read();
	}

	client_channel_impl(armi::client_context_base::ptr const& context, async::loop::ptr lp)
	: m_context{context}, m_loop{lp} 
	{}

	// async_client(client_context_base& context)
	// : m_context{context}
	// {}

	void
	connect(async::options const& opts, std::error_code& err, connect_handler handler)
	{
		err.clear();
		async::options options_override{opts};
		options_override.framing(true);
		m_loop->connect_channel(options_override, err, [=,handler{std::move(handler)}](async::channel::ptr chan, std::error_code err)
		{
			if (err)
			{
				handler(nullptr, err);
			}
			else
			{
				m_channel = chan;
				start_read();
				handler(shared_from_this(), std::error_code{});
			}
		});
	}

	virtual void
	send_request(
			std::uint64_t             request_id,
			std::chrono::milliseconds timeout,
			util::mutable_buffer&&    req,
			std::error_code&          err) override
	{
		err.clear();
		if (timeout.count() > 0)
		{
			std::error_code err;
			m_loop->schedule(timeout, err, [=]() {
				if (!m_context.expired())
				{
					m_context.lock()->cancel_request(request_id, std::make_error_code(std::errc::timed_out));
				}
			});
			if (err)
			{
				if (!m_context.expired())
				{
					m_context.lock()->cancel_request(request_id, err);
				}
				goto exit;
			}
		}
		m_channel->write(std::move(req), err);
		// m_channel->write(std::move(req), err, [=](channel::ptr const& chan, util::mutable_buffer&& buf, std::error_code err)
		// {
		// 	std::cout << "write complete";
		// 	if (err)
		// 	{
		// 		std::cout << ", err: " << err.message();
		// 	}
		// 	std::cout << std::endl;
		// });
	exit:
		return;
	}

	virtual void
	close(close_handler handler) override
	{
		if (handler)
		{
			m_channel->close([handler{std::move(handler)}](async::channel::ptr)
			{
				handler();
			});
		}
		else
		{
			m_channel->close();
		}
		m_channel.reset();
	}

	virtual void
	close() override
	{
		m_channel->close();
		m_channel.reset();
	}

private:

	void start_read()
	{
		std::error_code err;
		m_channel->start_read(
				err,
				[=](async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err) {
					if (!m_context.expired())
					{
						auto cp = m_context.lock();
						if (err)
						{
							cp->transport_error(err);
						}
						else
						{
							cp->handle_reply(std::move(buf));
						}
					}
				});
		if (err)
		{
			if (!m_context.expired())
			{
				m_context.lock()->transport_error(err);
			}
		}
	}

	armi::client_context_base::wptr m_context;
	async::loop::ptr     m_loop;
	async::channel::ptr  m_channel;
};


class server_channel_impl : public armi::transport::server_channel, public ENABLE_SHARED_FROM_THIS<server_channel_impl>
{
public:
	using ptr = SHARED_PTR_TYPE<server_channel_impl>;

	server_channel_impl(async::channel::ptr chan) : m_channel{chan} {}

	virtual void
	send_reply(util::mutable_buffer&& buf) override
	{
		m_channel->write(std::move(buf));
	}

private:
	async::channel::ptr	m_channel;
};



class server_impl : public ENABLE_SHARED_FROM_THIS<server_impl>
{
public:

	using ptr = SHARED_PTR_TYPE<server_impl>;
	using channel_error_handler = std::function<void(server_impl::ptr const& srvr, async::channel::ptr const& chan, std::error_code err)>;
	using server_error_handler = std::function<void(server_impl::ptr const& srvr, std::error_code err)>;
	using server_close_handler = std::function<void()>;
	using channel_close_handler = std::function<void(async::channel::ptr const& ptr)>;

	bool
	close(async::channel::ptr const& chan)
	{
			bool deferred_close = chan->close([=](async::channel::ptr const& c) {
				m_open_channels.erase(c);
				if (m_on_channel_close)
				{
					m_on_channel_close(c);
				}
				if (m_open_channels.empty() && !m_acceptor)
				{
					cleanup();
				}
			});
			return deferred_close;
	}

	void
	close()
	{
		really_close();
	}


private:

	void
	cleanup()
	{
		if (m_on_server_close)
		{
			m_on_server_close();
			m_on_server_close = nullptr;
		}
		m_on_server_error = nullptr;
		m_on_channel_close = nullptr;
		m_on_channel_error = nullptr;
	}

	bool
	really_close()
	{
		bool result{false};
		if (m_acceptor)
		{
			bool acceptor_did_close = m_acceptor->close([=](async::acceptor::ptr const& lp) {
				m_acceptor.reset();
				if (m_open_channels.empty())
				{
					cleanup();
				}
			});
			if (acceptor_did_close)
			{
				result = true;
			}
			else
			{
				m_acceptor.reset();
			}
		}
		auto it = m_open_channels.begin();
		while (it != m_open_channels.end())
		{
			// bool channel_did_close = (*it)->close([=](async::channel::ptr const& c) {
			// 	m_open_channels.erase(c);
			// 	if (m_open_channels.empty() && !m_acceptor)
			// 	{
			// 		cleanup();
			// 	}
			// });
			bool channel_did_close = close(*it);
			if (channel_did_close)
			{
				result = true;
				++it;
			}
			else
			{
				it = m_open_channels.erase(it); // whack if already closed/closing
			}
		}
		if (!result)    // there was no deferred closing action; wipe everything
		{
			assert(!m_acceptor);
			assert(m_open_channels.empty());
			cleanup();
			// m_acceptor.reset();         // probably unnecessary, but whatever
			// m_open_channels.clear();    // ditto
			// m_on_close = nullptr;
		}
		return result;
	}	
public:

	server_impl(armi::server_context_base::ptr const& server_context, async::loop::ptr const& lp)
	:
	m_context{server_context},
	m_loop{lp}
	{
	}

	void
	on_server_error(server_error_handler handler)
	{
		m_on_server_error = std::move(handler);
	}

	void
	on_channel_error(channel_error_handler handler)
	{
		m_on_channel_error = std::move(handler);
	}

	void
	on_server_close(server_close_handler handler)
	{
		m_on_server_close = std::move(handler);
	}

	void
	on_channel_close(channel_close_handler handler)
	{
		m_on_channel_close = std::move(handler);
	}

	void
	server_error_default_handler(std::error_code err)
	{
		close();
	}

	void
	channel_error_default_handler(async::channel::ptr const& chan, std::error_code err)
	{
		close(chan);
	}

	void
	bind(async::options const& opts, std::error_code& err)
	{
		err.clear();
		async::options opts_override{opts};
		opts_override.framing(true);
		m_acceptor = m_loop->create_acceptor(
				opts_override, err, [=,self{this->shared_from_this()}](async::acceptor::ptr const& sp, async::channel::ptr const& chan, std::error_code err) {
					if (err)
					{
						if (self->m_on_server_error)
						{
							self->m_on_server_error(self, err);
						}
						else
						{
							self->server_error_default_handler(err);
						}
					}
					else
					{
						m_open_channels.insert(chan);
						chan->start_read(
								err,
								[=,self{this->shared_from_this()}](async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err) {
									if (err)
									{
										if (self->m_on_channel_error)
										{
											self->m_on_channel_error(self, chan, err);
										}
										else
										{
											self->channel_error_default_handler(chan, err);
										}
									}
									else
									{
										auto server_channel = MAKE_SHARED<server_channel_impl>(chan);
										bstream::imbstream is{std::move(buf), m_context->stream_context()};
										m_context->handle_request(is, server_channel);
									}
								});
					}
				});
	}

private:
	armi::server_context_base::ptr          m_context;
	async::loop::ptr                        m_loop;
	async::acceptor::ptr                    m_acceptor;
	channel_error_handler                   m_on_channel_error;
	server_error_handler                    m_on_server_error;
	channel_close_handler                   m_on_channel_close;
	server_close_handler                    m_on_server_close;
	std::unordered_set<async::channel::ptr> m_open_channels;
};

}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_ASYNC_ADAPTER_H
