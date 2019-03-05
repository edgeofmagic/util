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
#include <logicmill/util/promise.h>

namespace logicmill
{
namespace armi
{

#if 1
template<class PromiseType, class Fail>
class method_proxy_base<util::promise<PromiseType>, Fail, 
	std::enable_if_t<!std::is_void<PromiseType>::value && std::is_void<Fail>::value>>
{
protected:
	using promise_type = PromiseType;
	using reply_handler_type = reply_handler<util::promise<promise_type>>;

	method_proxy_base(client_context_base& context, std::size_t method_id)
	: m_context{context}, m_method_id{method_id}
	{}

	std::chrono::milliseconds
	get_timeout()
	{
		millisecs timeout{m_context.get_and_clear_transient_timeout()};
		if (timeout.count() <= 0)
		{
			timeout = m_context.get_default_timeout();
		}
		return timeout;
	}

	request_id_type
	add_handler(util::promise<promise_type> p)
	{
		auto request_id = m_context.next_request_id();
		m_context.add_handler(request_id, std::make_unique<reply_handler_type>(p));

		return request_id;
	}

	void
	send_request(request_id_type request_id, util::mutable_buffer&& req, std::chrono::milliseconds timeout)
	{
		m_context.send_request(m_context.get_and_clear_transient_channel_id(), request_id, std::move(req), timeout);
	}

	bstream::context_base::ptr
	stream_context() const
	{
		return m_context.stream_context();
	}

	client_context_base&
	context() const
	{
		return m_context;
	}

	std::size_t          m_method_id;
	client_context_base& m_context;

};
#endif

#if 1
template<class PromiseType, class Fail>
class method_proxy_base<util::promise<PromiseType>, Fail, 
	std::enable_if_t<std::is_void<PromiseType>::value && std::is_void<Fail>::value>>
{
protected:
	using reply_handler_type = reply_handler<util::promise<void>>;

	method_proxy_base(client_context_base& context, std::size_t method_id)
	: m_context{context}, m_method_id{method_id}
	{}

	std::chrono::milliseconds
	get_timeout()
	{
		millisecs timeout{m_context.get_and_clear_transient_timeout()};
		if (timeout.count() <= 0)
		{
			timeout = m_context.get_default_timeout();
		}
		return timeout;
	}

	request_id_type
	add_handler(util::promise<void> p)
	{
		auto request_id = m_context.next_request_id();
		m_context.add_handler(request_id, std::make_unique<reply_handler_type>(p));

		return request_id;
	}

	void
	send_request(request_id_type request_id, util::mutable_buffer&& req, std::chrono::milliseconds timeout)
	{
		m_context.send_request(m_context.get_and_clear_transient_channel_id(), request_id, std::move(req), timeout);
	}

	bstream::context_base::ptr
	stream_context() const
	{
		return m_context.stream_context();
	}

	client_context_base&
	context() const
	{
		return m_context;
	}

	std::size_t          m_method_id;
	client_context_base& m_context;

};
#endif

template<class Reply, class Fail>
class method_proxy_base<
		Reply,
		Fail,
		std::enable_if_t<!std::is_void<Fail>::value && std::is_same<Fail, fail_reply>::value
		&& !util::is_promise<Reply>::value>>
{
protected:
	using reply_hndlr_type = reply_handler<Reply, fail_reply>;

	method_proxy_base(client_context_base& context, std::size_t method_id)
		: m_context{context}, m_method_id{method_id}
	{}

	std::chrono::milliseconds
	get_timeout()
	{
		millisecs timeout{m_context.get_and_clear_transient_timeout()};
		if (timeout.count() <= 0)
		{
			timeout = m_context.get_default_timeout();
		}
		return timeout;
	}

	request_id_type
	add_handler(Reply reply, fail_reply fail)
	{
		auto request_id = m_context.next_request_id();
		m_context.add_handler(request_id, std::make_unique<reply_hndlr_type>(reply, fail));
		return request_id;
	}

	void
	send_request(request_id_type request_id, util::mutable_buffer&& req, std::chrono::milliseconds timeout)
	{
		m_context.send_request(m_context.get_and_clear_transient_channel_id(), request_id, std::move(req), timeout);
	}

	bstream::context_base::ptr
	stream_context() const
	{
		return m_context.stream_context();
	}

	client_context_base&
	context() const
	{
		return m_context;
	}

	std::size_t          m_method_id;
	client_context_base& m_context;
};


template<class Reply, class Fail>
class method_proxy_base<Reply, Fail, std::enable_if_t<std::is_void<Fail>::value && !util::is_promise<Reply>::value>>
{
protected:
	using reply_hndlr_type = reply_handler<Reply>;

