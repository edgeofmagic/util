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
 * File:   reply_proxy.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 9:49 PM
 */

#ifndef LOGICMILL_RAS_REPLY_PROXY_H
#define LOGICMILL_RAS_REPLY_PROXY_H

#include <logicmill/ras/server_context_base.h>
#include <system_error>
#include <logicmill/ras/types.h>

namespace logicmill
{
namespace ras
{

template<class T>
class reply_proxy;

template<class... Args>
class reply_proxy<std::function<void(std::error_code, Args...)>>
{
public:
	reply_proxy(server_context_base& context, std::uint64_t req_ord, async::channel::ptr const& chan)
		: m_context{context}, m_req_ord{req_ord}, m_channel{chan}
	{}

	reply_proxy(reply_proxy const& other)
		: m_context{other.m_context}, m_req_ord{other.m_req_ord}, m_channel{other.m_channel}
	{}

	inline void
	operator()(std::error_code ec, Args... args)
	{
		bstream::ombstream os{m_context.stream_context()};
		os << m_req_ord;
		os << reply_kind::normal;
		os.write_array_header(sizeof...(Args) + 1);
		os << ec;
		append(os, args...);
		m_context.send_reply(m_channel, os.release_mutable_buffer());

		// auto os = m_context.create_reply_stream();
		// *os << m_req_ord;
		// *os << reply_kind::normal;
		// os->write_array_header(sizeof...(Args) + 1);
		// *os << ec;
		// append(*os, args...);
		// m_context.send_reply(m_channel, os->release_mutable_buffer());
	}

private:
	void
	append(bstream::obstream& os)
	{}

	template<class First, class... More>
	void
	append(bstream::obstream& os, First first, More... more)
	{
		os << first;
		append(os, more...);
	}

	server_context_base& m_context;
	std::uint64_t        m_req_ord;
	async::channel::ptr  m_channel;
};

template<class... Args>
class reply_proxy<std::function<void(Args...)>>
{
public:
	reply_proxy(server_context_base& context, std::uint64_t req_ord, async::channel::ptr const& chan)
		: m_context{context}, m_req_ord{req_ord}, m_channel{chan}
	{}

	reply_proxy(reply_proxy const& other)
		: m_context{other.m_context}, m_req_ord{other.m_req_ord}, m_channel{other.m_channel}
	{}

	inline void
	operator()(Args... args)
	{
		bstream::ombstream os{m_context.stream_context()};
		os << m_req_ord;
		os << reply_kind::normal;
		os.write_array_header(sizeof...(Args));
		append(os, args...);
		m_context.send_reply(m_channel, os.release_mutable_buffer());


		// auto os = m_context.create_reply_stream();
		// *os << m_req_ord;
		// *os << reply_kind::normal;
		// os->write_array_header(sizeof...(Args));
		// append(*os, args...);
		// m_context.send_reply(m_channel, os->release_mutable_buffer());
	}

private:
	void
	append(bstream::obstream& os)
	{}

	template<class First, class... More>
	void
	append(bstream::obstream& os, First first, More... more)
	{
		os << first;
		append(os, more...);
	}

	server_context_base& m_context;
	std::uint64_t        m_req_ord;
	async::channel::ptr  m_channel;
};

}    // namespace ras
}    // namespace logicmill

#endif /* LOGICMILL_RAS_REPLY_PROXY_H */
