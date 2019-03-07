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

#ifndef LOGICMILL_ARMI_ADAPTER_ASYNC_CLIENT_H
#define LOGICMILL_ARMI_ADAPTER_ASYNC_CLIENT_H

#include <functional>
#include <logicmill/armi/adapters/async/channel_manager.h>
#include <logicmill/armi/client_context.h>
#include <logicmill/armi/transport.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/util/promise.h>

namespace logicmill
{
namespace async
{

class client_adapter_base : public armi::transport::client, public channel_manager
{
public:
	using channel_error_handler = std::function<void(armi::channel_id_type channel_id, std::error_code err)>;

	/* public interface for client_adapter */

	client_adapter_base(loop::ptr lp, armi::client_context_base& context)
		: m_loop{lp}, m_context_base{context}, m_is_closing{false}
	{}

	virtual ~client_adapter_base() {}

	void
	close(armi::channel_id_type channel_id, std::error_code err);

	void
	close()
	{
		close(make_error_code(armi::errc::context_closed));
	}

	void
	close(std::error_code err);

	void
	on_channel_read_error(channel_error_handler handler)
	{
		m_on_channel_read_error = std::move(handler);
	}

	virtual bool
	is_valid_channel(armi::channel_id_type channel_id) override;

	virtual void
	close(armi::channel_id_type channel_id) override;

	virtual void
	send_request(
			armi::channel_id_type     channel_id,
			armi::request_id_type     request_id,
			std::chrono::milliseconds timeout,
			util::mutable_buffer&&    req) override;

protected:
	void
	really_close(armi::channel_id_type channel_id, std::error_code err);

	void
	really_close(std::error_code err);

	async::loop::ptr           m_loop;
	armi::client_context_base& m_context_base;
	armi::channel_id_type      m_next_channel_id;
	channel_error_handler      m_on_channel_read_error;
	bool                       m_is_closing;
};

template<class T>
class client_adapter : public client_adapter_base
{
public:
	using client_context_type    = T;
	using channel_error_handler  = std::function<void(armi::channel_id_type channel_id, std::error_code err)>;
	using client_connect_handler = std::function<void(typename client_context_type::client_channel, std::error_code)>;

	client_adapter(loop::ptr lp) : client_adapter_base{lp, m_context}, m_context{*this} {}

	virtual ~client_adapter()
	{
		close();
	}

	util::promise<typename client_context_type::client_channel>
	connect(async::options const& opts)
	{
		async::options options_override{opts};
		options_override.framing(true);
		util::promise<typename client_context_type::client_channel> p;
		std::error_code                                             err;
		m_loop->connect_channel(options_override, err, [=](async::channel::ptr chan, std::error_code err) mutable {
			if (err)
				p.reject(err);
			else
			{
				auto id = new_channel(chan);
				chan->start_read(
						err, [=](async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err) {
							if (err)
							{
								if (m_on_channel_read_error)
									m_on_channel_read_error(id, err);
								else
									close(id, err);
							}
							else
								m_context.handle_reply(std::move(buf));
						});
				if (err)
					p.reject(err);
				else
					p.resolve(m_context.create_channel(id));
			}
		});
		if (err)
			p.reject(err);
		return p;
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
						handler(m_context.create_channel(0), err);
					else
					{
						auto id = new_channel(chan);
						chan->start_read(
								err,
								[=](async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err) {
									if (err)
									{
										if (m_on_channel_read_error)
											m_on_channel_read_error(id, err);
										else
											close(id, err);
									}
									else
										m_context.handle_reply(std::move(buf));
								});
						handler(m_context.create_channel(id), std::error_code{});
					}
				});
	}

private:
	client_context_type m_context;
};

}    // namespace async
}    // namespace logicmill
#endif    // LOGICMILL_ARMI_ADAPTER_ASYNC_CLIENT_H