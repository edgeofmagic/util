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

namespace detail
{

class control_blk_base
{
public:
	~control_blk_base()
	{
		assert(m_use_count == 0);
	}

	virtual void destroy() = 0;

	control_blk_base(void* p) : m_elem{p}, m_use_count{1} {}

	void*
	get_vptr() const
	{
		return m_elem;
	}

	void
	clear()
	{
		m_elem = nullptr;
	}

	std::size_t
	increment_use_count()
	{
		return ++m_use_count;
	}

	std::size_t
	decrement_use_count()
	{
		return --m_use_count;
	}

	std::size_t
	use_count() const
	{
		return m_use_count;
	}

private:
	void*       m_elem;
	std::size_t m_use_count;
};

}    // namespace detail

template<class T, class Deleter = std::default_delete<T>>
class shared_ptr
{
public:
	using element_type = T;
	using deleter_type = Deleter;

// protected:
	class control_blk : protected detail::control_blk_base
	{
	protected:
		friend class shared_ptr;

		template<class _Del>
		control_blk(element_type* ep, _Del&& del) : detail::control_blk_base{ep}, m_deleter{std::forward<_Del>(del)}
		{}

		virtual void destroy()
		{
			delete this;
		}

		~control_blk()
		{
			assert(use_count() == 0);

			if (get_vptr())
			{
				m_deleter(static_cast<element_type*>(get_vptr()));
				clear();
			}
		}

		element_type*
		get_ptr()
		{
			return static_cast<element_type*>(get_vptr());
		}

		deleter_type m_deleter;
	};

	template<class _Alloc>
	class shareable_value : public control_blk
	{
	public:
		friend class shared_ptr;

		using allocator_type = typename _Alloc::template rebind<shareable_value<_Alloc>>::other;

		~shareable_value()
		{
			detail::control_blk_base::clear();
		}

		virtual void destroy()
		{
			allocator_type alloc{std::move(m_allocator)};
			alloc.destroy(this);
			alloc.deallocate(this, 1);
		}

		template<class _A, class... Args>
		shareable_value(_A&& alloc, Args&&... args)
			: control_blk{&m_value, std::default_delete<element_type>{}},
			  m_allocator{std::forward<_A>(alloc)},
			  m_value(std::forward<Args>(args)...)
		{}

		allocator_type m_allocator;
		element_type m_value;
	};

public:
	template<class U, class V>
	friend class shared_ptr;

	template<class... Args>
	static shared_ptr
	create(Args&&... args)
	{
		return shared_ptr{new shareable_value<std::allocator<T>>{std::allocator<T>{}, std::forward<Args>(args)...}};
	}

	template<class _Alloc, class... Args>
	static shared_ptr
	allocate(_Alloc, Args&&... args)
	{
		typename shareable_value<_Alloc>::allocator_type alloc;
		shareable_value<_Alloc>* cp = alloc.allocate(1);
		new(cp) shareable_value<_Alloc>{std::move(alloc), std::forward<Args>(args)...};
		return shared_ptr{cp};
	}

	template<class U, class V>
	static shared_ptr
	static_ptr_cast(shared_ptr<U, V> const& p)
	{
		detail::control_blk_base* cp = p.get_ctrl_blk();
		if (cp)
		{
			cp->increment_use_count();
		}
		return shared_ptr{cp};
	}

	template<class U, class V>
	static shared_ptr
	dynamic_ptr_cast(shared_ptr<U, V> const& p)
	{
		element_type* ep = dynamic_cast<element_type*>(p.get());
		if (ep)
		{
			auto cp = p.get_ctrl_blk();
			cp->increment_use_count();
			return shared_ptr{cp, ep};
		}
		else
		{
			return shared_ptr{};
		}
	}

	shared_ptr() : m_cblk_ptr{nullptr}, m_elem_ptr{nullptr} {}

	// shared_ptr(control_blk* sp) : m_cblk_ptr{sp}, m_elem_ptr{static_cast<element_type*>(m_cblk_ptr->get_vptr())} {}

	template<class _Del>
	shared_ptr(element_type* ep, _Del&& del)
		: m_cblk_ptr{new control_blk{ep, std::forward<_Del>(del)}},
		  m_elem_ptr{static_cast<element_type*>(m_cblk_ptr->get_vptr())}
	{}

	shared_ptr(shared_ptr const& rhs)
	{
		adopt(rhs);
	}

	template<class U, class V, class = typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
	shared_ptr(shared_ptr<U, V> const& rhs)
	{
		adopt(rhs);
	}

