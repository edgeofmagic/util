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
#include <logicmill/armi/traits.h>
#include <logicmill/armi/types.h>
#include <logicmill/bstream/ombstream.h>

namespace logicmill
{
namespace armi
{

template<class Reply>
class method_proxy_base
{
protected:
	using reply_hndlr_type = reply_handler<Reply>;

	method_proxy_base(client_context_base& context, std::size_t interface_id, std::size_t method_id)
		: m_context{context}, m_interface_id{interface_id}, m_method_id{method_id}
	{}

	std::chrono::milliseconds
	get_timeout()
	{
		auto timeout = m_context.transient_timeout();
		if (timeout.count() <= 0)
		{
			timeout = m_context.default_timeout();
		}
		else
		{
			m_context.clear_transient_timeout();
		}
		return timeout;
	}

	std::uint64_t
	add_handler(Reply reply)
	{
		auto req_ord = m_context.next_request_ordinal();
		m_context.add_handler(req_ord, std::make_unique<reply_hndlr_type>(reply));
		return req_ord;
	}

	void
	send_request(std::uint64_t req_ord, bstream::ombstream& os, std::chrono::milliseconds timeout)
	{
		m_context.send_request(req_ord, os, timeout);
	}

	bstream::context_base const&
	stream_context() const
	{
		return m_context.stream_context();
	}


	std::size_t          m_interface_id;
	std::size_t          m_method_id;
	client_context_base& m_context;
};

template<class F, class Enable = void>
class method_proxy;

/*
 * Form 1 - An error-safe reply, no other parameters
 */
template<class Target, class Reply>
class method_proxy<
		void (Target::*)(Reply),
		std::enable_if_t<is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value>>
	: public method_proxy_base<Reply>
{
public:
	using base = method_proxy_base<Reply>;

	method_proxy(client_context_base& context, std::size_t interface_id, std::size_t method_id)
		: method_proxy_base<Reply>{context, interface_id, method_id}
	{}

	void
	operator()(Reply reply)
	{
		auto timeout = base::get_timeout();
		auto req_ord = base::add_handler(reply);

		bstream::ombstream os{base::stream_context()};

		os << req_ord << base::m_interface_id << base::m_method_id;
		os.write_array_header(0);
		base::send_request(req_ord, os, timeout);
	}
};

/*
 * Form 2 - An error-safe reply and one other parameter, which is not fail_reply)
 */
template<class Target, class Reply, class First>
class method_proxy<
		void (Target::*)(Reply, First),
		std::enable_if_t<
				!std::is_same<First, fail_reply>::value
				&& (is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value)>>
	: method_proxy_base<Reply>
{
public:
	using base = method_proxy_base<Reply>;

	method_proxy(client_context_base& context, std::size_t interface_id, std::size_t method_id)
		: method_proxy_base<Reply>(context, interface_id, method_id)
	{}

	void
	operator()(Reply reply, First first)
	{
		auto               timeout = base::get_timeout();
		auto               req_ord = base::add_handler(reply);
		bstream::ombstream os{base::stream_context()};

		os << req_ord << base::m_interface_id << base::m_method_id;
		os.write_array_header(1);
		os << first;

		base::send_request(req_ord, os, timeout);
	}
};

/*
 * Form 3 - An error-safe reply and multiple parameters, where the 
 * first is not fail_reply
 */
template<class Target, class Reply, class First, class... Args>
class method_proxy<
		void (Target::*)(Reply, First, Args...),
		std::enable_if_t<
				!std::is_same<First, fail_reply>::value
				&& (is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value)>>
	: public method_proxy_base<Reply>
{
public:
	using base = method_proxy_base<Reply>;

	method_proxy(client_context_base& context, std::size_t interface_id, std::size_t method_id)
		: method_proxy_base<Reply>(context, interface_id, method_id)
	{}

	void
	operator()(Reply reply, First first, Args... args)
	{
		auto               timeout = base::get_timeout();
		auto               req_ord = base::add_handler(reply);
		bstream::ombstream os{base::stream_context()};

		os << req_ord << base::m_interface_id << base::m_method_id;

		os.write_array_header(1 + sizeof...(Args));
		os << first;
		append(os, args...);

		base::send_request(req_ord, std::move(os), timeout);
	}

private:
	void
	append(bstream::ombstream& os)
	{}

	template<class First_, class... More_>
	void
	append(bstream::ombstream& os, First_ first, More_... more)
	{
		os << first;
		append(os, more...);
	}
};

/*
 * Form 4 - A reply (not necessarily error_safe), and an error_reply (no other parameters).
 */
template<class Target, class Reply>
class method_proxy<void (Target::*)(Reply, fail_reply)> : public method_proxy_base<Reply>
{
public:
	using base = method_proxy_base<Reply>;

	method_proxy(client_context_base& context, std::size_t interface_id, std::size_t method_id)
		: method_proxy_base<Reply>(context, interface_id, method_id)
	{}

	void
	operator()(Reply reply, fail_reply fail)
	{
		auto               timeout = base::get_timeout();
		auto               req_ord = base::add_handler(reply);
		bstream::ombstream os{base::stream_context()};

		os << req_ord << base::m_interface_id << base::m_method_id;
		os.write_array_header(0);

		base::send_request(req_ord, os, timeout);
	}
};

/*
 * Form 5 - A reply (not necessarily error_safe), an error_reply, and additional parameters.
 */
template<class Target, class Reply, class... Args>
class method_proxy<void (Target::*)(Reply, fail_reply, Args...)> : public method_proxy_base<Reply>
{
public:
	using base = method_proxy_base<Reply>;

	method_proxy(client_context_base& context, std::size_t interface_id, std::size_t method_id)
		: method_proxy_base<Reply>(context, interface_id, method_id)
	{}

	void
	operator()(Reply reply, fail_reply freply, Args... args)
	{
		auto               timeout = base::get_timeout();
		auto               req_ord = base::add_handler(reply);
		bstream::ombstream os{base::stream_context()};

		os << req_ord << base::m_interface_id << base::m_method_id;
		os.write_array_header(sizeof...(Args));    // item_count
		append(os, args...);

		base::send_request(req_ord, os, timeout);
	}

private:
	void
	append(bstream::ombstream& os)
	{}

	template<class First_, class... More_>
	void
	append(bstream::ombstream& os, First_ first, More_... more)
	{
		os << first;
		append(os, more...);
	}
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_METHOD_PROXY_H
