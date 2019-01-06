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
#include <functional>
#include <memory>
#include <boost/compressed_pair.hpp>

namespace logicmill
{
namespace util
{

namespace detail
{

template<class Alloc>
class allocator_deleter
{
	typedef std::allocator_traits<Alloc> alloc_traits;

public:
	using pointer   = typename alloc_traits::pointer;
	using size_type = typename alloc_traits::size_type;

private:
	Alloc&    m_alloc;
	size_type m_size;

public:
	allocator_deleter(Alloc& a, size_type s) noexcept : m_alloc(a), m_size(s) {}
	void
	operator()(pointer p) noexcept
	{
		alloc_traits::destroy(m_alloc, p);
		alloc_traits::deallocate(m_alloc, p, m_size);
	}
};

class ctrl_blk_base
{
public:
	// using delete_erasure = std::function<void(void*)>;
	// using destruct_erasure = std::function<void(void*)>;

	// ~ctrl_blk_base()
	// {
	// 	assert(m_use_count == 0);
	// 	if (m_elem)
	// 	{
	// 		m_delete(m_elem);
	// 	}
	// }
	// void destroy()
	// {
	// 	m_destruct(this);
	// }

	// ctrl_blk_base(void* p, delete_erasure&& deleter, destruct_erasure&& destruct)
	// 	: m_elem{p}, m_use_count{1}, m_delete{std::move(deleter)}, m_destruct{std::move(destruct)}
	// {}

	ctrl_blk_base() : m_use_count{1} {}

	// void*
	// get_vptr() const
	// {
	// 	return m_elem;
	// }

	// void
	// clear()
	// {
	// 	m_elem = nullptr;
	// }

	virtual void
	on_zero_use_count()
			= 0;

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
	// void*       m_elem;
	std::size_t m_use_count;
	// delete_erasure m_delete;
	// destruct_erasure m_destruct;
};

template<class T, class Del, class Alloc>
class ptr_ctrl_blk : public ctrl_blk_base
{
public:
	using element_type   = T;
	using allocator_type = Alloc;
	using deleter_type   = Del;

	template<class _Del, class _Alloc>
	ptr_ctrl_blk(element_type* p, _Del&& del, _Alloc&& alloc)
		: m_data{{p, std::forward<_Del>(del)}, std::forward<_Alloc>(alloc)}
		// : m_ptr{p}, m_del{std::forward<_Del>(del)}, m_alloc{std::forward<_Alloc>(alloc)}
	{}

	virtual void
	on_zero_use_count()
	{
		using ctrl_blk_allocator_type = typename allocator_type::template rebind<ptr_ctrl_blk>::other;

		get_deleter()(get_elem_ptr());
		get_deleter().~deleter_type();

		ctrl_blk_allocator_type cblk_alloc(get_alloc());
		get_alloc().~allocator_type();
		cblk_alloc.deallocate(this, 1);
	}

	element_type*
	get_ptr() const noexcept
	{
		return get_elem_ptr();
	}

private:
	using element_pointer = element_type*;
	using data_type = boost::compressed_pair<boost::compressed_pair<element_pointer, deleter_type>, allocator_type>;
	data_type m_data;

	element_pointer& get_elem_ptr() { return m_data.first().first(); }
	deleter_type& get_deleter() { return m_data.first().second(); }
	allocator_type& get_alloc() {  return m_data.second(); }

	// element_type* m_ptr;
	// Del           m_del;
	// Alloc         m_alloc;
};

template<class T, class Alloc>
class value_ctrl_blk : public ctrl_blk_base
{
public:
	using element_type   = T;
	using allocator_type = Alloc;

	template<class _Alloc, class... Args>
	value_ctrl_blk(_Alloc&& alloc, Args&&... args)
		: m_value{std::forward<Args>(args)...}, m_alloc{std::forward<_Alloc>(alloc)}
	{}

	element_type*
	get_ptr() noexcept
	{
		return &m_value;
	}

