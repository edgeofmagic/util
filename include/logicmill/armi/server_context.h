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

#ifndef LOGICMILL_ARMI_SERVER_CONTEXT_H
#define LOGICMILL_ARMI_SERVER_CONTEXT_H

#include <logicmill/armi/server_context_builder.h>
#include <logicmill/traits.h>

namespace logicmill
{
namespace armi
{
template<class T>
class server_context;

template<template<class...> class Template, class... Args>
class server_context<Template<Args...>> : public logicmill::armi::server_context_builder<Args...>
{
public:
	using base                = logicmill::armi::server_context_builder<Args...>;
	using stub_list_carrier   = Template<Args...>;
	using target_list_carrier = traits::_arg_list<typename Args::target_type...>;

	template<class T>
	using stub_of = typename traits::replace_arg<typename traits::first_arg<stub_list_carrier>::type, T>::type;

	using server_error_handler = std::function<void(server_context& server, std::error_code ec)>;

	server_context(
			async::loop::ptr const&      lp             = async::loop::get_default(),
			bstream::context_base const& stream_context = armi::get_default_stream_context())
		: base{lp, stream_context}
	{}

	template<class T>
	void
	register_impl(std::shared_ptr<T> impl_ptr)
	{
		logicmill::armi::server_context_base::set_impl(traits::index_from<T, target_list_carrier>::value, impl_ptr);
	}

	template<class T>
	std::shared_ptr<T>
	get_impl()
	{
		return std::static_pointer_cast<T>(
				server_context_base::get_impl(traits::index_from<T, target_list_carrier>::value));
	}

private:
	class error_handler_wrapper : public server_context_base::error_handler_wrapper_base
	{
	public:
		template<
				class Handler,
				class = typename std::enable_if_t<
						std::is_convertible<Handler, server_context::server_error_handler>::value>>
		error_handler_wrapper(server_context& cntxt, Handler&& handler)
			: m_server_context{cntxt}, m_handler{std::forward<Handler>(handler)}
		{}

		virtual void
		invoke(std::error_code err) override
		{
			m_handler(m_server_context, err);
		}

		server_context&                      m_server_context;
		server_context::server_error_handler m_handler;
	};

public:
	template<class T, class U>
	typename std::enable_if_t<
			std::is_convertible<T, server_error_handler>::value
					&& std::is_convertible<U, server_context_base::channel_error_handler>::value,
			server_context&>
	bind(async::options const& opts, std::error_code& err, T&& on_acceptor_error, U&& on_channel_error)
	{
		async::options opts_override{opts};
		opts_override.framing(true);
		server_context_base::m_on_server_error
				= std::make_unique<error_handler_wrapper>(*this, std::forward<T>(on_acceptor_error));
		server_context_base::m_on_channel_error = std::forward<U>(on_channel_error);
		server_context_base::really_bind(opts_override, err);
		return *this;
	}

	server_context&
	bind(async::options const& opts, std::error_code& err)
	{
		async::options opts_override{opts};
		opts_override.framing(true);
		server_context_base::really_bind(opts_override, err);
		return *this;
	}

	template<class T>
	typename std::enable_if_t<std::is_convertible<T, server_error_handler>::value, server_context&>
	on_acceptor_error(T&& handler)
	{
		server_context_base::m_on_server_error
				= std::make_unique<error_handler_wrapper>(*this, std::forward<T>(handler));
		return *this;
	}

	template<class T>
	typename std::
			enable_if_t<std::is_convertible<T, server_context_base::channel_error_handler>::value, server_context&>
			on_channel_error(T&& handler)
	{
		server_context_base::m_on_channel_error = std::forward<T>(handler);
		return *this;
	}
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_SERVER_CONTEXT_H
