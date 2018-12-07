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
 * File:   reply_stub.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 9:37 PM
 */

#ifndef LOGICMILL_RAS_REPLY_STUB_H
#define LOGICMILL_RAS_REPLY_STUB_H

#include <system_error>
#include <functional>
#include <type_traits>
#include <logicmill/ras/client_context_base.h>
#include <logicmill/ras/error.h>
// #include <logicmill/ras/error_code_adaptor.h>
#include <logicmill/bstream/ibstream.h>


namespace logicmill
{
namespace ras
{
	template<class T>
	class reply_stub;

	template<class... Args>
	class reply_stub<std::function< void (std::error_code, Args...)>>
	{
	public:
		
		using reply_type = std::function< void (std::error_code, Args...)>;

		inline reply_stub(reply_type reply)
			: m_reply{reply}
		{}

		inline void
		cancel(std::error_code ec)
		{
			m_reply(ec, Args{}...);
			// This is why the parameters of a reply must be default_constructible
			// when the method doesn't have a fail reply (in case you were wondering).
		}

		void
		operator()(bstream::ibstream & is)
		{
			std::error_code err;
			auto item_count = is.read_array_header(err);
			if (err)
			{
				cancel(err);
			}
			else
			{
				if (!expected_count<sizeof...(Args) + 1 > (item_count))
				{
					cancel(make_error_code(ras::errc::invalid_argument_count));
				}
				else
				{
					try
					{
						std::error_code ec = is.read_as<std::error_code>();
						invoke(ec, is.read_as<typename std::remove_const_t<typename std::remove_reference_t<Args>>> ()...);
					}
					catch (std::system_error const& e)
					{
						cancel(e.code());
					}
					catch (std::exception const& e)
					{
						cancel(make_error_code(ras::errc::exception_thrown_by_reply_handler));
					}
				}
			}
		}

		inline void
		invoke(std::error_code ec, Args ...args)
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
	class reply_stub<std::function< void (Args...)>>
	{
	public:
		
		using reply_type = std::function< void (Args...)>;

		inline
		reply_stub(reply_type reply) : m_reply{reply}
		{}

		void
		operator()(bstream::ibstream& is)
		{
			auto item_count = is.read_array_header();

			// this reply stub throws exceptions, because it has no cancel method
			// they are caught by the calling reply_handler
			if (!expected_count<sizeof...(Args)>(item_count))
			{
				throw std::system_error{make_error_code(ras::errc::invalid_argument_count)};
			}
			else
			{
				invoke(is.read_as<typename std::remove_const_t<typename std::remove_reference_t<Args>>>()...);
			}
		}

		inline void
		invoke(Args ...args)
		{
			m_reply(args...);
		}

	private:
		
		reply_type m_reply;
	};

	class fail_stub
	{
	public:
		
		inline
		fail_stub(fail_reply reply) : m_reply{reply}
		{}

		inline void
		cancel(std::error_code ec)
		{
			m_reply(ec);
		}

		inline void
		operator()(bstream::ibstream& is)
		{
			std::error_code err;
			auto item_count = is.read_array_header(err);
			if (err)
			{
				m_reply(err);
				goto exit;
			}
			if (!expected_count<1>(item_count))
			{
				m_reply(make_error_code(ras::errc::invalid_argument_count));
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

} // namespace ras
} // namespace logicmill


#endif /* LOGICMILL_RAS_REPLY_STUB_H */

