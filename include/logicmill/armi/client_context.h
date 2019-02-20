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

// #include <logicmill/armi/client_context_builder.h>
#include <logicmill/traits.h>
#include <logicmill/armi/interface_proxy.h>

namespace logicmill
{
namespace armi
{
// template<class T>
// class client_context;

template<class Proxy, class StreamContext>
class client_context : public client_context_base 
{
public:
	using ptr = SHARED_PTR_TYPE<client_context>;
	using base = client_context_base;
	using proxy_type = Proxy;

	client_context()
		: base{StreamContext::get()}, m_proxy{*this}
	{}

	proxy_type&
	proxy()
	{
		return m_proxy;
	}

private:
	proxy_type m_proxy;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_CONTEXT_H
