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

#ifndef LOGICMILL_UTIL_SHAREABLE_H
#define LOGICMILL_UTIL_SHAREABLE_H

#include <cassert>
#include <cstdint>
#include <memory>

namespace logicmill
{
namespace util
{

template<class T, class Alloc = std::allocator<T>>
class handle
{
protected:
	class shareable;

	template<class _Alloc>
	handle(shareable* ptr, _Alloc const& alloc) : m_ptr{ptr}, m_allocator{alloc}
	{}

	class shareable
	{
	protected:
		template<class U, class V>
		friend class handle;

		template<class U, class V>
		friend class phandle;

		template<class... Args>
		shareable(Args&&... args) : m_value(std::forward<Args>(args)...), m_use_count{1}
		{}

		T*
		get_value_ptr()
		{
			return &m_value;
		}

	private:
		T           m_value;
		std::size_t m_use_count;
	};

public:
	using value_type               = T;
	using pointer_type             = T*;
	using shareable_type           = shareable;
	using allocator_type           = Alloc;
	using shareable_allocator_type = typename Alloc::template rebind<shareable_type>::other;

	// template<class... Args>
	// static handle
	// create(Args&&... args)
	// {
	// 	return handle{std::forward<Args>(args)...};
	// 	// auto p = shareable_allocator_type{}.allocate(1);
	// 	// new(p) shareable_type(std::forward<Args>(args)...);
	// 	// return handle(p, shareable_allocator_type{});
	// }

protected:
	template<class... Args>
	handle(Args&&... args) : m_ptr{shareable_allocator_type{}.allocate(1)}, m_allocator{shareable_allocator_type{}}
	{
		new (m_ptr) shareable_type(std::forward<Args>(args)...);
	}

	handle(handle const& h)
	{
		adopt(h);
	}

	handle(handle&& rhs)
	{
		steal(std::move(rhs));
	}

	~handle()
	{
		disown();
	}

	void
	assign(handle const& rhs)
	{
		disown();
		adopt(rhs);
	}

	void
	assign(handle&& rhs)
	{
		disown();
		steal(std::move(rhs));
	}

	std::size_t
	get_refcount() const
	{
		return m_ptr->m_use_count;
	}

	T*
	get_ptr() const
	{
		return m_ptr->get_value_ptr();
	}

	void
	adopt(handle const& rhs)
	{
		m_ptr       = rhs.m_ptr;
		m_allocator = rhs.m_allocator;
		++m_ptr->m_use_count;
	}

	void
	steal(handle&& rhs)
	{
		m_ptr       = rhs.m_ptr;
		m_allocator = rhs.m_allocator;
		rhs.m_ptr   = nullptr;
	}

	void
	disown()
	{
		if (m_ptr)
		{
			assert(m_ptr->m_use_count > 0);
			if (--m_ptr->m_use_count == 0)
			{
				m_allocator.destroy(m_ptr);
				m_allocator.deallocate(m_ptr, 1);
			}
			m_ptr = nullptr;
		}
	}

private:
	shareable*               m_ptr;
	shareable_allocator_type m_allocator;
};

template<class T, class Alloc = std::allocator<T>>
class phandle : public handle<T, Alloc>
{
protected:
	using base = handle<T, Alloc>;

public:
	using value_type               = typename base::value_type;
	using pointer_type             = typename base::pointer_type;
	using shareable_type           = typename base::shareable_type;
	using allocator_type           = typename base::allocator_type;
	using shareable_allocator_type = typename base::shareable_allocator_type;

protected:
	phandle(shareable_type* ptr, shareable_allocator_type const& alloc) : base{ptr, alloc} {}

	using base::disown;
	using base::assign;
	using base::get_ptr;

public:
	template<class... Args>
	static phandle
	create(Args&&... args)
	{
		auto p = shareable_allocator_type{}.allocate(1);
		new (p) shareable_type(std::forward<Args>(args)...);
		return phandle(p, shareable_allocator_type{});
	}

	phandle(phandle const& rhs) : base{static_cast<base const&>(rhs)} {}

	phandle(phandle&& rhs) : base{std::move(static_cast<base&&>(rhs))} {}

	~phandle() {}

	phandle&
	operator=(phandle const& rhs)
	{
		assign(rhs);
		return *this;
	}

	phandle&
	operator=(phandle&& rhs)
	{
		assign(std::move(rhs));
		return *this;
	}

	void
	reset()
	{
		disown();
	}

	explicit operator bool() const
	{
		return get_ptr() != nullptr;
	}

	bool
	operator==(phandle const& rhs) const
	{
		return get_ptr() == rhs.get_ptr();
	}

	bool
	operator!=(phandle const& rhs) const
	{
		return !(*this == rhs);
	}

	T*
	get() const
	{
		return get_ptr();
	}

	T* operator->() const
	{
		return get_ptr();
	}

	T& operator*() const
	{
		return *get_ptr();
	}

	std::size_t
	use_count() const
	{
		return base::get_refcount();
	}
};

}    // namespace util
}    // namespace logicmill

#endif    // LOGICMILL_UTIL_SHAREABLE_H