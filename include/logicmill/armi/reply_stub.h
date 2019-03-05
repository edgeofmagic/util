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

#ifndef LOGICMILL_ARMI_REPLY_STUB_H
#define LOGICMILL_ARMI_REPLY_STUB_H

#include <functional>
#include <logicmill/armi/client_context_base.h>
#include <logicmill/armi/error.h>
#include <system_error>
#include <type_traits>
#include <logicmill/bstream/ibstream.h>
#include <logicmill/util/promise.h>


namespace logicmill
{
namespace armi
{

template<class T, class Enable = void>
class reply_stub;

template<class PromiseType>
class reply_stub<util::promise<PromiseType>, typename std::enable_if_t<!std::is_void<PromiseType>::value>>
{
public:
	using reply_type = util::promise<PromiseType>;

	reply_stub(reply_type reply) : m_reply{reply} {}

	void
	cancel(std::error_code err)
	{
		m_reply.reject(err);
	}

	void
	operator()(bstream::ibstream& is)
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
		{
			cancel(err);
		}
		else
		{
			if (!expected_count<1>(item_count))
			{
				cancel(make_error_code(armi::errc::invalid_argument_count));
			}
			else
			{
				try
				{
					m_reply.resolve(is.read_as<typename std::remove_const_t<typename std::remove_reference_t<PromiseType>>>());
				}
				catch (std::system_error const& e)
				{
					cancel(e.code());
				}
				catch (std::exception const& e)
				{
					cancel(make_error_code(armi::errc::exception_thrown_by_reply_handler));
				}
			}
		}

	}

 private:
	reply_type m_reply;

 };

template<class PromiseType>
class reply_stub<util::promise<PromiseType>, typename std::enable_if_t<std::is_void<PromiseType>::value>>
{
public:
	using reply_type = util::promise<void>;

	reply_stub(reply_type reply) : m_reply{reply} {}

	void
	cancel(std::error_code err)
	{
		m_reply.reject(err);
	}

	void
	operator()(bstream::ibstream& is)
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
		{
			cancel(err);
		}
		else
		{
			if (!expected_count<0>(item_count))
			{
				cancel(make_error_code(armi::errc::invalid_argument_count));
			}
			else
			{
				m_reply.resolve();
			}
		}

	}

 private:
	reply_type m_reply;

 };



template<class... Args>
class reply_stub<std::function<void(std::error_code, Args...)>>
{
public:
	using reply_type = std::function<void(std::error_code, Args...)>;

	reply_stub(reply_type reply) : m_reply{reply} {}

	void
	cancel(std::error_code ec)
	{
		m_reply(ec, Args{}...);
		// This is why the parameters of a reply must be default_constructible
		// when the method doesn't have a fail reply (in case you were wondering).
	}

	void
	operator()(bstream::ibstream& is)
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
		{
			cancel(err);
		}
		else
		{
			if (!expected_count<sizeof...(Args) + 1>(item_count))
			{
				cancel(make_error_code(armi::errc::invalid_argument_count));
			}
			else
			{
				try
				{
					std::error_code ec = is.read_as<std::error_code>();
					invoke(ec, is.read_as<typename std::remove_const_t<typename std::remove_reference_t<Args>>>()...);
				}
				catch (std::system_error const& e)
				{
					cancel(e.code());
				}
				catch (std::exception const& e)
				{
					cancel(make_error_code(armi::errc::exception_thrown_by_reply_handler));
				}
			}
		}
	}

	void
	invoke(std::error_code ec, Args... args)
	{
		m_reply(ec, args...);
	}

private:
	reply_type m_reply;
};

/*
 *	This specialization is used for replies in signatures that have a 
 *	fail lambda; it omits the error_code parameter. On errors, system_error
 *	exceptions are thrown, to be caught by the calling reply_handler.
 */
template<class... Args>
class reply_stub<std::function<void(Args...)>>
{
public:
	using reply_type = std::function<void(Args...)>;

	inline reply_stub(reply_type reply) : m_reply{reply} {}

	void
	operator()(bstream::ibstream& is)
	{
		auto item_count = is.read_array_header();

		// this reply stub throws exceptions, because it has no cancel method
		// they are caught by the calling reply_handler
		if (!expected_count<sizeof...(Args)>(item_count))
		{
			throw std::system_error{make_error_code(armi::errc::invalid_argument_count)};
		}
		else
		{
			invoke(is.read_as<typename std::remove_const_t<typename std::remove_reference_t<Args>>>()...);
		}
	}

	void
	invoke(Args... args)
	{
		m_reply(args...);
	}

private:
	reply_type m_reply;
};

class fail_stub
{
public:
	fail_stub(fail_reply reply) : m_reply{reply} {}

	void
	cancel(std::error_code ec)
	{
		m_reply(ec);
	}

	void
	operator()(bstream::ibstream& is)
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
		{
			m_reply(err);
			goto exit;
		}
		if (!expected_count<1>(item_count))
		{
			m_reply(make_error_code(armi::errc::invalid_argument_count));
			goto exit;
		}

		{
			std::error_code err_from_stream = is.read_as<std::error_code>(err);
			if (err)
			{
				m_reply(err);
			}
			else
			{
				m_reply(err_from_stream);
			}
		}
	exit:
		return;
	}

private:
	fail_reply m_reply;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_REPLY_STUB_H
