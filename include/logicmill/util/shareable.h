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

#include <cstdint>
#include <logicmill/traits.h>
#include <cassert>
#include <memory>

namespace logicmill
{
namespace util
{

template<class U>
struct default_deleter
{
	void operator()(U* p) const { delete p; }
};

template<class U>
struct do_not_delete
{
	void operator()(U* p) const { }
};

template<class U, class Alloc = std::allocator<U>>
struct allocator_delete
{
	void operator()(U*p) const
	{
		Alloc{}.deallocate(p, sizeof(U));
	}
};

template<class Derived>
class shareable
{
public:

	template<class U, class V>
	friend class handle;

protected:
	shareable() : m_use_count{1} {}

private:

	std::size_t m_use_count;
};

template<class T, class Deleter = default_deleter<T>>
class handle
{
protected:
	handle(T* ptr, Deleter del = Deleter{}) : m_ptr{ptr}, m_deleter{del} {} 

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

	T* get_ptr() const
	{
		return m_ptr;
	}

	void adopt(handle const& rhs)
	{
		m_ptr = rhs.m_ptr;
		m_deleter = rhs.m_deleter;
		++m_ptr->m_use_count;
	}

	void steal(handle&& rhs)
	{
		m_ptr = rhs.m_ptr;
		m_deleter = std::move(rhs.m_deleter);
		rhs.m_ptr = nullptr;
	}

	void disown()
	{
		if (m_ptr)
		{
			assert(m_ptr->m_use_count > 0);
			if (--m_ptr->m_use_count == 0)
			{
				m_deleter(m_ptr);
			}
			m_ptr = nullptr;
		}
	}

private:
	T* m_ptr;
	Deleter m_deleter;
};

template<class T, class Deleter = default_deleter<T>>
class phandle : public handle<T, Deleter>
{
protected:
	using base = handle<T, Deleter>;

public:

	phandle(T* ptr, Deleter del = Deleter{}) : base{ptr, del} {} 

	using base::disown;
	using base::get_ptr;

	phandle(phandle const& rhs) : base{rhs} {}

	phandle(phandle&& rhs) : base{std::move(rhs)} {}

	~phandle()
	{
	}

	phandle& operator=(phandle const& rhs)
	{
		assign(rhs);
		return *this;
	}

	phandle& operator=(phandle&& rhs)
	{
		assign(std::move(rhs));
		return *this;
	}

	void reset()
	{
		disown();
	}

	explicit operator bool() const
	{
		return get_ptr() != nullptr;
	}

	bool operator==(phandle const& rhs) const
	{
		return get_ptr() == rhs.get_ptr();
	}

	bool operator!=(phandle const& rhs) const
	{
		return !(*this == rhs);
	}

	T* get() const
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

template<class T, class... Args>
inline phandle<T>
make_shareable(Args&&... args)
{
	T* ptr = new T(std::forward<Args>(args)...);
	return phandle<T>{ptr};
}

}    // namespace util
}    // namespace logicmill

#endif    // LOGICMILL_UTIL_SHAREABLE_H