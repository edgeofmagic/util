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

#include <logicmill/ras/fail_proxy.h>
#include <logicmill/ras/server_context_base.h>
#include <logicmill/ras/error.h>
#include <logicmill/bstream/imbstream.h>

using namespace logicmill;
using namespace ras;


server_context_base::server_context_base(std::size_t interface_count, async::loop::ptr lp, bstream::context_base const& stream_context)
:
m_loop{lp},
m_stream_context{stream_context},
m_impl_ptrs{interface_count, nullptr}
{}

server_context_base::~server_context_base()
{
	// if (transport_)
	// {
	// 	transport_->disassociate();
	// }
}

// void
// server_context_base::use_transport(server_transport_impl_base::ptr transport)
// {
// 	if (transport_)
// 	{
// 		transport_->disassociate();
// 	}
// 	transport_ = std::move(transport);
// 	transport_->associate(this);
// }

void
server_context_base::really_bind(async::options const& opts, std::error_code& err)
{
	m_listener = m_loop->create_listener(opts, err, [=](async::listener::ptr const& sp, async::channel::ptr const& chan, std::error_code err)
	{
		if (err)
		{
			m_on_listener_error(err);
		}
		else
		{
			m_open_channels.insert(chan);
			chan->start_read(err, [=](async::channel::ptr const& chan, bstream::const_buffer&& buf, std::error_code err)
			{
				if (err)
				{
					if (m_on_channel_error)
					{
						m_on_channel_error(chan, err);
					}
				}
				else
				{
					bstream::imbstream is{std::move(buf), m_stream_context};
					handle_request(is, chan);

				}
			});
		}
	});
}

void 
server_context_base::send_reply(async::channel::ptr const& chan, bstream::mutable_buffer&& buf)
{
	std::error_code err;
	assert(chan);
	if (chan->is_closing())
	{
		m_on_channel_error(chan, make_error_code(ras::errc::channel_not_connected));
	}
	else
	{	
		chan->write(std::move(buf), err, [=](async::channel::ptr const& chan, bstream::mutable_buffer&& buf, std::error_code err)
		{
			if (err && m_on_channel_error)
			{
				m_on_channel_error(chan, err);
			}
		});

		if (err && m_on_channel_error)
		{
			m_on_channel_error(chan, err);
		}
	}
}

void
server_context_base::handle_request(bstream::ibstream& is, async::channel::ptr const& chan)
{
	auto req_ord = is.read_as<std::uint64_t>();
    std::size_t if_index = is.read_as<std::size_t>();
	if (if_index >= m_stubs.size())
	{
		fail_proxy{*this, req_ord, chan}(make_error_code(ras::errc::invalid_interface_id));
	}
	else
	{
		auto& stub = get_stub(if_index);
		stub.process(req_ord, chan, is);
	}
}

// void
// server_context_base::handle_error(std::error_code ec)
// {
// 	if (m_on_error)
// 	{
// 		m_on_error(ec);
// 	}
// 	else
// 	{
// 		std::ostringstream os;
// 		os << "fatal error in server context: " << ec.message();
// 		FATAL_ERROR(os.str());
// 	}
// }