	virtual void
	on_zero_use_count()
	{
		using ctrl_blk_allocator_type = typename allocator_type::template rebind<value_ctrl_blk>::other;
		m_value.~element_type();
		ctrl_blk_allocator_type cblk_alloc(m_alloc);
		m_alloc.~allocator_type();
		cblk_alloc.deallocate(this, 1);
	}

private:
	element_type   m_value;
	allocator_type m_alloc;
};

}    // namespace detail

template<class T>
class shared_ptr
{
private:
	struct sig_flag
	{
		int iflag;
	};

public:
	using element_type = T;

	// protected:

#if 0

	class ptr_ctrl_blk : public detail::ctrl_blk_base
	{
	protected:

		friend class shared_ptr;

		ptr_ctrl_blk(element_type* ep, delete_erasure&& deleter, destruct_erasure&& destruct)
		: detail::ctrl_blk_base{ep, std::move(deleter), std::move(destruct)}
		{}

		element_type*
		get_ptr()
		{
			return static_cast<element_type*>(get_vptr());
		}
	private:
		element_type* m_elem;

	};

	class value_control_block : public ptr_ctrl_blk
	{
	public:
		friend class shared_ptr;

		~value_control_block()
		{
			detail::ctrl_blk_base::clear();
		}

		struct use_allocator {};

