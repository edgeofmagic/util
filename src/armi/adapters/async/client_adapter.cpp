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

#include <logicmill/armi/adapters/async/client_adapter.h>

using namespace logicmill;
using namespace armi;

void
async::client_adapter_base::close(armi::channel_id_type channel_id, std::error_code err)
{
	if (is_valid_channel(channel_id))
		m_loop->dispatch([=]() { really_close(channel_id, err); });
}

void
async::client_adapter_base::close(std::error_code err)
{
	if (!m_is_closing)
	{
		m_is_closing = true;
		really_close(make_error_code(armi::errc::context_closed));
	}
}

bool
async::client_adapter_base::is_valid_channel(armi::channel_id_type channel_id)
{
	bool result{false};
	auto chan = get_channel(channel_id);
	if (chan && !chan->is_closing())
		result = true;
	return result;
}

void
async::client_adapter_base::close(armi::channel_id_type channel_id)
{
	close(channel_id, make_error_code(armi::errc::channel_closed));
}

void
async::client_adapter_base::send_request(
		armi::channel_id_type     channel_id,
		armi::request_id_type     request_id,
		std::chrono::milliseconds timeout,
		util::mutable_buffer&&    req)
{
	std::error_code err;

	auto chan = get_channel(channel_id);
	if (!chan)
	{
		err = make_error_code(armi::errc::invalid_channel_id);
		goto exit;
	}

	if (timeout.count() > 0)
	{
		m_loop->schedule(timeout, err, [=]() {
			m_context_base.cancel_request(request_id, make_error_code(std::errc::timed_out));
		});
		if (err)
			goto exit;
	}

	chan->write(std::move(req), err);
	if (err)
		goto exit;

	// chan->write(std::move(req), err, [=](channel::ptr const& chan, util::mutable_buffer&& buf, std::error_code err)
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
	{
		if (m_loop->is_alive())
			m_loop->dispatch([=]() { m_context_base.cancel_request(request_id, err); });
		else
			m_context_base.cancel_request(request_id, make_error_code(armi::errc::no_event_loop));
	}
	return;
}

void
async::client_adapter_base::really_close(armi::channel_id_type channel_id, std::error_code err)
{
	auto chan = get_channel(channel_id);
	if (chan)
	{
		remove(channel_id);
		if (m_loop->is_alive())
		{
			chan->close();
			m_loop->dispatch([=]() { m_context_base.cancel_channel_requests(channel_id, err); });
		}
		else
			m_context_base.cancel_channel_requests(channel_id, make_error_code(armi::errc::no_event_loop));
	}
}

void
async::client_adapter_base::really_close(std::error_code err)
{
	if (m_loop->is_alive())
	{
		visit_and_remove_all([](armi::channel_id_type channel_id, async::channel::ptr const& chan) {
			if (chan)
				chan->close();
		});
		m_loop->dispatch([=]() { m_context_base.cancel_all_requests(err); });
	}
	else
	{
		clear_map();
		m_context_base.cancel_all_requests(make_error_code(armi::errc::no_event_loop));
	}
}
