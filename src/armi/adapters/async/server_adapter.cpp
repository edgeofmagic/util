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

#include <logicmill/armi/adapters/async/server_adapter.h>

using namespace logicmill;
using namespace armi;

bool
async::server_adapter_base::is_valid_channel(armi::channel_id_type channel_id)
{
	bool result{false};
	auto chan = get_channel(channel_id);
	if (chan && !chan->is_closing())
		result = true;
	return result;
}

void
async::server_adapter_base::send_reply(armi::channel_id_type channel_id, util::mutable_buffer&& req)
{
	auto chan = get_channel(channel_id);
	if (!chan)
		channel_error(channel_id, make_error_code(armi::errc::invalid_channel_id));
	else
	{
		std::error_code err;
		chan->write(std::move(req), err);
		if (err)
			channel_error(channel_id, err);
	}
}

void
async::server_adapter_base::accept_error(std::error_code err)
{
	if (m_on_accept_error)
		m_on_accept_error(err);
	else
		default_accept_error_handler(err);
}

void
async::server_adapter_base::channel_error(armi::channel_id_type channel_id, std::error_code err)
{
	if (m_on_channel_error)
		m_on_channel_error(channel_id, err);
	else
		default_channel_error_handler(channel_id, err);
}
void
async::server_adapter_base::cleanup_base()
{
	m_on_server_close    = nullptr;
	// m_on_request         = nullptr;
	m_on_channel_close   = nullptr;
	m_on_channel_error   = nullptr;
	m_on_accept_error    = nullptr;
	m_on_channel_connect = nullptr;
}

void
async::server_adapter_base::really_close(armi::channel_id_type channel_id)
{
	visit_and_remove(channel_id, [](armi::channel_id_type channel_id, async::channel::ptr chan) {
		if (chan)
			chan->close();
	});
	if (m_on_channel_close)
		m_on_channel_close(channel_id);
}

void
async::server_adapter_base::really_close()
{
	if (!m_is_server_closing)
	{
		m_is_server_closing = true;

		if (m_acceptor)
		{
			m_acceptor->close();
			if (m_on_server_close)
				m_on_server_close();
		}
		std::vector<armi::channel_id_type> closed_channels;
		closed_channels.reserve(active_channel_count());
		visit_and_remove_all([&closed_channels](armi::channel_id_type channel_id, async::channel::ptr chan) {
			closed_channels.push_back(channel_id);
			if (chan)
				chan->close();
		});
		if (m_on_channel_close)
		{
			for (auto channel_id : closed_channels)
				m_on_channel_close(channel_id);
		}
		cleanup();
	}
}