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
 * File:   reply_handler.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 9:43 PM
 */

#ifndef LOGICMILL_ARMI_REPLY_HANDLER_H
#define LOGICMILL_ARMI_REPLY_HANDLER_H

#include <logicmill/armi/reply_handler_base.h>
#include <logicmill/armi/reply_stub.h>

namespace logicmill
{
namespace armi
{

template<class Reply, class Fail = void, class Enable = void>
class reply_handler;

template<class Reply, class Fail>
class reply_handler<Reply, Fail, typename std::enable_if_t<std::is_void<Fail>::value>> : public reply_handler_base
{
public:
	inline reply_handler(Reply reply)
		: m_reply_stub{reply_stub<Reply>(reply)}
	{}

	virtual void
	handle_reply(bstream::ibstream& is) override
	{
		reply_kind rk = is.read_as<reply_kind>();
		if (rk == reply_kind::fail)
		{
			auto item_count = is.read_array_header();
			if (!expected_count<1>(item_count))
			{
				cancel(make_error_code(armi::errc::invalid_argument_count));
			}
			else
			{
				cancel(is.read_as<std::error_code>());
			}
		}
		else
		{
			m_reply_stub(is);
		}
	}

	virtual void
	cancel(std::error_code ec) override
	{
		m_reply_stub.cancel(ec);
	}

private:
	reply_stub<Reply>    m_reply_stub;
};

template<class Reply, class Fail>
class reply_handler<Reply, Fail, typename std::enable_if_t<!std::is_void<Fail>::value>> : public reply_handler_base
{
public:
	reply_handler(Reply reply, Fail fail)
		: m_reply{reply_stub<Reply>(reply)}, m_fail{fail}
	{}

	virtual void
	handle_reply(bstream::ibstream& is) override
	{
		reply_kind rk = is.read_as<reply_kind>();
		if (rk == reply_kind::fail)
		{
			m_fail(is);
		}
		else
		{
			// note: this reply_stub (m_reply) can't cancel itself or return an error,
			// so failure (e.g., invalid argument count) will throw an exception
			try
			{
				m_reply(is);
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

	virtual void
	cancel(std::error_code ec) override
	{
		m_fail.cancel(ec);
	}

private:
	reply_stub<Reply> m_reply;
	fail_stub         m_fail;
};

}    // namespace armi
}    // namespace logicmill

#endif /* LOGICMILL_ARMI_REPLY_HANDLER_H */
