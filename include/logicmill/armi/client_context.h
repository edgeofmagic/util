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
 * File:   client_context.h
 * Author: David Curtis
 *
 * Created on October 15, 2017, 4:09 PM
 */

#ifndef LOGICMILL_ARMI_CLIENT_CONTEXT_H
#define LOGICMILL_ARMI_CLIENT_CONTEXT_H

#include <logicmill/armi/client_context_builder.h>
#include <logicmill/traits.h>

namespace logicmill
{
namespace armi
{
template<class T>
class client_context;

template<template<class...> class Template, class... Args>
class client_context<Template<Args...>> : public logicmill::armi::client_context_builder<Args...>
{
public:
	using base                = logicmill::armi::client_context_builder<Args...>;
	using proxy_list_carrier  = Template<Args...>;
	using target_list_carrier = traits::_arg_list<typename Args::target_type...>;

	using connect_handler         = std::function<void(client_context&, std::error_code)>;

	template<class T>
	using proxy_of = typename traits::nth_element_from<traits::index_from<T, target_list_carrier>::value, proxy_list_carrier>::type;

	template<class T>
	using proxy_ptr = std::shared_ptr<proxy_of<T>>;

	template<class T>
	using proxy_ref = proxy_of<T>&;

	client_context(async::loop::ptr const& lp, bstream::context_base const& cntxt)
		: base{lp, cntxt}
	{}

	template<class T>
	inline proxy_ref<T>
	proxy()
	{
		return dynamic_cast<proxy_of<T>&>(*(base::m_proxies[traits::index_from<T, target_list_carrier>::value]));
	}

private:

	class connect_channel_handler 
	{
	public:

		template<
				class Handler,
				class = typename std::enable_if_t<
						std::is_convertible<Handler, client_context::connect_handler>::value>>
		connect_channel_handler(client_context& cntxt, Handler&& handler)
			: m_client_context{cntxt}, m_handler{std::forward<Handler>(handler)}
		{}

		void
		operator()(async::channel::ptr const& chan, std::error_code err)
		{
			if (err)
			{
				m_client_context.channel().reset();
			}
			else
			{
				m_client_context.channel(chan);
				m_client_context.channel()->start_read(
						err, [=](async::channel::ptr const& chan, bstream::const_buffer&& buf, std::error_code err) {
							if (err)
							{
								m_client_context.really_close(err);
							}
							else
							{
								assert(chan == m_client_context.channel());
								m_client_context.on_read(std::move(buf), err);
							}
						});
			}
			m_handler(m_client_context, err);
		}

		client_context&                 m_client_context;
		client_context::connect_handler m_handler;
	};

public:

	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, connect_handler>::value>
	connect(async::options const& opts, std::error_code& err, Handler&& handler)
	{
		logicmill::async::options opts_override{opts};
		opts_override.framing(true);
		client_context_base::loop()->connect_channel(opts_override, err, connect_channel_handler{*this, std::forward<Handler>(handler)});
	}

};

}    // namespace armi
}    // namespace logicmill

#endif /* LOGICMILL_ARMI_CLIENT_CONTEXT_H */
