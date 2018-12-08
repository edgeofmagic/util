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

	template<class T>
	using proxy_of = typename traits::nth_element_from<traits::index_from<T, target_list_carrier>::value, proxy_list_carrier>::type;

	template<class T>
	using proxy_ptr = std::shared_ptr<proxy_of<T>>;

	client_context(async::loop::ptr const& lp, bstream::context_base const& cntxt)
		: base{lp, cntxt}
	{}

	template<class T>
	inline proxy_ptr<T>
	proxy()
	{
		return std::dynamic_pointer_cast<proxy_of<T>>(base::m_proxies[traits::index_from<T, target_list_carrier>::value]);
	}
};
}    // namespace armi
}    // namespace logicmill

#endif /* LOGICMILL_ARMI_CLIENT_CONTEXT_H */
