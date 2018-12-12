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

#ifndef LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H
#define LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H

#include <cstdint>
#include <functional>
#include <logicmill/armi/interface_stub_base.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/bstream/ombstream.h>
#include <memory>
#include <unordered_set>

namespace logicmill
{
namespace armi
{

class fail_proxy;

class server_context_base
{
public:
	using channel_error_handler = std::function<void(async::channel::ptr const& ptr, std::error_code ec)>;
	using close_handler         = std::function<void()>;

	server_context_base(std::size_t interface_count, async::loop::ptr lp, bstream::context_base const& stream_context);

	virtual ~server_context_base();

	template<class T>
	typename std::enable_if_t<std::is_convertible<T, close_handler>::value, bool>
	close(T&& handler)
	{
		m_on_close = std::forward<T>(handler);
		return really_close();
	}

	bool
	close()
	{
		m_on_close = nullptr;
		return really_close();
	}

	bstream::context_base&
	stream_context()
	{
		return m_stream_context;
	}

	void
	send_reply(async::channel::ptr const& chan, bstream::mutable_buffer&& buf);

	std::shared_ptr<void>
	get_impl(std::size_t index)
	{
		return m_impl_ptrs[index];
	}

protected:
	class error_handler_wrapper_base
	{
	public:
		virtual ~error_handler_wrapper_base() {}

		virtual void
		invoke(std::error_code err)
				= 0;
	};

	void
	on_listener_error_default()
	{
		close();
	}

	void
	on_channel_error_default(async::channel::ptr const& chan)
	{
		chan->close();
	}

	void
	cleanup();

	bool
	really_close();

	std::unique_ptr<bstream::ombstream>
	create_reply_stream()
	{
		return std::make_unique<bstream::ombstream>(m_stream_context);
	}

	interface_stub_base&
	get_stub(std::size_t index)
	{
		return *(m_stubs[index]);
	}

	void
	set_impl(std::size_t index, std::shared_ptr<void> ptr)
	{
		m_impl_ptrs[index] = ptr;
	}

	void
	handle_request(bstream::ibstream& is, async::channel::ptr const& chan);

	void
	really_bind(async::options const& opts, std::error_code& err);

	async::loop::ptr                                  m_loop;
	bstream::cloned_context                           m_stream_context;
	async::listener::ptr                              m_listener;
	std::vector<std::unique_ptr<interface_stub_base>> m_stubs;
	std::vector<std::shared_ptr<void>>                m_impl_ptrs;
	channel_error_handler                             m_on_channel_error;
	std::unique_ptr<error_handler_wrapper_base>       m_on_server_error;
	close_handler                                     m_on_close;
	std::unordered_set<async::channel::ptr>           m_open_channels;
};

}    // namespace armi
}    // namespace logicmill


#endif /* LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H */