		template<class... Args>
		value_control_block(Args&&... args)
			: ptr_ctrl_blk{&m_value,
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
			: ptr_ctrl_blk{&m_value,
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
#endif

public:
	template<class U>
	friend class shared_ptr;

	template<class U, class... _Args>
	friend shared_ptr<U>
	make_shared(_Args&&...);

	template<class U, class _Alloc, class... _Args>
	friend shared_ptr<U>
	allocate_shared(_Alloc&&, _Args&&...);

	template<class U, class V>
	friend shared_ptr<U>
	dynamic_pointer_cast(shared_ptr<V> const&);

	template<class U, class V>
	friend shared_ptr<U>
	static_pointer_cast(shared_ptr<V> const&);

protected:
	template<class... Args>
	static shared_ptr
	create(Args&&... args)
	{
		using allocator_type      = std::allocator<element_type>;
		using deleter_type        = detail::allocator_deleter<allocator_type>;
		using ctrl_blk_type       = detail::value_ctrl_blk<element_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		allocator_type alloc{};
		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(std::move(alloc), std::forward<Args>(args)...);
		return shared_ptr{cblk_ptr, cblk_ptr->get_ptr()};
	}

	template<class _Alloc, class... Args>
	static shared_ptr
	allocate(_Alloc&& alloc, Args&&... args)
	{
		using allocator_type      = _Alloc;
		using deleter_type        = detail::allocator_deleter<allocator_type>;
		using ctrl_blk_type       = detail::value_ctrl_blk<element_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(std::forward<_Alloc>(alloc), std::forward<Args>(args)...);
		return shared_ptr{cblk_ptr, cblk_ptr->get_ptr()};
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
		return shared_ptr{cp, static_cast<element_type*>(p.get())};
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

public:
	shared_ptr() : m_cblk_ptr{nullptr}, m_elem_ptr{nullptr} {}

	shared_ptr(element_type* ep) : m_cblk_ptr{nullptr}, m_elem_ptr{ep}
	{
		using allocator_type      = std::allocator<element_type>;
		using deleter_type        = detail::allocator_deleter<allocator_type>;
		using ctrl_blk_type       = detail::ptr_ctrl_blk<element_type, deleter_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		allocator_type alloc{};
		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(ep, deleter_type{alloc, 1}, std::move(alloc));
		m_cblk_ptr = cblk_ptr;
	}

	template<class _Del>
	shared_ptr(element_type* ep, _Del&& del) : m_cblk_ptr{nullptr}, m_elem_ptr{ep}
	{
		using allocator_type      = std::allocator<element_type>;
		using deleter_type        = _Del;
		using ctrl_blk_type       = detail::ptr_ctrl_blk<element_type, deleter_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		allocator_type alloc{};
		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(ep, std::forward<_Del>(del), std::move(alloc));
		m_cblk_ptr = cblk_ptr;
	}

	template<class _Del, class _Alloc>
	shared_ptr(element_type* ep, _Del&& del, _Alloc&& alloc) : m_cblk_ptr{nullptr}, m_elem_ptr{ep}
	{

		using allocator_type      = _Alloc;
		using deleter_type        = _Del;
		using ctrl_blk_type       = detail::ptr_ctrl_blk<element_type, deleter_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(ep, std::forward<_Del>(del), std::forward<_Alloc>(alloc));
		m_cblk_ptr = cblk_ptr;
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

	shared_ptr(std::nullptr_t) : m_cblk_ptr{nullptr}, m_elem_ptr{nullptr} {}


#if 1

	template<class U, class _Del>
	shared_ptr(
			std::unique_ptr<U, _Del>&& uptr,
			typename std::enable_if_t<
					!std::is_lvalue_reference<_Del>::value && !std::is_array<U>::value
							&& std::is_convertible<typename std::unique_ptr<U, _Del>::pointer, element_type*>::value,
					sig_flag> = sig_flag{})
		: m_elem_ptr(uptr.get())
	{
		if (m_elem_ptr == nullptr)
		{
			m_cblk_ptr = nullptr;
		}
		else
		{
			using allocator_type      = std::allocator<element_type>;
			using deleter_type        = _Del;
			using ctrl_blk_type       = detail::ptr_ctrl_blk<element_type, deleter_type, allocator_type>;
			using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

			allocator_type alloc;
			ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
			new (cblk_ptr) ctrl_blk_type(uptr.get(), uptr.get_deleter(), std::move(alloc));
			m_cblk_ptr = cblk_ptr;
		}
		uptr.release();
	}

	template<class U, class _Del>
	shared_ptr(
			std::unique_ptr<U, _Del>&& uptr,
			typename std::enable_if_t<
					std::is_lvalue_reference<_Del>::value && !std::is_array<U>::value
							&& std::is_convertible<typename std::unique_ptr<U, _Del>::pointer, element_type*>::value,
					sig_flag> = sig_flag{})
		: m_elem_ptr(uptr.get())
	{
		if (m_elem_ptr == nullptr)
		{
			m_cblk_ptr = nullptr;
		}
		else
		{
			using allocator_type      = std::allocator<element_type>;
			using deleter_type        = std::reference_wrapper<typename std::remove_reference<_Del>::type>;
			using ctrl_blk_type       = detail::ptr_ctrl_blk<element_type, deleter_type, allocator_type>;
			using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

			allocator_type alloc;
			ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
			new (cblk_ptr) ctrl_blk_type(uptr.get(), std::ref(uptr.get_deleter()), std::move(alloc));
			m_cblk_ptr = cblk_ptr;
		}
		uptr.release();
	}

#endif

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

	detail::ctrl_blk_base*
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

	bool operator==(std::nullptr_t) const
	{
		return !bool(*this);
	}

	bool operator!=(std::nullptr_t) const
	{
		return bool(*this);
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
	shared_ptr(detail::ctrl_blk_base* p, element_type* ep) : m_cblk_ptr{p}, m_elem_ptr{ep} {}

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
				auto cblk_ptr = m_cblk_ptr;
				m_cblk_ptr    = nullptr;
				m_elem_ptr    = nullptr;
				cblk_ptr->on_zero_use_count();
			}
			else
			{
				m_cblk_ptr = nullptr;
				m_elem_ptr = nullptr;
			}
		}
	}

	detail::ctrl_blk_base* m_cblk_ptr;
	element_type*          m_elem_ptr;
};

template<class T, class... Args>
shared_ptr<T>
make_shared(Args&&... args)
{
	return shared_ptr<T>::create(std::forward<Args>(args)...);
}

template<class T, class Alloc, class... Args>
shared_ptr<T>
allocate_shared(Alloc&& alloc, Args&&... args)
{
	return shared_ptr<T>::allocate(std::forward<Alloc>(alloc), std::forward<Args>(args)...);
}

template<class T, class U>
shared_ptr<T>
dynamic_pointer_cast(shared_ptr<U> const& uptr)
{
	return shared_ptr<T>::dynamic_ptr_cast(uptr);
}

template<class T, class U>
shared_ptr<T>
static_pointer_cast(shared_ptr<U> const& uptr)
{
	return shared_ptr<T>::static_ptr_cast(uptr);
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

}    // namespace std

#endif    // LOGICMILL_UTIL_SHARED_PTR_H