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
 * File:   interface_stub_builder.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 11:05 PM
 */

#ifndef LOGICMILL_ARMI_INTERFACE_STUB_BUILDER_H
#define LOGICMILL_ARMI_INTERFACE_STUB_BUILDER_H

#include <logicmill/armi/method_stub.h>
#include <memory>
#include <vector>

namespace logicmill
{
namespace armi
{
template<std::size_t... Ns>
struct indices
{
	typedef indices<Ns..., sizeof...(Ns)> next;
};

template<std::size_t N>
struct make_indices
{
	typedef typename make_indices<N - 1>::type::next type;
};

template<>
struct make_indices<0>
{
	typedef indices<> type;
};

class interface_stub_builder : public interface_stub_base
{
protected:
	template<class... Args, std::size_t... Ns>
	inline interface_stub_builder(server_context_base& context, std::size_t index, indices<Ns...> _i, Args... args)
		: interface_stub_base(context, index)
	{
		m_stubs.reserve(sizeof...(Args));
		append(std::unique_ptr<method_stub_base>(new armi::method_stub<decltype(args)>(context, args, index, Ns))...);
	}

	template<class First, class... Args>
	inline void
	append(First&& first, Args&&... args)
	{
		m_stubs.push_back(std::move(first));
		append(std::move(args)...);
	}

	inline void
	append()
	{}

	virtual std::size_t
	method_count() const noexcept
	{
		return m_stubs.size();
	}

	virtual method_stub_base&
	get_method_stub(std::size_t index)
	{
		return *m_stubs[index];
	}

protected:
	std::vector<std::unique_ptr<method_stub_base>> m_stubs;
};
}    // namespace armi
}    // namespace logicmill


#endif /* LOGICMILL_ARMI_INTERFACE_STUB_BUILDER_H */
