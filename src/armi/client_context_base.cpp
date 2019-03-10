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

#include <logicmill/armi/client_context_base.h>
#include <logicmill/armi/error.h>

using namespace logicmill;
using namespace armi;

client_context_base::client_context_base(transport::client& transport_client, bstream::context_base const& cntxt)
	: m_stream_context{cntxt},
	  m_next_request_id{1},
	  m_default_timeout{millisecs{0}},
	  m_transient_timeout{millisecs{0}},
	  m_transient_channel_id{0},
	  m_transport{transport_client}
{}

void
client_context_base::send_request(
		channel_id_type        channel_id,
		request_id_type        request_id,
		util::mutable_buffer&& req,
		millisecs              timeout)
{
	m_transport.send_request(channel_id, request_id, timeout, std::move(req));
}

void
client_context_base::invoke_handler(bstream::ibstream& is)
{
	auto request_id = is.read_as<request_id_type>();
	visit_handler(request_id, [=, &is](reply_handler_map_type::iterator it) { it->second.second->handle_reply(is); });
}

void
client_context_base::cancel_request(request_id_type request_id, std::error_code err)
{
	visit_handler(request_id, [=](reply_handler_map_type::iterator it) { it->second.second->cancel(err); });
}

void
client_context_base::cancel_all_requests(std::error_code err)
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
client_context_base::cancel_channel_requests(channel_id_type channel_id, std::error_code err)
{
	auto it = m_channel_request_map.find(channel_id);
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
