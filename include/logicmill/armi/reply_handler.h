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

#ifndef LOGICMILL_ARMI_REPLY_HANDLER_H
#define LOGICMILL_ARMI_REPLY_HANDLER_H

#include <logicmill/armi/reply_handler_base.h>
#include <logicmill/util/promise.h>

namespace logicmill
{
namespace armi
{

template<class Bridge, class PromiseType, class Enable = void>
class reply_handler;

template<class Bridge, class PromiseType>
class reply_handler<
		Bridge,
		util::promise<PromiseType>,
		typename std::enable_if_t<!std::is_void<PromiseType>::value>>
	: public reply_handler_base<Bridge>
{
public:
	using bridge_type             = Bridge;
	using serialization_traits    = typename bridge_type::serialization_traits;
	using deserializer_type       = typename serialization_traits::deserializer_type;
	using deserializer_param_type = typename bridge_type::deserializer_param_type;

	reply_handler(util::promise<PromiseType> p) : m_promise{p} {}

	virtual void
	handle_reply(deserializer_param_type reply) override
	{
		reply_kind rk = serialization_traits::template read<reply_kind>(reply);

		if (rk == reply_kind::fail)
		{
			auto item_count = serialization_traits::read_sequence_prefix(reply);

			if (!expected_count<1>(item_count))
			{
				cancel(make_error_code(armi::errc::invalid_argument_count));
			}
			else
			{
				cancel(serialization_traits::template read<std::error_code>(reply));
			}
		}
		else
		{
			resolve(reply);
		}
	}

	void
	resolve(deserializer_param_type reply)
	{
		std::error_code err;
		auto            item_count = serialization_traits::read_sequence_prefix(reply);

		if (!expected_count<1>(item_count))
		{
			cancel(make_error_code(armi::errc::invalid_argument_count));
		}
		else
		{
			try
			{
				m_promise.resolve(serialization_traits::template read<
								  typename std::remove_const_t<typename std::remove_reference_t<PromiseType>>>(reply));
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
	cancel(std::error_code err) override
	{
		m_promise.reject(err);
	}

private:
	util::promise<PromiseType> m_promise;
};

template<class Bridge, class PromiseType>
class reply_handler<
		Bridge,
		util::promise<PromiseType>,
		typename std::enable_if_t<std::is_void<PromiseType>::value>>
	: public reply_handler_base<Bridge>
{
public:
	using bridge_type             = Bridge;
	using serialization_traits    = typename bridge_type::serialization_traits;
	using deserializer_type       = typename serialization_traits::deserializer_type;
	using deserializer_param_type = typename bridge_type::deserializer_param_type;

	reply_handler(util::promise<PromiseType> p) : m_promise{p} {}

	virtual void
	handle_reply(deserializer_param_type reply) override
	{
		reply_kind rk = serialization_traits::template read<reply_kind>(reply);

		if (rk == reply_kind::fail)
		{
			auto item_count = serialization_traits::read_sequence_prefix(reply);

			if (!expected_count<1>(item_count))
			{
				cancel(make_error_code(armi::errc::invalid_argument_count));
			}
			else
			{
				cancel(serialization_traits::template read<std::error_code>(reply));
			}
		}
		else
		{
			resolve(reply);
		}
	}

	void
	resolve(deserializer_param_type reply)
	{
		std::error_code err;
		auto            item_count = serialization_traits::read_sequence_prefix(reply);

		if (!expected_count<0>(item_count))
		{
			cancel(make_error_code(armi::errc::invalid_argument_count));
		}
		else
		{
			m_promise.resolve();
		}
	}

	virtual void
	cancel(std::error_code err) override
	{
		m_promise.reject(err);
	}

private:
	util::promise<void> m_promise;
};

#if 0
template<class PromiseType>
class reply_handler<util::promise<PromiseType>, typename std::enable_if_t<std::is_void<PromiseType>::value>>
	: public reply_handler_base
{
public:
	reply_handler(util::promise<void> p) : m_promise{p} {}

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
			resolve(is);
		}
	}

	void
	resolve(bstream::ibstream& is)
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
				m_promise.resolve();
			}
		}
	}

	virtual void
	cancel(std::error_code err) override
	{
		m_promise.reject(err);
	}

private:
	util::promise<void> m_promise;
};
#endif

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_REPLY_HANDLER_H
