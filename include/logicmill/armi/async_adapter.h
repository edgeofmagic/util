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
#include <logicmill/armi/client_context.h>

#include <functional>

namespace logicmill
{
namespace async
{

template<class T>
class client_adaptor : public armi::transport::client
{
public:
	using client_context_type    = T;
	using channel_error_handler = std::function<void(armi::channel_id_type channel_id, std::error_code err)>;
	using client_connect_handler = std::function<void(typename client_context_type::client_channel, std::error_code)>;
	using channel_map_type       = std::unordered_map<armi::channel_id_type, async::channel::ptr>;

	/* public interface for client_adapter */

	client_adaptor(loop::ptr lp) : m_loop{lp}, m_context{*this}, m_next_channel_id{1}, m_is_closing{false} 
	{
		m_on_channel_read_error = [=](armi::channel_id_type channel_id, std::error_code err)
		{
			close(channel_id, err);
		};
	}

	void
	connect(async::options const& opts, std::error_code& err, client_connect_handler handler)
	{
		err.clear();
		async::options options_override{opts};
		options_override.framing(true);
		m_loop->connect_channel(
				options_override, err, [=, handler{std::move(handler)}](async::channel::ptr chan, std::error_code err) {
					if (err)
					{
						handler(m_context.create_channel(0), err);
					}
					else
					{
						auto id = get_next_channel_id();
						m_channel_map.emplace(id, chan);
						chan->start_read(err, [=] (async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err)
						{
							if (err)
							{
								if (m_on_channel_read_error)
								{
									m_on_channel_read_error(id, err);
								}
								else
								{
									close(id, err);
								}
							}
							else
							{
								m_context.handle_reply(std::move(buf));
							}
						});
						handler(m_context.create_channel(id), std::error_code{});
					}
				});
	}

	void
	close(armi::channel_id_type channel_id, std::error_code err)
	{
		if (is_valid_channel(channel_id))
		{
			m_loop->dispatch([=]()
			{
				really_close(channel_id, err);
			});
		}
	}

	void
	close(std::error_code err)
	{
		if (!m_is_closing)
		{
			m_is_closing = true;
			m_loop->dispatch([=]()
			{
				really_close(make_error_code(armi::errc::context_closed));
			});
		}
	}

	void
	on_channel_read_error(channel_error_handler handler)
	{
		m_on_channel_read_error = std::move(handler);
	}

	virtual bool
	is_valid_channel(armi::channel_id_type id) override
	{
		bool result{false};
		if (id != 0)
		{
			auto it = m_channel_map.find(id);
			if (it != m_channel_map.end())
			{
				if (it->second && !it->second->is_closing())
				{
					result = true;
				}
			}
		}
		return result;
	}

	virtual void
	close(armi::channel_id_type channel_id) override
	{
		close(channel_id, make_error_code(armi::errc::channel_closed));
	}

	virtual void
	send_request(
			armi::channel_id_type     channel_id,
			armi::request_id_type     request_id,
			std::chrono::milliseconds timeout,
			util::mutable_buffer&&    req) override
	{
		std::error_code err;

		auto it = m_channel_map.find(channel_id);
		if (it == m_channel_map.end())
		{
			err = make_error_code(armi::errc::invalid_channel_id);
			goto exit;
		}

		if (timeout.count() > 0)
		{
			m_loop->schedule(timeout, err, [=]() {
				m_context.cancel_request(request_id, make_error_code(std::errc::timed_out));
			});
			if (err)
				goto exit;
		}

		it->second->write(std::move(req), err);
		if (err)
			goto exit;

		// it->second->write(std::move(req), err, [=](channel::ptr const& chan, util::mutable_buffer&& buf, std::error_code err)
		// {
		// 	std::cout << "write complete";
		// 	if (err)
		// 	{
		// 		std::cout << ", err: " << err.message();
		// 	}
		// 	std::cout << std::endl;
		// });

	exit:
		if (err)
			m_loop->dispatch([=]()
			{
				m_context.cancel_request(request_id, err);
			});
		return;
	}

private:
	void
	really_close(armi::channel_id_type channel_id, std::error_code err)
	{
		if (channel_id != 0)
		{
			auto it = m_channel_map.find(channel_id);
			if (it != m_channel_map.end())
			{
				auto chan = it->second;
				m_channel_map.erase(it);
				if (!chan->is_closing())
				{
					chan->close();
				}
				m_loop->dispatch([=]()
				{
					m_context.cancel_channel_requests(channel_id, err);
				});
			}
		}
	}

	void
	really_close(std::error_code err)
	{
		auto it = m_channel_map.begin();
		while(it != m_channel_map.end())
		{
			if (!it->second->is_closing())
			{
				it->second->close();
			}
			it = m_channel_map.erase(it);
		}
		m_loop->dispatch([=]()
		{
			m_context.cancel_all_requests(err);
		});
	}

	armi::channel_id_type
	get_next_channel_id()
	{
		return m_next_channel_id++;
	}