	shared_ptr(shared_ptr&& rhs)
	{
		steal(std::move(rhs));
	}

	template<class U, class V, class = typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
	shared_ptr(shared_ptr<U, V>&& rhs)
	{
		steal(std::move(rhs));
	}

	~shared_ptr()
	{
		disown();
	}

	shared_ptr&
	operator=(shared_ptr const& rhs)
	{
		assign(rhs);
		return *this;
	}


	template<class U, class V>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, shared_ptr&>
	operator=(shared_ptr<U, V> const& rhs)
	{
		assign(rhs);
		return *this;
	}

	shared_ptr&
	operator=(shared_ptr&& rhs)
	{
		assign(std::move(rhs));
		return *this;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, shared_ptr&>
	operator=(shared_ptr<U>&& rhs)
	{
		assign(std::move(rhs));
		return *this;
	}

	// control_blk* get_shareable() const
	// {
	// 	return static_cast<control_blk*>(m_cblk_ptr);
	// }

	detail::control_blk_base*
	get_ctrl_blk() const
	{
		return m_cblk_ptr;
	}

	void
	reset()
	{
		disown();
	}

	explicit operator bool() const
	{
		return m_elem_ptr != nullptr;
	}

	template<class U, class V>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, bool>
	operator==(shared_ptr<U, V> const& rhs) const
	{
		return m_elem_ptr == rhs.m_elem_ptr;
	}

	template<class U, class V>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, bool>
	operator!=(shared_ptr<U, V> const& rhs) const
	{
		return !(*this == rhs);
	}

	element_type*
	get() const
	{
		return m_elem_ptr;
	}

	element_type* operator->() const
	{
		return m_elem_ptr;
	}

	element_type& operator*() const
	{
		return *m_elem_ptr;
	}

	std::size_t
	use_count() const
	{
		return m_cblk_ptr->use_count();
	}

private:

	shared_ptr(detail::control_blk_base* p)
		: m_cblk_ptr{p}, m_elem_ptr{static_cast<element_type*>(m_cblk_ptr->get_vptr())}
	{}

	shared_ptr(detail::control_blk_base* p, element_type* ep) : m_cblk_ptr{p}, m_elem_ptr{ep} {}

	template<class U, class V>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>
	assign(shared_ptr<U, V> const& rhs)
	{
		disown();
		adopt(rhs);
	}

	template<class U, class V>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>
	assign(shared_ptr<U, V>&& rhs)
	{
		disown();
		steal(std::move(rhs));
	}

	// void adopt(shared_ptr const& rhs)
	// {
	// 	m_cblk_ptr = rhs.m_cblk_ptr;
	// 	m_elem_ptr = static_cast<element_type*>(rhs.m_elem_ptr);
	// 	if (m_cblk_ptr)
	// 	{
	// 		m_cblk_ptr->increment_use_count();
	// 	}
	// }

	template<class U, class V>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>
	adopt(shared_ptr<U, V> const& rhs)
	{
		m_cblk_ptr = rhs.m_cblk_ptr;
		m_elem_ptr = static_cast<element_type*>(rhs.m_elem_ptr);
		if (m_cblk_ptr)
		{
			m_cblk_ptr->increment_use_count();
		}
	}

	// template<class U>
	// void
	// force_adopt(shared_ptr<U> const& rhs)
	// {
	// 	m_cblk_ptr = rhs.m_cblk_ptr;
	// 	m_elem_ptr = static_cast<element_type*>(rhs.m_elem_ptr);
	// 	if (m_cblk_ptr)
	// 	{
	// 		m_cblk_ptr->increment_use_count();
	// 	}
	// }ß

	template<class U, class V>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>
	steal(shared_ptr<U, V>&& rhs)
	{
		m_cblk_ptr     = rhs.m_cblk_ptr;
		m_elem_ptr     = static_cast<element_type*>(rhs.m_elem_ptr);
		rhs.m_cblk_ptr = nullptr;
		rhs.m_elem_ptr = nullptr;
	}

	void
	disown()
	{
		if (m_cblk_ptr)
		{
			assert(m_cblk_ptr->use_count() > 0);
			if (m_cblk_ptr->decrement_use_count() == 0)
			{
				// delete m_cblk_ptr;
				m_cblk_ptr->destroy();
			}
			m_cblk_ptr = nullptr;
			m_elem_ptr = nullptr;
		}
	}

private:
	detail::control_blk_base* m_cblk_ptr;
	element_type*             m_elem_ptr;
};


}    // namespace util
}    // namespace logicmill

#endif    // LOGICMILL_UTIL_SHAREABLE_H