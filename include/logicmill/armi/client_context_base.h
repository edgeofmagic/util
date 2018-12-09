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

/* 
 * File:   client_context_base.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 5:52 PM
 */

#ifndef LOGICMILL_ARMI_CLIENT_CONTEXT_BASE_H
#define LOGICMILL_ARMI_CLIENT_CONTEXT_BASE_H

#include <chrono>
#include <logicmill/armi/reply_handler_base.h>
#include <logicmill/armi/types.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/bstream/imbstream.h>
#include <logicmill/bstream/ombstream.h>
#include <unordered_map>

namespace logicmill
{
namespace armi
{

class interface_proxy;

template<class T>
class method_proxy_base;

class client_context_base
{
public:
	using connect_handler         = std::function<void(std::error_code err)>;
	using request_timeout_handler = std::function<void(std::error_code err)>;

	struct reply_handler_info
	{
		reply_handler_info(reply_handler_base::ptr&& hndlr, bool prst) : handler{std::move(hndlr)}, persist{prst} {}
		reply_handler_base::ptr handler;
		bool                    persist;
	};

	client_context_base(async::loop::ptr const& lp, bstream::context_base const& cntxt);

	async::loop::ptr
	loop() const
	{
		return m_loop;
	}

	bstream::context_base const&
	stream_context() const
	{
		return m_stream_context;
	}

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, connect_handler>::value>
	connect(async::options const& opts, std::error_code& err, Handler&& handler);

	void
	close();

	virtual ~client_context_base();

private:

	friend class logicmill::armi::interface_proxy;

	template<class T>
	friend class logicmill::armi::method_proxy_base;

	void
	add_handler(std::uint64_t req_ord, reply_handler_base::ptr&& handler, bool persist = false)
	{
		auto result = m_reply_handler_map.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(req_ord),
				std::forward_as_tuple(std::move(handler), persist));
		assert(result.second);
	}

	std::unique_ptr<bstream::ombstream>
	create_request_stream()
	{
		return std::make_unique<bstream::ombstream>(m_stream_context);
	}

	void
	check_async(std::error_code& err);

	void
	send_request(std::uint64_t req_ord, bstream::ombstream& os, millisecs timeout);

	void
	on_read(bstream::const_buffer&& buf, std::error_code& err)
	{
		bstream::imbstream is{std::move(buf), m_stream_context};
		invoke_handler(is);
	}

	void
	cancel_all(std::error_code ec)
	{
		cancel_all_reply_handlers(ec);
	}

	bool
	invoke_handler(bstream::ibstream& is);

	bool
	cancel_handler(std::uint64_t req_ord, std::error_code ec);

	bool
	cancel_handler(std::uint64_t req_ord);    // no notification;

	void
	cancel_all_reply_handlers(std::error_code ec);

	std::uint64_t
	next_request_ordinal()
	{
		return m_next_request_ordinal++;
	}

	template<class T>
	std::size_t
	get_index()
	{
		return T::index;
	}

	millisecs
	default_timeout() const
	{
		return m_default_timeout;
	}

	void
	default_timeout(millisecs timeout)
	{
		m_default_timeout = timeout;
	}

	millisecs
	transient_timeout()
	{
		return m_transient_timeout;
	}

	void
	transient_timeout(millisecs timeout)
	{
		m_transient_timeout = timeout;
	}

	void
	clear_transient_timeout()
	{
		m_transient_timeout = millisecs{0};
	}

	class connect_channel_handler;
	friend class client_context_base::connect_channel_handler;

	class connect_channel_handler_base
	{
	public:
		connect_channel_handler_base(client_context_base& client_context) : m_context{client_context} {}

		virtual void
		operator()(client_context_base& cntxt, async::channel::ptr chan, std::error_code err) = 0;
	
	private:
		client_context_base&                 m_context;
	};

	class connect_channel_handler
	{
	public:
		template<
				class Handler,
				class = typename std::enable_if_t<
						std::is_convertible<Handler, client_context_base::connect_handler>::value>>
		connect_channel_handler(client_context_base& cntxt, Handler&& handler)
			: m_context{cntxt}, m_handler{std::forward<Handler>(handler)}
		{}

		void
		operator()(async::channel::ptr const& chan, std::error_code err);

		client_context_base&                 m_context;
		client_context_base::connect_handler m_handler;
	};

	async::loop::ptr                                      m_loop;
	bstream::cloned_context                               m_stream_context;
	std::uint64_t                                         m_next_request_ordinal;
	std::unordered_map<std::uint64_t, reply_handler_info> m_reply_handler_map;
	millisecs                                             m_default_timeout;
	millisecs                                             m_transient_timeout;
	async::channel::ptr                                   m_channel;
};

}    // namespace armi
}    // namespace logicmill

template<class Handler>
typename std::enable_if_t<std::is_convertible<Handler, logicmill::armi::client_context_base::connect_handler>::value>
logicmill::armi::client_context_base::connect(
		logicmill::async::options const& opts,
		std::error_code&                 err,
		Handler&&                        handler)
{
	logicmill::async::options opts_override{opts};
	opts_override.framing(true);
	m_loop->connect_channel(opts_override, err, connect_channel_handler{*this, std::forward<Handler>(handler)});
}

#endif /* LOGICMILL_ARMI_CLIENT_CONTEXT_BASE_H */
