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

#ifndef LOGICMILL_UTIL_SHARED_PTR_H
#define LOGICMILL_UTIL_SHARED_PTR_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <functional>

namespace logicmill
{
namespace util
{

namespace detail
{

class control_blk_base
{
public:
	using delete_erasure = std::function<void(void*)>;
	using destruct_erasure = std::function<void(void*)>;

	~control_blk_base()
	{
		assert(m_use_count == 0);
		if (m_elem)
		{
			m_delete(m_elem);
		}
	}
	void destroy()
	{
		m_destruct(this);
	}

	control_blk_base(void* p, delete_erasure&& deleter, destruct_erasure&& destruct)
		: m_elem{p}, m_use_count{1}, m_delete{std::move(deleter)}, m_destruct{std::move(destruct)}
	{}

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
	delete_erasure m_delete;
	destruct_erasure m_destruct;
};

}    // namespace detail

template<class T>
class shared_ptr
{
public:
	using element_type = T;

// protected:
	class control_blk : public detail::control_blk_base
	{
	protected:

		friend class shared_ptr;

		control_blk(element_type* ep, delete_erasure&& deleter, destruct_erasure&& destruct)
		: detail::control_blk_base{ep, std::move(deleter), std::move(destruct)}
		{}

		element_type*
		get_ptr()
		{
			return static_cast<element_type*>(get_vptr());
		}
	};

	class value_control_block : public control_blk
	{
	public:
		friend class shared_ptr;

		~value_control_block()
		{
			detail::control_blk_base::clear();
		}

		struct use_allocator {};

		template<class... Args>
		value_control_block(Args&&... args)
			: control_blk{&m_value,
						  [](void*) { assert(false); },
						  [](void* p) {
							  auto cp = static_cast<value_control_block*>(p);
							  cp->clear();
							  delete cp;
						  }},
			  m_value(std::forward<Args>(args)...)
		{}

		template<class _Alloc, class... Args>
		value_control_block(use_allocator, _Alloc&& alloc, Args&&... args)
			: control_blk{&m_value,
						  [](void*) mutable { assert(false); },
						  [&alloc](void* p) {
							  _Alloc a{std::forward<_Alloc>(alloc)};
							  auto cp = static_cast<value_control_block*>(p);
							  cp->clear();
							  a.destroy(cp);
							  a.deallocate(cp, 1);
						  }},
			  m_value(std::forward<Args>(args)...)
		{}

		element_type m_value;
	};

public:
	template<class U>
	friend class shared_ptr;

	template<class... Args>
	static shared_ptr
	create(Args&&... args)
	{
		return shared_ptr{new value_control_block{std::forward<Args>(args)...}};
	}

	template<class _Alloc, class... Args>
	static shared_ptr
	allocate(_Alloc&& alloc, Args&&... args)
	{
		using allocator_type = typename _Alloc::template rebind<value_control_block>::other;

		allocator_type salloc{std::forward<_Alloc>(alloc)};
		value_control_block* cp = salloc.allocate(1);
		new(cp) value_control_block(typename value_control_block::use_allocator{}, std::move(salloc), std::forward<Args>(args)...);
		return shared_ptr{cp};
	}

	template<class U>
	static shared_ptr
	static_ptr_cast(shared_ptr<U> const& p)
	{
		auto cp = p.get_ctrl_blk();
		if (cp)
		{
			cp->increment_use_count();
		}
		return shared_ptr{cp};
	}

	template<class U>
	static shared_ptr
	dynamic_ptr_cast(shared_ptr<U> const& p)
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

	shared_ptr(element_type* ep)
		: m_cblk_ptr{new control_blk{ep,
									 [](void* p) {
										 if (p)
										 {
											 delete static_cast<element_type*>(p);
										 }
									 },
									 [](void* p) { delete static_cast<control_blk*>(p); }}},
		  m_elem_ptr{ep}
	{}

	template<class _Del>
	shared_ptr(element_type* ep, _Del&& del)
		: m_cblk_ptr{new control_blk{ep,
									 [del{std::forward<_Del>(del)}](void* p) {
										 if (p)
										 {
											 del(static_cast<element_type*>(p));
										 }
									 },
									 [](void* p) { delete static_cast<control_blk*>(p); }}},
		  m_elem_ptr{ep}
	{}

	template<class _Del, class _Alloc>
	shared_ptr(element_type* ep, _Del&& del, _Alloc&& alloc) : m_cblk_ptr{nullptr}, m_elem_ptr{nullptr}
	{
		using allocator_type = typename _Alloc::template rebind<control_blk>::other;

		allocator_type cballoc{std::forward<_Alloc>(alloc)};
		m_cblk_ptr = cballoc.allocate(1);
		new (m_cblk_ptr) control_blk{ep,
									 [del{std::forward<_Del>(del)}](void* p) {
										 if (p)
										 {
											 del(static_cast<element_type*>(p));
										 }
									 },
									 [cballoc{std::move(cballoc)}](void* p) mutable {
										 cballoc.destroy(static_cast<control_blk*>(p));
										 cballoc.deallocate(static_cast<control_blk*>(p), 1);
									 }


		};
		m_elem_ptr = ep;
	}

	shared_ptr(shared_ptr const& rhs)
	{
		adopt(rhs);
	}

	template<class U, class = typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
	shared_ptr(shared_ptr<U> const& rhs)
	{
		adopt(rhs);
	}

	shared_ptr(shared_ptr&& rhs)
	{
		steal(std::move(rhs));
	}

	template<class U, class = typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
	shared_ptr(shared_ptr<U>&& rhs)
	{
		steal(std::move(rhs));
	}

	shared_ptr(std::nullptr_t)  : m_cblk_ptr{nullptr}, m_elem_ptr{nullptr} {}

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


	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, shared_ptr&>
	operator=(shared_ptr<U> const& rhs)
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

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, bool>
	operator==(shared_ptr<U> const& rhs) const
	{
		return m_elem_ptr == rhs.m_elem_ptr;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, bool>
	operator!=(shared_ptr<U> const& rhs) const
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

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>
	assign(shared_ptr<U> const& rhs)
	{
		disown();
		adopt(rhs);
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>
	assign(shared_ptr<U>&& rhs)
	{
		disown();
		steal(std::move(rhs));
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>
	adopt(shared_ptr<U> const& rhs)
	{
		m_cblk_ptr = rhs.m_cblk_ptr;
		m_elem_ptr = static_cast<element_type*>(rhs.m_elem_ptr);
		if (m_cblk_ptr)
		{
			m_cblk_ptr->increment_use_count();
		}
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>
	steal(shared_ptr<U>&& rhs)
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
				auto cp = m_cblk_ptr;
				m_cblk_ptr = nullptr;
				m_elem_ptr = nullptr;
				cp->destroy();
			}
			else
			{
				m_cblk_ptr = nullptr;
				m_elem_ptr = nullptr;
			}
		}
	}

	detail::control_blk_base*  m_cblk_ptr;
	element_type* m_elem_ptr;
};

template<class T, class... Args>
shared_ptr<T>
make_shared(Args&&... args)
{
	return shared_ptr<T>::create(std::forward<Args>(args)...);
}

}    // namespace util
}    // namespace logicmill

namespace std
{

template<class U>
struct hash<logicmill::util::shared_ptr<U>>
{
	typedef logicmill::util::shared_ptr<U> argument_type;
	typedef std::size_t                    result_type;

	result_type
	operator()(const argument_type& v) const
	{
		return std::hash<void*>()(v.get());
	}
};

}

#endif    // LOGICMILL_UTIL_SHARED_PTR_H