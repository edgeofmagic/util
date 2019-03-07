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

#ifndef LOGICMILL_ARMI_METHOD_PROXY_H
#define LOGICMILL_ARMI_METHOD_PROXY_H

#include <logicmill/armi/client_context_base.h>
#include <logicmill/armi/reply_handler.h>
#include <logicmill/armi/types.h>
#include <logicmill/bstream/ombstream.h>
#include <logicmill/util/promise.h>
#include <type_traits>

namespace logicmill
{
namespace armi
{


template<class MemberFuncPtr>
class member_func_proxy;

template<class Target, class PromiseType, class... Args>
class member_func_proxy<util::promise<PromiseType> (Target::*)(Args...)>
{
public:
	using reply_handler_type = reply_handler<util::promise<PromiseType>>;

	member_func_proxy(client_context_base& context, std::size_t member_func_id)
		: m_context{context}, m_member_func_id{member_func_id}
	{}

	util::promise<PromiseType>
	operator()(Args... args) const
	{
		auto                       timeout = get_timeout();
		util::promise<PromiseType> p;
		auto                       request_id = m_context.next_request_id();
		m_context.add_handler(request_id, std::make_unique<reply_handler_type>(p));

		bstream::ombstream os{m_context.stream_context()};
		os << request_id << m_member_func_id;
		os.write_array_header(sizeof...(Args));    // item_count
		append(os, args...);
		m_context.send_request(
				m_context.get_and_clear_transient_channel_id(), request_id, os.release_mutable_buffer(), timeout);
		return p;
	}

	std::chrono::milliseconds
	get_timeout() const
	{
		millisecs timeout{m_context.get_and_clear_transient_timeout()};
		if (timeout.count() <= 0)
		{
			timeout = m_context.get_default_timeout();
		}
		return timeout;
	}

private:
	void
	append(bstream::ombstream& os) const
	{}

	template<class First_, class... More_>
	void
	append(bstream::ombstream& os, First_ first, More_... more) const
	{
		os << first;
		append(os, more...);
	}

	client_context_base& m_context;
	std::size_t          m_member_func_id;
};

template<class T>
class stripped_member_func_proxy : public member_func_proxy<typename traits::remove_member_func_cv_noexcept<T>::type>
{
public:
	using base = member_func_proxy<typename traits::remove_member_func_cv_noexcept<T>::type>;
	stripped_member_func_proxy(client_context_base& context, std::size_t member_func_id) : base{context, member_func_id}
	{}
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_METHOD_PROXY_H
