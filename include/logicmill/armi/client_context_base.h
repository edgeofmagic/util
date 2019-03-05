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
#include <set>

namespace logicmill
{
namespace armi
{

// class interface_proxy;

template<class Reply, class Fail = void, class Enable = void>
class method_proxy_base;

class client_context_base
{
public:
	using close_handler           = std::function<void()>;
	using transport_error_handler = std::function<void(std::error_code err)>;

	using reply_handler_map_type = std::unordered_map<request_id_type, std::pair<channel_id_type, reply_handler_base::ptr>>;
	using channel_request_map_type = std::unordered_map<channel_id_type, std::set<request_id_type>>;
	// using request_id_type         = std::uint64_t;

	client_context_base(transport::client& tclient, bstream::context_base::ptr const& stream_context);

	// TODO: fix close chain

	virtual ~client_context_base()
	{
		cancel_all_requests(make_error_code(armi::errc::context_closed));
	}

	void
	cancel_request(request_id_type request_id, std::error_code err);

	// void
	// cancel_request(request_id_type request_id);    // no notification

	void
	cancel_all_requests(std::error_code ec);

	// void
	// cancel_all_requests();    // no notification;

	void
	cancel_channel_requests(channel_id_type channel_id, std::error_code err);

	void
	handle_reply(util::const_buffer&& buf)
	{
		bstream::imbstream is{std::move(buf), m_stream_context};
		invoke_handler(is);
	}

	// void
	// on_transport_error(transport_error_handler handler)
	// {
	// 	m_on_transport_error = std::move(handler);
	// }

	// void
	// transport_error(std::error_code err)
	// {
	// 	if (m_on_transport_error)
	// 	{
	// 		m_on_transport_error(err);
	// 	}
	// }

protected:
	// friend class logicmill::armi::interface_proxy;

	template<class T, class U, class V>
	friend class logicmill::armi::method_proxy_base;

	bstream::context_base::ptr const&
	stream_context() const
	{
		return m_stream_context;
	}

	void
	add_handler(request_id_type request_id, reply_handler_base::ptr&& handler)
	{
		auto channel_id = get_transient_channel_id();
		auto it = m_channel_request_map.find(channel_id);
		if (it == m_channel_request_map.end())
		{
			m_channel_request_map.emplace(channel_id, std::set<request_id_type>{request_id});
		}
		else
		{
			it->second.insert(request_id);
		}
		
		auto result = m_reply_handler_map.emplace(request_id, std::make_pair(channel_id, std::move(handler)));

		assert(result.second);
	}

	std::unique_ptr<bstream::ombstream>
	create_request_stream()
	{
		return std::make_unique<bstream::ombstream>(m_stream_context);
	}

	void
	send_request(
			channel_id_type channel_id,
			request_id_type            request_id,
			util::mutable_buffer&&        req,
			millisecs                  timeout);

	void
	invoke_handler(bstream::ibstream& is);

	request_id_type
	next_request_id()
	{
		return m_next_request_id++;
	}

	bool
	is_valid_channel_id(channel_id_type id)
	{
		return m_transport.is_valid_channel(id);
	}

	void
	close(channel_id_type channel_id)
	{
		m_transport.close(channel_id);
	}

	millisecs
	get_default_timeout() const
	{
		return m_default_timeout;
	}

	void
	set_default_timeout(millisecs timeout)
	{
		m_default_timeout = timeout;
	}

	millisecs
	get_transient_timeout() const
	{
		return m_transient_timeout;
	}

	void
	set_transient_timeout(millisecs timeout)
	{
		m_transient_timeout = timeout;
	}

	millisecs
	get_and_clear_transient_timeout()
	{
		millisecs result{m_transient_timeout};
		m_transient_timeout = millisecs{0};
		return result;
	}

	void
	set_transient_channel_id(channel_id_type id)
	{
		m_transient_channel_id = id;
	}

	channel_id_type
	get_transient_channel_id() const
	{
		return m_transient_channel_id;
	}

	channel_id_type
	get_and_clear_transient_channel_id()
	{
		channel_id_type result{m_transient_channel_id};
		m_transient_channel_id = 0;
		return result;
	}

	template<class V>
	void 
	visit_handler(request_id_type request_id, V visitor)
	{
		auto it = m_reply_handler_map.find(request_id);
		if (it != m_reply_handler_map.end())
		{
			auto channel_id = it->second.first;
			visitor(it);
			m_reply_handler_map.erase(it);

			auto cit = m_channel_request_map.find(channel_id);
			if (cit != m_channel_request_map.end())
			{
				cit->second.erase(request_id);
				if (cit->second.empty())
				{
					m_channel_request_map.erase(cit);
				}
			}
		}
	}

	bstream::context_base::ptr m_stream_context;
	request_id_type            m_next_request_id;
	reply_handler_map_type     m_reply_handler_map;
	channel_request_map_type   m_channel_request_map;
	millisecs                  m_default_timeout;
	millisecs                  m_transient_timeout;
	channel_id_type            m_transient_channel_id;
	transport::client&         m_transport;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_CONTEXT_BASE_H
