
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

#ifndef LOGICMILL_ARMI_CLIENT_PROXY_BASE_H
#define LOGICMILL_ARMI_CLIENT_PROXY_BASE_H

#include <chrono>
#include <logicmill/armi/adapters/bridge.h>
#include <logicmill/armi/reply_handler_base.h>
#include <logicmill/armi/serialization_traits.h>
#include <logicmill/armi/async_io_traits.h>
#include <logicmill/armi/types.h>
#include <memory>
#include <set>
#include <unordered_map>

namespace logicmill
{
namespace armi
{

template<class U, class V>
class member_func_proxy;

template<class SerializationTraits, class AsyncIOTraits>
class client_proxy_base
{
public:
	using serialization_traits = SerializationTraits;
	using async_io_traits     = AsyncIOTraits;

	using bridge_type = adapters::bridge<serialization_traits, async_io_traits>;

	using deserializer_param_type = typename bridge_type::deserializer_param_type;
	using serializer_param_type   = typename bridge_type::serializer_param_type;

	using close_handler           = std::function<void()>;
	using async_io_error_handler = std::function<void(std::error_code err)>;

	using reply_handler_map_type = std::unordered_map<
			request_id_type,
			std::pair<channel_id_type, typename reply_handler_base<bridge_type>::ptr>>;
	using channel_request_map_type = std::unordered_map<channel_id_type, std::set<request_id_type>>;

	client_proxy_base()
		: m_next_request_id{1},
		  m_default_timeout{millisecs{0}},
		  m_transient_timeout{millisecs{0}},
		  m_transient_channel{}
	{}

	virtual ~client_proxy_base()
	{
		cancel_all_requests(make_error_code(armi::errc::client_closed));
	}

	void
	cancel_request(request_id_type request_id, std::error_code err)
	{
		visit_handler(
				request_id, [=](typename reply_handler_map_type::iterator it) { it->second.second->cancel(err); });
	}

	void
	cancel_all_requests(std::error_code err)
	{
		// use the crowbar
		for (auto it = m_reply_handler_map.begin(); it != m_reply_handler_map.end(); ++it)
		{
			it->second.second->cancel(err);
		}
		m_reply_handler_map.clear();
		m_channel_request_map.clear();
	}

	void
	cancel_channel_requests(channel_id_type channel, std::error_code err)
	{
		auto it = m_channel_request_map.find(channel);
		if (it != m_channel_request_map.end())
		{
			for (auto request_id : it->second)
			{
				auto rit = m_reply_handler_map.find(request_id);
				if (rit != m_reply_handler_map.end())
				{
					rit->second.second->cancel(err);
					m_reply_handler_map.erase(rit);
				}
			}
			m_channel_request_map.erase(it);
		}
	}

	void
	handle_reply(deserializer_param_type reply)
	{
		invoke_handler(reply);
	}

protected:
	template<class U, class V>
	friend class logicmill::armi::member_func_proxy;

	virtual bool
	is_valid_channel(channel_id_type channel)
			= 0;

	virtual void
	close(channel_id_type channel)
			= 0;

	virtual void
	send_request(
			channel_id_type           channel,
			request_id_type           request_id,
			std::chrono::milliseconds timeout,
			serializer_param_type     req)
			= 0;

	void
	add_handler(request_id_type request_id, typename reply_handler_base<bridge_type>::ptr&& handler)
	{
		auto channel = get_transient_channel();
		auto it      = m_channel_request_map.find(channel);
		if (it == m_channel_request_map.end())
		{
			m_channel_request_map.emplace(channel, std::set<request_id_type>{request_id});
		}
		else
		{
			it->second.insert(request_id);
		}

		auto result = m_reply_handler_map.emplace(request_id, std::make_pair(channel, std::move(handler)));

		assert(result.second);
	}

	void
	send_request(request_id_type request_id, millisecs timeout, serializer_param_type req)
	{
		send_request(get_and_clear_transient_channel(), request_id, timeout, req);
	}

	void
	invoke_handler(deserializer_param_type reply)
	{
		auto request_id = serialization_traits::template read<request_id_type>(reply);
		visit_handler(request_id, [=, &reply](typename reply_handler_map_type::iterator it) {
			it->second.second->handle_reply(reply);
		});
	}


	request_id_type
	next_request_id()
	{
		return m_next_request_id++;
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
	set_transient_channel(channel_id_type channel) const
	{
		m_transient_channel = channel;
	}

	channel_id_type
	get_transient_channel() const
	{
		return m_transient_channel;
	}

	channel_id_type
	get_and_clear_transient_channel()
	{
		channel_id_type result{m_transient_channel};
		m_transient_channel = null_channel;
		return result;
	}

	template<class V>
	void
	visit_handler(request_id_type request_id, V visitor)
	{
		auto it = m_reply_handler_map.find(request_id);
		if (it != m_reply_handler_map.end())
		{
			auto channel = it->second.first;
			visitor(it);
			m_reply_handler_map.erase(it);

			auto cit = m_channel_request_map.find(channel);
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

	request_id_type          m_next_request_id;
	reply_handler_map_type   m_reply_handler_map;
	channel_request_map_type m_channel_request_map;
	millisecs                m_default_timeout;
	millisecs                m_transient_timeout;
	mutable channel_id_type  m_transient_channel;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_PROXY_BASE_H
