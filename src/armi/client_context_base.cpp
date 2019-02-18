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

client_context_base::client_context_base(bstream::context_base::ptr const& cntxt)
	: m_stream_context{cntxt},
	  m_next_request_ordinal{1},
	  m_default_timeout{millisecs{0}},
	  m_transient_timeout{millisecs{0}},
	  m_is_closing{false},
	  m_channel{nullptr}
{}

client_context_base::~client_context_base()
{
	close();
}

void
client_context_base::send_request(std::uint64_t req_ord, bstream::ombstream& os, millisecs timeout)
{
	std::error_code err;

	if (m_channel)
	{
		m_channel->send_request(req_ord, timeout, os.release_mutable_buffer(), err);
	}
	else
	{
		err = make_error_code(armi::errc::transport_not_set);
	}

	if (err)
	{
		cancel_request(req_ord, err);
	}
}

void
client_context_base::invoke_handler(bstream::ibstream& is)
{
	auto req_ord = is.read_as<std::uint64_t>();
	auto it      = m_reply_handler_map.find(req_ord);
	if (it != m_reply_handler_map.end())
	{
		it->second->handle_reply(is);
		m_reply_handler_map.erase(it);
	} // else discard silently, probably canceled
}

void
client_context_base::cancel_request(std::uint64_t req_ord, std::error_code err)
{
	auto it = m_reply_handler_map.find(req_ord);
	if (it != m_reply_handler_map.end())
	{
		it->second->cancel(err);
		m_reply_handler_map.erase(it);
	}
}

void
client_context_base::cancel_request(std::uint64_t req_ord)    // no notification
{
	auto it = m_reply_handler_map.find(req_ord);
	if (it != m_reply_handler_map.end())
	{
		m_reply_handler_map.erase(it);
	}
}

void
client_context_base::cancel_all_requests(std::error_code err)
{
	for (auto it = m_reply_handler_map.begin(); it != m_reply_handler_map.end(); it = m_reply_handler_map.erase(it))
	{
		it->second->cancel(err);
	}
}

void
client_context_base::cancel_all_requests()
{
	for (auto it = m_reply_handler_map.begin(); it != m_reply_handler_map.end(); it = m_reply_handler_map.erase(it))
	{
	}
}