	async::loop::ptr      m_loop;
	client_context_type   m_context;
	channel_map_type      m_channel_map;
	armi::channel_id_type m_next_channel_id;
	channel_error_handler m_on_channel_read_error;
	bool m_is_closing;
};

template<class T>
class server_adaptor : public armi::transport::server
{
public:
	using server_context_type      = T;
	using target_ptr_type          = std::shared_ptr<typename server_context_type::target_type>;
	using channel_error_handler    = std::function<void(armi::channel_id_type, std::error_code)>;
	using accept_error_handler     = std::function<void(std::error_code err)>;
	using server_close_handler     = std::function<void()>;
	using channel_close_handler    = std::function<void(armi::channel_id_type)>;
	using request_handler          = std::function<target_ptr_type(armi::channel_id_type)>;
	using connection_handler       = std::function<void(armi::channel_id_type)>;
	using channel_map_type         = std::unordered_map<armi::channel_id_type, async::channel::ptr>;
	using channel_map_iterator     = channel_map_type::iterator;
	using channel_map_element_type = channel_map_type::value_type;

private:
	server_context_type   m_context;
	async::loop::ptr      m_loop;
	async::acceptor::ptr  m_acceptor;
	armi::channel_id_type m_next_channel_id;
	bool                  m_is_server_closing;
	channel_map_type      m_channel_map;
	request_handler       m_on_request;
	channel_close_handler m_on_channel_close;
	channel_error_handler m_on_channel_error;
	accept_error_handler  m_on_accept_error;
	server_close_handler  m_on_server_close;
	connection_handler    m_on_channel_connect;

public:
	server_adaptor(async::loop::ptr lp)
		: m_context{*this},
		  m_loop{lp},
		  m_next_channel_id{1},
		  m_is_server_closing{false}
	{
		m_on_channel_error = [=](armi::channel_id_type channel_id, std::error_code err)
		{
			close(channel_id);
		};

		m_on_accept_error = [=](std::error_code err)
		{
			close();
		};
	}

	server_adaptor&
	on_request(request_handler handler)
	{
		m_on_request = std::move(handler);
		return *this;
	}

	server_adaptor&
	on_channel_close(channel_close_handler handler)
	{
		m_on_channel_close = std::move(handler);
		return *this;
	}

	server_adaptor&
	on_channel_error(channel_error_handler handler)
	{
		m_on_channel_error = std::move(handler);
		return *this;
	}

	server_adaptor&
	on_channel_connect(connection_handler handler)
	{
		m_on_channel_connect = std::move(handler);
		return *this;
	}

	server_adaptor&
	on_server_close(server_close_handler handler)
	{
		m_on_server_close = std::move(handler);
		return *this;
	}

	server_adaptor&
	on_accept_error(accept_error_handler handler)
	{
		m_on_accept_error = std::move(handler);
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
					{
						if (m_on_accept_error)
						{
							m_on_accept_error(err);
						}
					}
					else
					{
						auto channel_id = get_next_channel_id();
						m_channel_map.emplace(channel_id, chan);
						if (m_on_channel_connect)
						{
							m_on_channel_connect(channel_id);
						}
						chan->start_read(
								err,
								[=](async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err) {
									if (err)
									{
										if (m_on_channel_error)
										{
											m_on_channel_error(channel_id, err);
										}
									}
									else
									{
										// TODO: check to see if channel_id is still in channel_map first
										bstream::imbstream is{std::move(buf), m_context.stream_context()};
										m_context.handle_request(channel_id, is, m_on_request(channel_id));
									}
								});
					}
				});
	}

	void
	close()
	{
		really_close();
	}

	virtual bool
	is_valid_channel(armi::channel_id_type channel_id)
	{
		bool result{false};
		if (channel_id != 0)
		{
			auto it = m_channel_map.find(channel_id);
			if (it != m_channel_map.end())
			{
				if (it->second && !it->second->is_closing())
				{
					result = true;
				}
			}
		}
		return result;
	}

	virtual void
	close(armi::channel_id_type channel_id)
	{
		if (channel_id != 0)
		{
			auto it = m_channel_map.find(channel_id);
			if (it != m_channel_map.end())
			{
				really_close(*it);
			}
		}
	}

	virtual void
	send_reply(armi::channel_id_type channel_id, util::mutable_buffer&& req)
	{
		auto it = m_channel_map.find(channel_id);
		if (it == m_channel_map.end())
		{
			if (m_on_channel_error)
			{
				m_on_channel_error(channel_id, make_error_code(armi::errc::invalid_channel_id));
			}
		}
		else
		{
			std::error_code err;
			it->second->write(std::move(req), err);
			if (err && m_on_channel_error)
			{
				m_on_channel_error(channel_id, err);
			}
		}
	}

private:
	armi::channel_id_type
	get_next_channel_id()
	{
		return m_next_channel_id++;
	}

	void
	cleanup()
	{
		if (m_on_server_close)
		{
			m_on_server_close();
			m_on_server_close = nullptr;
		}
		m_on_request  = nullptr;
		m_on_channel_close  = nullptr;
		m_on_channel_error  = nullptr;
		m_on_accept_error  = nullptr;
		m_on_server_close  = nullptr;
		m_on_channel_connect  = nullptr;
	}

	void
	really_close(channel_map_element_type& map_element)
	{
		auto channel_id = map_element.first;
		map_element.second->close([=](async::channel::ptr const& c) {
			auto cit = m_channel_map.find(channel_id);
			if (cit != m_channel_map.end())
			{
				m_channel_map.erase(cit);
				if (m_on_channel_close)
				{
					m_on_channel_close(channel_id);
				}
			}
			if (m_is_server_closing)
			{
				if (m_channel_map.empty() && !m_acceptor)
				{
					cleanup();
				}
			}
		});
	}

	void
	really_close()
	{
		if (!m_is_server_closing)
		{
			m_is_server_closing = true;
			if (m_acceptor)
			{
				m_acceptor->close([=](async::acceptor::ptr const& lp) {
					m_acceptor.reset();
					if (m_channel_map.empty())
					{
						cleanup();
					}
				});
			}
			for (channel_map_element_type& element : m_channel_map)
			{
				really_close(element);
			}
		}
	}
};

}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_ASYNC_ADAPTER_H
