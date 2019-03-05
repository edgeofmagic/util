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

#ifndef LOGICMILL_ARMI_REPLY_PROXY_H
#define LOGICMILL_ARMI_REPLY_PROXY_H

#include <logicmill/armi/server_context_base.h>
#include <logicmill/armi/types.h>
#include <logicmill/bstream/context.h>
#include <system_error>
#include <logicmill/util/promise.h>

namespace logicmill
{
namespace armi
{

template<class T>
class reply_proxy;

template<class... Args>
class reply_proxy<std::function<void(std::error_code, Args...)>>
{
public:
	reply_proxy(request_id_type request_id, channel_id_type channel_id, server_context_base& context)
		: m_request_id{request_id}, m_channel_id{channel_id}, m_context{context}
	{}

	reply_proxy(reply_proxy const& other)
		: m_request_id{other.m_request_id}, m_channel_id{other.m_channel_id}, m_context{other.m_context}
	{}

	reply_proxy(reply_proxy&& other)
		: m_request_id{other.m_request_id}, m_channel_id{other.m_channel_id}, m_context{other.m_context}
	{}

	inline void
	operator()(std::error_code ec, Args... args)
	{
		bstream::ombstream os{m_context.stream_context()};
		os << m_request_id;
		os << reply_kind::normal;
		os.write_array_header(sizeof...(Args) + 1);
		os << ec;
		append(os, args...);
		m_context.get_transport().send_reply(m_channel_id, os.release_mutable_buffer());
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

	// server_context_base& m_context;
	request_id_type        m_request_id;
	channel_id_type m_channel_id;
	server_context_base& m_context;
};

template<class... Args>
class reply_proxy<std::function<void(Args...)>>
{
public:
	reply_proxy(request_id_type request_id, channel_id_type channel_id, server_context_base& context)
		: m_request_id{request_id}, m_channel_id{channel_id}, m_context{context}
	{}

	reply_proxy(reply_proxy const& other)
		: m_request_id{other.m_request_id}, m_channel_id{other.m_channel_id}, m_context{other.m_context}
	{}

	reply_proxy(reply_proxy&& other)
		: m_request_id{other.m_request_id}, m_channel_id{other.m_channel_id}, m_context{other.m_context}
	{}

	inline void
	operator()(Args... args)
	{
		bstream::ombstream os{m_context.stream_context()};
		os << m_request_id;
		os << reply_kind::normal;
		os.write_array_header(sizeof...(Args));
		append(os, args...);
		m_context.get_transport().send_reply(m_channel_id, os.release_mutable_buffer());
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

	// server_context_base& m_context;
	request_id_type        m_request_id;
	channel_id_type  m_channel_id;
	server_context_base& m_context;
};

/*
template<class PromiseType>
class reply_proxy<util::promise<PromiseType>>
{
public:
	reply_proxy(request_id_type request_id, channel_id_type channel_id, server_context_base& context)
		: m_request_id{request_id}, m_channel_id{channel_id}, m_context{context}
	{}

	reply_proxy(reply_proxy const& other)
		: m_request_id{other.m_request_id}, m_channel_id{other.m_channel_id}, m_context{other.m_context}
	{}

	reply_proxy(reply_proxy&& other)
		: m_request_id{other.m_request_id}, m_channel_id{other.m_channel_id}, m_context{other.m_context}
	{}

	inline void
	operator()(Args... args)
	{
		bstream::ombstream os{m_context.stream_context()};
		os << m_request_id;
		os << reply_kind::normal;
		os.write_array_header(sizeof...(Args));
		append(os, args...);
		m_context.get_transport().send_reply(m_channel_id, os.release_mutable_buffer());
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

	// server_context_base& m_context;
	request_id_type        m_request_id;
	channel_id_type  m_channel_id;
	server_context_base& m_context;
};

*/

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_REPLY_PROXY_H
