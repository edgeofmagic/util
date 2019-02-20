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

#ifndef LOGICMILL_ARMI_CLIENT_CONTEXT_BASE_H
#define LOGICMILL_ARMI_CLIENT_CONTEXT_BASE_H

#include <chrono>
#include <logicmill/armi/reply_handler_base.h>
#include <logicmill/armi/transport.h>
#include <logicmill/armi/types.h>
// #include <logicmill/async/channel.h>
// #include <logicmill/async/loop.h>
#include <logicmill/bstream/imbstream.h>
#include <logicmill/bstream/ombstream.h>
#include <memory>
#include <unordered_map>

namespace logicmill
{
namespace armi
{

class interface_proxy;

template<class T>
class method_proxy_base;

class client_context_base : public ENABLE_SHARED_FROM_THIS<client_context_base>
{
public:
	using ptr                     = SHARED_PTR_TYPE<client_context_base>;
	using wptr                    = WEAK_PTR_TYPE<client_context_base>;
	using close_handler           = std::function<void()>;
	using transport_error_handler = std::function<void(std::error_code err)>;


	client_context_base(bstream::context_base::ptr const& stream_context);

	void
	use(transport::client_channel::ptr const& transp)
	{
		if (m_channel)
		{
			cancel_all_requests(make_error_code(armi::errc::transport_closed));
			m_channel->close(nullptr);
		}
		m_channel = transp;
	}

	template<class T>
	typename std::enable_if_t<std::is_convertible<T, close_handler>::value>
	close(T&& handler)
	{
		if (!m_is_closing)
		{
			m_is_closing = true;
			cancel_all_requests(make_error_code(armi::errc::context_closed));
			if (m_channel)
			{
				m_channel->close(std::forward<T>(handler));
			}
			m_channel.reset();
		}
	}

	void
	close()
	{
		if (!m_is_closing)
		{
			m_is_closing = true;
			cancel_all_requests(make_error_code(armi::errc::context_closed));
			if (m_channel)
			{
				m_channel->close();
			}
			m_channel.reset();
		}
	}

	virtual ~client_context_base();

	void
	cancel_request(std::uint64_t request_id, std::error_code err);

	void
	cancel_request(std::uint64_t req_ord);    // no notification

	void
	cancel_all_requests(std::error_code ec);

	void
	cancel_all_requests();    // no notification;

	void
	handle_reply(util::const_buffer&& buf)
	{
		bstream::imbstream is{std::move(buf), m_stream_context};
		invoke_handler(is);
	}

	void
	on_transport_error(transport_error_handler handler)
	{
		m_on_transport_error = std::move(handler);
	}

	void
	transport_error(std::error_code err)
	{
		if (m_on_transport_error)
		{
			m_on_transport_error(err);
		}
	}

private:
	friend class logicmill::armi::interface_proxy;

	template<class T>
	friend class logicmill::armi::method_proxy_base;

	bstream::context_base::ptr const&
	stream_context() const
	{
		return m_stream_context;
	}

	void
	add_handler(std::uint64_t req_ord, reply_handler_base::ptr&& handler)
	{
		auto result = m_reply_handler_map.emplace(req_ord, std::move(handler));
		assert(result.second);
	}

	std::unique_ptr<bstream::ombstream>
	create_request_stream()
	{
		return std::make_unique<bstream::ombstream>(m_stream_context);
	}

	void
	send_request(std::uint64_t req_ord, bstream::ombstream& os, millisecs timeout);

	void
	invoke_handler(bstream::ibstream& is);

	std::uint64_t
	next_request_ordinal()
	{
		return m_next_request_ordinal++;
	}

	// template<class T>
	// std::size_t
	// get_index()
	// {
	// 	return T::index;
	// }

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

	bstream::context_base::ptr                                 m_stream_context;
	std::uint64_t                                              m_next_request_ordinal;
	std::unordered_map<std::uint64_t, reply_handler_base::ptr> m_reply_handler_map;
	millisecs                                                  m_default_timeout;
	millisecs                                                  m_transient_timeout;
	bool                                                       m_is_closing;
	transport::client_channel::ptr                                     m_channel;
	transport_error_handler                                    m_on_transport_error;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_CONTEXT_BASE_H
