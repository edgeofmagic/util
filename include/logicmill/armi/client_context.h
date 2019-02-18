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
	using ptr = SHARED_PTR_TYPE<client_context>;
	using base                = logicmill::armi::client_context_builder<Args...>;
	using proxy_list_carrier  = Template<Args...>;
	using target_list_carrier = traits::_arg_list<typename Args::target_type...>;

	// using connect_handler = std::function<void(client_context&, std::error_code)>;

	template<class T>
	using proxy_of = typename traits::
			nth_element_from<traits::index_from<T, target_list_carrier>::value, proxy_list_carrier>::type;

	// template<class T>
	// using proxy_ptr = std::shared_ptr<proxy_of<T>>;

	template<class T>
	using proxy_ref = proxy_of<T>&;

	client_context(/* transport::client_connection::ptr const& cp, */ bstream::context_base::ptr const& cntxt)
		: base{/* cp, */ cntxt}
	{}

	template<class T>
	proxy_ref<T>
	proxy()
	{
		return dynamic_cast<proxy_of<T>&>(*(base::m_proxies[traits::index_from<T, target_list_carrier>::value]));
	}
/*
	template<class Handler>
	typename std::enable_if_t<std::is_convertible<Handler, connect_handler>::value>
	connect(async::options const& opts, std::error_code& err, Handler&& handler)
	{
		logicmill::async::options opts_override{opts};
		opts_override.framing(true);
		client_context_base::loop()->connect_channel(
				opts_override,
				err,
				[=, handler{std::forward<Handler>(handler)}](async::channel::ptr const& chan, std::error_code err) {
					if (err)
					{
						this->channel().reset();
					}
					else
					{
						this->channel(chan);
						this->channel()->start_read(
								err,
								[=](async::channel::ptr const& chan, util::const_buffer&& buf, std::error_code err) {
									if (err)
									{
										this->really_close(err);
									}
									else
									{
										assert(chan == this->channel());
										this->on_read(std::move(buf), err);
									}
								});
					}
					handler(*this, err);
				});
	}
*/
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_CONTEXT_H
