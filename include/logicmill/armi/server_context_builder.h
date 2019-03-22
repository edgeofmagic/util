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

#ifndef SERVER_CONTEXT_BUILDER_H
#define SERVER_CONTEXT_BUILDER_H

#include <logicmill/armi/server_context_base.h>

namespace logicmill
{
namespace armi
{

template<class SerializationContext, class TransportContext, class... Args>
class server_context_builder : public server_context_base<SerializationContext, TransportContext>
{
public:
	server_context_builder() : server_context_base{}
	{
		m_stubs.reserve(sizeof...(Args));
		append_stubs<Args...>();
	}

	template<class T>
	void
	append_stubs()
	{
		append_stub<T>();
	}

	template<class First_, class... Args_>
	typename std::enable_if_t<(sizeof...(Args_) > 0)>
	append_stubs()
	{
		append_stub<First_>();
		append_stubs<Args_...>();
	}

	template<class T>
	void
	append_stub()
	{

		std::size_t index = m_stubs.size();
		m_stubs.emplace_back(std::unique_ptr<interface_stub_base>(new T{*this, index}));
	}
};

}    // namespace armi
}    // namespace logicmill

#endif    // SERVER_CONTEXT_BUILDER_H
