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

#ifndef LOGICMILL_ARMI_METHOD_STUB_H
#define LOGICMILL_ARMI_METHOD_STUB_H

#include <logicmill/armi/fail_proxy.h>
#include <logicmill/armi/method_stub_base.h>
#include <logicmill/armi/reply_proxy.h>
#include <logicmill/armi/server_context_base.h>
#include <logicmill/armi/traits.h>

namespace logicmill
{
namespace armi
{
template<class F, class Enable = void>
class method_stub;

/*
 * Form 1 - An error-safe reply, no other parameters
 */
template<class Target, class Reply>
class method_stub<
		void (Target::*)(Reply),
		std::enable_if_t<is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value>>
	: public method_stub_base<Target>
{
public:
	using method_ptr_type  = void (Target::*)(Reply);
	using reply_proxy_type = reply_proxy<Reply>;
	using target_ptr_type    = std::shared_ptr<Target>;
	using method_stub_base<Target>::request_failed;
	using method_stub_base<Target>::context;

	inline method_stub(server_context_base& context, method_ptr_type method_ptr, std::size_t method_id)
		: method_stub_base<Target>{context}, m_method_id{method_id}, m_method_ptr{method_ptr}
	{}

	virtual void
	dispatch(request_id_type request_id, channel_id_type channel_id, bstream::ibstream& is, target_ptr_type const& target)
			const override
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
			goto fail;

		if (!expected_count<0>(item_count))
		{
			err = make_error_code(armi::errc::invalid_argument_count);
			goto fail;
		}

		invoke(request_id, channel_id, target);
		return;

	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

	void
	invoke(request_id_type request_id, channel_id_type channel_id, target_ptr_type const& target) const
	{
		// target_ptr_type target = std::static_pointer_cast<Target>(m_context.get_type_erased_impl());
		if (!target)
		{
			request_failed(
					request_id,
					channel_id,
					make_error_code(armi::errc::no_implementation_instance_registered));
		}
		else
		{
			try
			{
				((*target).*m_method_ptr)(reply_proxy_type{request_id, channel_id, context()});
			}
			catch (std::system_error const& e)
			{
				request_failed(request_id, channel_id, e.code());
			}
			catch (std::exception const& e)
			{
				request_failed(
						request_id,
						channel_id,
						make_error_code(armi::errc::uncaught_server_exception));
			}
		}
	}

private:
	std::size_t          m_method_id;
	method_ptr_type      m_method_ptr;
};

/*
 * Form 2 - An error-safe reply and one other parameter, which is not fail_reply)
 */
template<class Target, class Reply, class First>
class method_stub<
		void (Target::*)(Reply, First),
		std::enable_if_t<
				!std::is_same<First, fail_reply>::value
				&& (is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value)>>
	: public method_stub_base<Target>
{
public:
	using method_ptr_type  = void (Target::*)(Reply, First);
	using reply_proxy_type = reply_proxy<Reply>;
	using target_ptr_type    = std::shared_ptr<Target>;
	using method_stub_base<Target>::request_failed;
	using method_stub_base<Target>::context;

	inline method_stub(server_context_base& context, method_ptr_type method_ptr, std::size_t method_id)
		: method_stub_base<Target>{context}, m_method_id{method_id}, m_method_ptr{method_ptr}
	{}

	virtual void
	dispatch(request_id_type request_id, channel_id_type channel_id, bstream::ibstream& is, target_ptr_type const& target)
			const override
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
			goto fail;

		if (!expected_count<1>(item_count))
		{
			err = make_error_code(armi::errc::invalid_argument_count);
			goto fail;
		}

		try
		{
			invoke(request_id,
				   channel_id,
				   target,
				   is.read_as<typename std::remove_cv_t<typename std::remove_reference_t<First>>>());
		}
		catch (std::system_error const& e)
		{
			err = e.code();
		}
		catch (std::exception const& e)
		{
			err = make_error_code(armi::errc::exception_thrown_by_method_stub);
		}
		if (err)
			goto fail;

		return;

	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

	inline void
	invoke(request_id_type request_id, channel_id_type channel_id, target_ptr_type const& target, First first) const
	{
		std::error_code err;
		// target_ptr_type   target = std::static_pointer_cast<Target>(m_context.get_type_erased_impl());
		if (!target)
		{
			err = make_error_code(armi::errc::no_implementation_instance_registered);
			goto fail;
		}

		try
		{
			((*target).*m_method_ptr)(reply_proxy_type{request_id, channel_id, context()}, first);
		}
		catch (std::system_error const& e)
		{
			err = e.code();
		}
		catch (std::exception const& e)
		{
			err = make_error_code(armi::errc::uncaught_server_exception);
		}
		if (err)
			goto fail;

		return;
	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

private:
	std::size_t          m_method_id;
	method_ptr_type      m_method_ptr;
};

/*
 * Form 3 - An error-safe reply and multiple parameters, where the 
 * first is not fail_reply
 */
template<class Target, class Reply, class First, class... Args>
class method_stub<
		void (Target::*)(Reply, First, Args...),
		std::enable_if_t<
				!std::is_same<First, fail_reply>::value
				&& (is_error_safe_reply<Reply>::value || std::is_same<Reply, fail_reply>::value)>>
	: public method_stub_base<Target>
{
public:
	using method_ptr_type  = void (Target::*)(Reply, First, Args...);
	using reply_proxy_type = reply_proxy<Reply>;
	using target_ptr_type    = std::shared_ptr<Target>;
	using method_stub_base<Target>::request_failed;
	using method_stub_base<Target>::context;

	inline method_stub(server_context_base& context, method_ptr_type method_ptr, std::size_t method_id)
		: method_stub_base<Target>{context}, m_method_id{method_id}, m_method_ptr{method_ptr}
	{}

	virtual void
	dispatch(request_id_type request_id, channel_id_type channel_id, bstream::ibstream& is, target_ptr_type const& target)
			const override
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
			goto fail;

		if (!expected_count<1 + sizeof...(Args)>(item_count))
		{
			err = make_error_code(armi::errc::invalid_argument_count);
			goto fail;
		}

		try
		{
			invoke(request_id,
				   channel_id,
				   target,
				   is.read_as<typename std::remove_cv_t<typename std::remove_reference_t<First>>>(),
				   is.read_as<typename std::remove_cv_t<typename std::remove_reference_t<Args>>>()...);
		}
		catch (std::system_error const& e)
		{
			err = e.code();
		}
		catch (std::exception const& e)
		{
			err = make_error_code(armi::errc::exception_thrown_by_method_stub);
		}
		if (err)
			goto fail;

		return;

	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

	inline void
	invoke(request_id_type request_id, channel_id_type channel_id, target_ptr_type const& target, First first, Args... args) const
	{
		std::error_code err;
		// target_ptr_type   target = std::static_pointer_cast<Target>(m_context.get_type_erased_impl());
		if (!target)
		{
			err = make_error_code(armi::errc::no_implementation_instance_registered);
			goto fail;
		}

		try
		{
			((*target)
			 .*m_method_ptr)(reply_proxy_type{request_id, channel_id, context()}, first, args...);
		}
		catch (std::system_error const& e)
		{
			err = e.code();
		}
		catch (std::exception const& e)
		{
			err = make_error_code(armi::errc::uncaught_server_exception);
		}
		if (err)
			goto fail;

		return;

	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

private:
	std::size_t          m_method_id;
	method_ptr_type      m_method_ptr;
};

/*
 * Form 4 - A reply (not necessarily error_safe), and an error_reply (no other parameters).
 */
template<class Target, class Reply>
class method_stub<void (Target::*)(Reply, fail_reply)> : public method_stub_base<Target>
{
public:
	using method_ptr_type  = void (Target::*)(Reply, fail_reply);
	using reply_proxy_type = reply_proxy<Reply>;
	using target_ptr_type    = std::shared_ptr<Target>;
	using method_stub_base<Target>::request_failed;
	using method_stub_base<Target>::context;

	inline method_stub(server_context_base& context, method_ptr_type method_ptr, std::size_t method_id)
		: method_stub_base<Target>{context}, m_method_id{method_id}, m_method_ptr{method_ptr}
	{}

	virtual void
	dispatch(request_id_type request_id, channel_id_type channel_id, bstream::ibstream& is, target_ptr_type const& target)
			const override
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
			goto fail;

		if (!expected_count<0>(item_count))
		{
			err = make_error_code(armi::errc::invalid_argument_count);
			goto fail;
		}

		invoke(request_id, channel_id, target);
		return;

	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

	inline void
	invoke(request_id_type request_id, channel_id_type channel_id, target_ptr_type const& target) const
	{
		std::error_code err;
		// target_ptr_type   target = std::static_pointer_cast<Target>(m_context.get_type_erased_impl());
		if (!target)
		{
			err = make_error_code(armi::errc::no_implementation_instance_registered);
			goto fail;
		}

		try
		{
			((*target).*m_method_ptr)(
					reply_proxy_type{request_id, channel_id, context()},
					fail_proxy{request_id, channel_id, context()});
		}
		catch (std::system_error const& e)
		{
			err = e.code();
		}
		catch (std::exception const& e)
		{
			err = make_error_code(armi::errc::uncaught_server_exception);
		}
		if (err)
			goto fail;

		return;

	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

private:
	std::size_t          m_method_id;
	method_ptr_type      m_method_ptr;
};

/*
 * Form 5 - A reply (not necessarily error_safe), an error_reply, and additional parameters.
 */
template<class Target, class Reply, class... Args>
class method_stub<void (Target::*)(Reply, fail_reply, Args...)> : public method_stub_base<Target>
{
public:
	using method_ptr_type  = void (Target::*)(Reply, fail_reply, Args...);
	using reply_proxy_type = reply_proxy<Reply>;
	using target_ptr_type    = std::shared_ptr<Target>;
	using method_stub_base<Target>::request_failed;
	using method_stub_base<Target>::context;

	inline method_stub(server_context_base& context, method_ptr_type method_ptr, std::size_t method_id)
		: method_stub_base<Target>{context}, m_method_id{method_id}, m_method_ptr{method_ptr}
	{}

	virtual void
	dispatch(request_id_type request_id, channel_id_type channel_id, bstream::ibstream& is, target_ptr_type const& target)
			const override
	{
		std::error_code err;
		auto            item_count = is.read_array_header(err);
		if (err)
			goto fail;

		if (!expected_count<sizeof...(Args)>(item_count))
		{
			err = make_error_code(armi::errc::invalid_argument_count);
			goto fail;
		}

		try
		{
			invoke(request_id,
				   channel_id,
				   target,
				   is.read_as<typename std::remove_const_t<typename std::remove_reference_t<Args>>>()...);
		}
		catch (std::system_error const& e)
		{
			err = e.code();
		}
		catch (std::exception const& e)
		{
			err = make_error_code(armi::errc::exception_thrown_by_method_stub);
		}
		if (err)
			goto fail;

		return;

	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

	inline void
	invoke(request_id_type request_id, channel_id_type channel_id, target_ptr_type const& target, Args... args) const
	{
		std::error_code err;
		// target_ptr_type   target = std::static_pointer_cast<Target>(m_context.get_type_erased_impl());
		if (!target)
		{
			err = make_error_code(armi::errc::no_implementation_instance_registered);
			goto fail;
		}

		try
		{
			((*target).*m_method_ptr)(
					reply_proxy_type{request_id, channel_id, context()},
					fail_proxy{request_id, channel_id, context()},
					args...);
		}
		catch (std::system_error const& e)
		{
			err = e.code();
		}
		catch (std::exception const& e)
		{
			err = make_error_code(armi::errc::uncaught_server_exception);
		}
		if (err)
			goto fail;

		return;

	fail:
		request_failed(request_id, channel_id, err);
		return;
	}

private:
	std::size_t          m_method_id;
	method_ptr_type      m_method_ptr;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_METHOD_STUB_H
