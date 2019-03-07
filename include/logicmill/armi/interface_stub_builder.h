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

#ifndef LOGICMILL_ARMI_INTERFACE_STUB_BUILDER_H
#define LOGICMILL_ARMI_INTERFACE_STUB_BUILDER_H

#include <logicmill/armi/interface_stub_base.h>
#include <logicmill/armi/member_func_stub.h>
#include <logicmill/traits.h>
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

template<class Target>
class interface_stub_builder : public interface_stub_base<Target>
{
protected:
	template<class... Args, std::size_t... Ns>
	interface_stub_builder(server_context_base& context, indices<Ns...> _i, Args... args)
		: interface_stub_base<Target>(context)
	{
		m_member_func_stubs.reserve(sizeof...(Args));
		append(std::unique_ptr<member_func_stub_base<Target>>(
				new armi::member_func_stub<
						decltype(args),
						typename traits::remove_member_func_cv_noexcept<decltype(args)>::type>(context, args, Ns))...);
	}

	template<class First, class... Args>
	void
	append(First&& first, Args&&... args)
	{
		m_member_func_stubs.push_back(std::move(first));
		append(std::move(args)...);
	}

	void
	append()
	{}

	virtual std::size_t
	member_func_count() const noexcept override
	{
		return m_member_func_stubs.size();
	}

	virtual member_func_stub_base<Target>&
	get_member_func_stub(std::size_t index) override
	{
		return *m_member_func_stubs[index];
	}

protected:
	std::vector<std::unique_ptr<member_func_stub_base<Target>>> m_member_func_stubs;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_INTERFACE_STUB_BUILDER_H
