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
 * File:   server_context_base.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 6:30 PM
 */

#ifndef LOGICMILL_RAS_SERVER_CONTEXT_BASE_H
#define LOGICMILL_RAS_SERVER_CONTEXT_BASE_H

#include <cstdint>
#include <functional>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/bstream/ombstream.h>
#include <logicmill/ras/interface_stub_base.h>
#include <memory>
#include <unordered_set>

namespace logicmill
{
namespace ras
{
class server_context_base
{
public:
	using listener_error_handler = std::function<void(std::error_code ec)>;
	using channel_error_handler  = std::function<void(async::channel::ptr const& ptr, std::error_code ec)>;
	using close_handler          = std::function<void()>;

	server_context_base(std::size_t interface_count, async::loop::ptr lp, bstream::context_base const& stream_context);

	virtual ~server_context_base();

	template<class T, class U>
	typename std::enable_if_t<
			std::is_convertible<T, listener_error_handler>::value
			&& std::is_convertible<U, channel_error_handler>::value>
	bind(async::options const& opts, std::error_code& err, T&& on_listener_error, U&& on_channel_error)
	{
		async::options opts_override{opts};
		opts_override.framing(true);
		m_on_listener_error = std::forward<T>(on_listener_error);
		m_on_channel_error  = std::forward<U>(on_channel_error);
		really_bind(opts_override, err);
	}

	void
	cleanup()
	{
		if (m_on_close)
		{
			m_on_listener_error = nullptr;
			m_on_channel_error  = nullptr;
			m_on_close();
			m_on_close = nullptr;
		}
	}

	template<class T>
	typename std::enable_if_t<std::is_convertible<T, close_handler>::value>
	close(T&& handler)
	{
		m_on_close = std::forward<T>(handler);
		really_close();
	}

	void
	really_close()
	{
		if (m_listener)
		{
			m_listener->close([=](async::listener::ptr const& lp) {
				m_listener.reset();
				if (m_open_channels.empty())
				{
					cleanup();
				}
			});
		}
		for (auto& chan : m_open_channels)
		{
			chan->close([=](async::channel::ptr const& c) {
				m_open_channels.erase(c);
				if (m_open_channels.empty() && !m_listener)
				{
					cleanup();
				}
			});
		}
	}

	std::unique_ptr<bstream::ombstream>
	create_reply_stream()
	{
		return std::make_unique<bstream::ombstream>(m_stream_context);
	}

	bstream::context_base&
	stream_context()
	{
		return m_stream_context;
	}

	inline interface_stub_base&
	get_stub(std::size_t index)
	{
		return *(m_stubs[index]);
	}

	inline std::shared_ptr<void>
	get_impl(std::size_t index)
	{
		return m_impl_ptrs[index];
	}

	inline void
	set_impl(std::size_t index, std::shared_ptr<void> ptr)
	{
		m_impl_ptrs[index] = ptr;
	}

	void
	send_reply(async::channel::ptr const& chan, bstream::mutable_buffer&& buf);

	void
	handle_request(bstream::ibstream& is, async::channel::ptr const& chan);

protected:
	void
	really_bind(async::options const& opts, std::error_code& err);

	async::loop::ptr                                  m_loop;
	bstream::cloned_context                           m_stream_context;
	async::listener::ptr                              m_listener;
	std::vector<std::unique_ptr<interface_stub_base>> m_stubs;
	std::vector<std::shared_ptr<void>>                m_impl_ptrs;
	channel_error_handler                             m_on_channel_error;
	listener_error_handler                            m_on_listener_error;
	close_handler                                     m_on_close;
	std::unordered_set<async::channel::ptr>           m_open_channels;
};

}    // namespace ras
}    // namespace logicmill


#endif /* LOGICMILL_RAS_SERVER_CONTEXT_BASE_H */
