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

#include <logicmill/ras/client_context_base.h>
#include <logicmill/ras/error.h>

using namespace logicmill;
using namespace ras;

client_context_base::client_context_base(async::loop::ptr const& lp, bstream::context_base const& cntxt)
:
m_loop{lp},
m_stream_context{cntxt},
m_next_request_ordinal{1}, 
m_default_timeout{millisecs{0}}, 
m_transient_timeout{millisecs{0}}
{}

void
client_context_base::check_async(std::error_code& err)
{
	err.clear();
	if (!m_loop)
	{
		err = make_error_code(ras::errc::no_event_loop);
		goto exit;
	}
	if (!m_channel)
	{
		err = make_error_code(ras::errc::channel_not_connected);
		goto exit;
	}

exit:
	return;
}

client_context_base::~client_context_base()
{
	close();
}

void
client_context_base::close()
{
	cancel_all_reply_handlers(make_error_code(std::errc::operation_canceled));
	if (m_channel)
	{
		m_channel->close();
	}
	m_channel.reset();
}

void
client_context_base::send_request(std::uint64_t req_ord, bstream::ombstream& os, millisecs timeout)
{
	std::error_code err;
	check_async(err);
	if (err) goto exit;

	if (timeout.count() > 0)
	{
		auto timer = m_loop->create_timer(err, [=](async::timer::ptr tp)
		{
			cancel_handler(req_ord, std::make_error_code(std::errc::timed_out));
		});
		if (err) goto exit;

		timer->start(timeout, err);
		if (err) goto exit;
	}

	m_channel->write(os.release_mutable_buffer(), err);

exit:
	if (err)
	{
		cancel_handler(req_ord, err);
	}
}

bool
client_context_base::invoke_handler(bstream::ibstream& is)
{
	auto req_ord = is.read_as<std::uint64_t>();
	auto it = m_reply_handler_map.find(req_ord);
	if (it == m_reply_handler_map.end())
	{
		return false;
	}
	else
	{
		it->second.handler->handle_reply(is);
		if (!it->second.persist)
		{
			m_reply_handler_map.erase(it);
		}
		return true;
	}
}

bool
client_context_base::cancel_handler(std::uint64_t req_ord, std::error_code ec)
{
	auto it = m_reply_handler_map.find(req_ord);
	if (it == m_reply_handler_map.end())
	{
		return false;
	}
	else
	{
		it->second.handler->cancel(ec);
		m_reply_handler_map.erase(it);
		return true;
	}
}

bool
client_context_base::cancel_handler(std::uint64_t req_ord) // no notification
{
	auto it = m_reply_handler_map.find(req_ord);
	if (it == m_reply_handler_map.end())
	{
		return false;
	}
	else
	{
		m_reply_handler_map.erase(it);
		return true;
	}
}

void
client_context_base::cancel_all_reply_handlers(std::error_code ec)
{
	for (auto it = m_reply_handler_map.begin();
		 it != m_reply_handler_map.end();
		 it = m_reply_handler_map.erase(it))
	{
		it->second.handler->cancel(ec);
	}
}
