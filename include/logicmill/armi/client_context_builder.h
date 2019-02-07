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

#ifndef LOGICMILL_ARMI_CLIENT_CONTEXT_BUILDER_H
#define LOGICMILL_ARMI_CLIENT_CONTEXT_BUILDER_H

#include <logicmill/armi/client_context_base.h>
#include <logicmill/armi/interface_proxy.h>
#include <memory>
#include <vector>

namespace logicmill
{
namespace armi
{
template<class... Args>
class client_context_builder : public client_context_base
{
public:
	client_context_builder(async::loop::ptr const& lp, bstream::context_base::ptr stream_context)
		: client_context_base{lp, stream_context}
	{
		m_proxies.reserve(sizeof...(Args));
		append_proxies<Args...>();
	}

	template<class T>
	void
	append_proxies()
	{
		append_proxy<T>();
	}

	template<class First_, class... Args_>
	typename std::enable_if<(sizeof...(Args_) > 0)>::type
	append_proxies()
	{
		append_proxy<First_>();
		append_proxies<Args_...>();
	}

	template<class T>
	void
	append_proxy()
	{
		std::size_t index = m_proxies.size();
		m_proxies.push_back(std::make_unique<T>(*this, index));
	}

protected:
	std::vector<std::unique_ptr<armi::interface_proxy>> m_proxies;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_CONTEXT_BUILDER_H