	method_proxy_base(client_context_base& context, std::size_t method_id)
		: m_context{context}, m_method_id{method_id}
	{}

	std::chrono::milliseconds
	get_timeout()
	{
		millisecs timeout{m_context.get_and_clear_transient_timeout()};
		if (timeout.count() <= 0)
		{
			timeout = m_context.get_default_timeout();
		}
		return timeout;
	}

	request_id_type
	add_handler(Reply reply)
	{
		auto request_id = m_context.next_request_id();
		m_context.add_handler(request_id, std::make_unique<reply_hndlr_type>(reply));
		return request_id;
	}

	void
	send_request(request_id_type request_id, util::mutable_buffer&& req, std::chrono::milliseconds timeout)
	{
		m_context.send_request(m_context.get_and_clear_transient_channel_id(), request_id, std::move(req), timeout);
	}

	bstream::context_base::ptr
	stream_context() const
	{
		return m_context.stream_context();
	}

	client_context_base&
	context() const
	{
		return m_context;
	}

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
		std::enable_if_t<(is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value) && !util::is_promise<Reply>::value>>
	: public method_proxy_base<Reply>
{
public:
	using base = method_proxy_base<Reply>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<Reply>{context, method_id}
	{}

	void
	operator()(Reply reply)
	{
		auto timeout = base::get_timeout();
		auto request_id = base::add_handler(reply);

		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;
		os.write_array_header(0);
		base::send_request(request_id, os.release_buffer(), timeout);
	}
};

/*
 * Form 2 - An error-safe reply and one other parameter, which is not fail_reply)
 */
template<class Target, class Reply, class First>
class method_proxy<
		void (Target::*)(Reply, First),
		std::enable_if_t<
				!std::is_same<First, fail_reply>::value && !util::is_promise<Reply>::value
				&& (is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value)>>
	: method_proxy_base<Reply>
{
public:
	using base = method_proxy_base<Reply>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<Reply>(context, method_id)
	{}

	void
	operator()(Reply reply, First first)
	{
		auto               timeout = base::get_timeout();
		auto               request_id = base::add_handler(reply);
		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;
		os.write_array_header(1);
		os << first;

		base::send_request(request_id, os.release_buffer(), timeout);
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
				!std::is_same<First, fail_reply>::value && !util::is_promise<Reply>::value
				&& (is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value)>>
	: public method_proxy_base<Reply>
{
public:
	using base = method_proxy_base<Reply>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<Reply>(context, method_id)
	{}

	void
	operator()(Reply reply, First first, Args... args)
	{
		auto               timeout = base::get_timeout();
		auto               request_id = base::add_handler(reply);
		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;

		os.write_array_header(1 + sizeof...(Args));
		os << first;
		append(os, args...);

		base::send_request(request_id, os.release_buffer(), timeout);
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
class method_proxy<void (Target::*)(Reply, fail_reply), std::enable_if_t<!util::is_promise<Reply>::value>> : public method_proxy_base<Reply, fail_reply>
{
public:
	using base = method_proxy_base<Reply, fail_reply>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<Reply, fail_reply>(context, method_id)
	{}

	void
	operator()(Reply reply, fail_reply fail)
	{
		auto               timeout = base::get_timeout();
		auto               request_id = base::add_handler(reply, fail);
		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;
		os.write_array_header(0);

		base::send_request(request_id, os.release_buffer(), timeout);
	}
};

/*
 * Form 5 - A reply (not necessarily error_safe), an error_reply, and additional parameters.
 */
template<class Target, class Reply, class... Args>
class method_proxy<void (Target::*)(Reply, fail_reply, Args...), std::enable_if_t<!util::is_promise<Reply>::value>> : public method_proxy_base<Reply, fail_reply>
{
public:
	using base = method_proxy_base<Reply, fail_reply>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<Reply, fail_reply>(context, method_id)
	{}

	void
	operator()(Reply reply, fail_reply fail, Args... args)
	{
		auto               timeout = base::get_timeout();
		auto               request_id = base::add_handler(reply, fail);
		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;
		os.write_array_header(sizeof...(Args));    // item_count
		append(os, args...);

		base::send_request(request_id, os.release_buffer(), timeout);
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

#if 1

/*
 * Form 6 - A non-void promise return value, no parameters
 */
template<class Target, class PromiseType>
class method_proxy<
		util::promise<PromiseType> (Target::*)(),
		std::enable_if_t<!std::is_void<PromiseType>::value>>
	: public method_proxy_base<util::promise<PromiseType>>
{
public:
	using base = method_proxy_base<util::promise<PromiseType>>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<util::promise<PromiseType>>{context, method_id}
	{}

	util::promise<PromiseType>
	operator()()
	{
		auto timeout = base::get_timeout();
		util::promise<PromiseType> p;
		auto request_id = base::add_handler(p);

		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;
		os.write_array_header(0);
		base::send_request(request_id, os.release_mutable_buffer(), timeout);
		return p;
	}
};

#endif

#if 1

/*
 * Form 7 - A non-void promise return value, with parameters
 */
template<class Target, class PromiseType, class... Args>
class method_proxy<
		util::promise<PromiseType> (Target::*)(Args...),
		std::enable_if_t<!std::is_void<PromiseType>::value>>
	: public method_proxy_base<util::promise<PromiseType>>
{
public:
	using base = method_proxy_base<util::promise<PromiseType>>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<util::promise<PromiseType>>{context, method_id}
	{}

	util::promise<PromiseType>
	operator()(Args... args)
	{
		auto timeout = base::get_timeout();
		util::promise<PromiseType> p;
		auto request_id = base::add_handler(p);

		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;

		os.write_array_header(sizeof...(Args));    // item_count
		append(os, args...);

		base::send_request(request_id, os.release_mutable_buffer(), timeout);
		return p;
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

#endif

#if 1

/*
 * Form 8 - A void promise return value, no parameters
 */
template<class Target, class PromiseType>
class method_proxy<
		util::promise<PromiseType> (Target::*)(),
		std::enable_if_t<std::is_void<PromiseType>::value>>
	: public method_proxy_base<util::promise<void>>
{
public:
	using base = method_proxy_base<util::promise<void>>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<util::promise<void>>{context, method_id}
	{}

	util::promise<void>
	operator()()
	{
		auto timeout = base::get_timeout();
		util::promise<void> p;
		auto request_id = base::add_handler(p);

		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;
		os.write_array_header(0);
		base::send_request(request_id, os.release_mutable_buffer(), timeout);
		return p;
	}
};

#endif

#if 1

/*
 * Form 9 - A void promise return value, with parameters
 */
template<class Target, class PromiseType, class... Args>
class method_proxy<
		util::promise<PromiseType> (Target::*)(Args...),
		std::enable_if_t<std::is_void<PromiseType>::value>>
	: public method_proxy_base<util::promise<void>>
{
public:
	using base = method_proxy_base<util::promise<void>>;

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<util::promise<void>>{context, method_id}
	{}

	util::promise<PromiseType>
	operator()(Args... args)
	{
		auto timeout = base::get_timeout();
		util::promise<void> p;
		auto request_id = base::add_handler(p);

		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;

		os.write_array_header(sizeof...(Args));    // item_count
		append(os, args...);

		base::send_request(request_id, os.release_mutable_buffer(), timeout);
		return p;
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

#endif


#if 0

/*
 * Form 6 - Promise return value and 0 parameters 
 *
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

	method_proxy(client_context_base& context, std::size_t method_id)
		: method_proxy_base<Reply>(context, method_id)
	{}

	void
	operator()(Reply reply, First first, Args... args)
	{
		auto               timeout = base::get_timeout();
		auto               request_id = base::add_handler(reply);
		bstream::ombstream os{base::stream_context()};

		os << request_id << base::m_method_id;

		os.write_array_header(1 + sizeof...(Args));
		os << first;
		append(os, args...);

		base::send_request(request_id, std::move(os), timeout);
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

#endif


}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_METHOD_PROXY_H
