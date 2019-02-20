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


#define USE_STD_SHARED_PTR 0

#if (USE_STD_SHARED_PTR)
#define SHARED_PTR_TYPE std::shared_ptr
#define WEAK_PTR_TYPE std::weak_ptr
#define MAKE_SHARED std::make_shared
#define DYNAMIC_POINTER_CAST std::dynamic_pointer_cast
#define ENABLE_SHARED_FROM_THIS std::enable_shared_from_this
#else
#define SHARED_PTR_TYPE logicmill::util::shared_ptr
#define WEAK_PTR_TYPE logicmill::util::weak_ptr
#define MAKE_SHARED logicmill::util::make_shared
#define DYNAMIC_POINTER_CAST logicmill::util::dynamic_pointer_cast
#define ENABLE_SHARED_FROM_THIS logicmill::util::enable_shared_from_this
#endif


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

class ctrl_blk
{
public:
	ctrl_blk() : m_use_count{1}, m_weak_count{1} {}

	virtual void
	on_zero_use_count()
			= 0;

	virtual void
	on_zero_weak_count()
			= 0;

	long
	increment_use_count()
	{
		return ++m_use_count;
	}

	long
	decrement_use_count()
	{
		return --m_use_count;
	}

	long
	increment_weak_count()
	{
		return ++m_weak_count;
	}

	long
	decrement_weak_count()
	{
		return --m_weak_count;
	}

	long
	use_count() const
	{
		return m_use_count;
	}

	long
	weak_count() const
	{
		return m_weak_count;
	}

	ctrl_blk*
	lock()
	{
		ctrl_blk* result{nullptr};

		if (use_count() > 0)
		{
			increment_use_count();
			result = this;
		}
		return result;
	}

private:
	long m_use_count;
	long m_weak_count;
};

template<class T, class Del, class Alloc>
class ptr_ctrl_blk : public ctrl_blk, public Del, public Alloc
{
public:
	using element_type   = T;
	using allocator_type = Alloc;
	using deleter_type   = Del;

	template<class _Del, class _Alloc>
	ptr_ctrl_blk(element_type* p, _Del&& del, _Alloc&& alloc)
		: Del{std::forward<_Del>(del)}, Alloc{std::forward<_Alloc>(alloc)}, m_ptr{p}
	{}

	virtual void
	on_zero_use_count()
	{
		static_cast<Del&> (*this)(m_ptr);
		static_cast<Del&>(*this).~deleter_type();
		m_ptr = nullptr;

		assert(weak_count() >= 1);
		if (decrement_weak_count() == 0)
		{
			on_zero_weak_count();
		}
	}

	virtual void
	on_zero_weak_count()
	{
		assert(use_count() == 0);
		using ctrl_blk_allocator_type = typename allocator_type::template rebind<ptr_ctrl_blk>::other;
		ctrl_blk_allocator_type cblk_alloc(static_cast<Alloc&>(*this));
		static_cast<Alloc&>(*this).~allocator_type();
		cblk_alloc.deallocate(this, 1);
	}

	element_type*
	get_ptr() const noexcept
	{
		return m_ptr;
	}

private:
	using element_pointer = element_type*;

	element_type* m_ptr;
};

template<class T, class Alloc>
class value_ctrl_blk : public ctrl_blk, public Alloc
{
public:
	using element_type   = T;
	using allocator_type = Alloc;

	template<class _Alloc, class... Args>
	value_ctrl_blk(_Alloc&& alloc, Args&&... args)
		: Alloc{std::forward<Alloc>(alloc)}, m_value{std::forward<Args>(args)...}
	{}

	element_type*
	get_ptr() noexcept
	{
		return &m_value;
	}

	virtual void
	on_zero_use_count()
	{
		m_value.~element_type();
		assert(weak_count() >= 1);
		if (decrement_weak_count() == 0)
		{
			on_zero_weak_count();
		}
	}

	virtual void
	on_zero_weak_count()
	{
		assert(use_count() == 0);
		using ctrl_blk_allocator_type = typename allocator_type::template rebind<value_ctrl_blk>::other;
		ctrl_blk_allocator_type cblk_alloc(static_cast<Alloc&>(*this));
		static_cast<Alloc&>(*this).~allocator_type();
		cblk_alloc.deallocate(this, 1);
	}

private:
	element_type m_value;
};

template<class T>
class pstate
{
public:
	using element_type = T;

	struct sig_flag
	{
		int iflag;
	};

	template<class U>
	friend class pstate;

	pstate(ctrl_blk* cp, element_type* p) : m_ctrl{cp}, m_ptr{p} {}
	pstate(element_type* p) : m_ctrl{nullptr}, m_ptr{p} {}

	template<class U>
	pstate(ctrl_blk* cp, U* p, typename std::enable_if_t<std::is_convertible<U*, T*>::value, sig_flag> = sig_flag{})
		: m_ctrl{cp}, m_ptr{static_cast<T*>(p)}
	{}

	pstate(pstate&& rhs) : m_ctrl{rhs.m_ctrl}, m_ptr{rhs.m_ptr}
	{
		rhs.m_ctrl = nullptr;
		rhs.m_ptr  = nullptr;
	}

	template<class U>
	pstate(pstate<U>&& rhs, typename std::enable_if_t<std::is_convertible<U*, T*>::value, sig_flag> = sig_flag{})
		: m_ctrl{rhs.m_ctrl}, m_ptr{static_cast<T*>(rhs.m_ptr)}
	{
		rhs.m_ctrl = nullptr;
		rhs.m_ptr  = nullptr;
	}

	pstate(pstate const& rhs) : m_ctrl{rhs.m_ctrl}, m_ptr{rhs.m_ptr} {}

	template<class U>
	pstate(pstate<U> const& rhs, typename std::enable_if_t<std::is_convertible<U*, T*>::value, sig_flag> = sig_flag{})
		: m_ctrl{rhs.m_ctrl}, m_ptr{static_cast<T*>(rhs.m_ptr)}
	{}

	pstate() : m_ctrl{nullptr}, m_ptr{nullptr} {}

	ctrl_blk*
	ctrl() const
	{
		return m_ctrl;
	}

	void
	ctrl(ctrl_blk* cp)
	{
		m_ctrl = cp;
	}

	element_type*
	ptr() const
	{
		return m_ptr;
	}

	void
	ptr(element_type* p)
	{
		m_ptr = p;
	}

	void
	swap(pstate& rhs) noexcept
	{
		std::swap(m_ctrl, rhs.m_ctrl);
		std::swap(m_ptr, rhs.m_ptr);
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible_v<U*, T*>>
	swap(pstate<U>& rhs) noexcept
	{
		std::swap(m_ctrl, rhs.m_ctrl);
		std::swap(m_ptr, rhs.m_ptr);
	}

	pstate&
	operator=(pstate const& rhs)
	{
		pstate{rhs}.swap(*this);
		return *this;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible_v<U*, T*>, pstate&>
	operator=(pstate<U> const& rhs)
	{
		pstate{rhs}.swap(*this);
		return *this;
	}

	pstate&
	operator=(pstate&& rhs)
	{
		pstate{std::move(rhs)}.swap(*this);
		return *this;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible_v<U*, T*>, pstate&>
	operator=(pstate<U>&& rhs)
	{
		pstate{std::move(rhs)}.swap(*this);
		return *this;
	}

private:
	ctrl_blk*     m_ctrl;
	element_type* m_ptr;
};

}    // namespace detail

template<class U>
class weak_ptr;

template<class U> 
class enable_shared_from_this;

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

public:
	template<class U>
	friend class shared_ptr;

	template<class U>
	friend class weak_ptr;

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
    	static_assert( std::is_constructible<T, Args...>::value, "Can't construct object in make_shared" );
		using allocator_type      = std::allocator<element_type>;
		using ctrl_blk_type       = detail::value_ctrl_blk<element_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		allocator_type alloc{};
		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(std::move(alloc), std::forward<Args>(args)...);
		shared_ptr result{dynamic_cast<detail::ctrl_blk*>(cblk_ptr), cblk_ptr->get_ptr()};
		result.enable_weak_this(result.m_state.ptr(), result.m_state.ptr());
		return result;
	}

	template<class _Alloc, class... Args>
	static shared_ptr
	allocate(_Alloc&& alloc, Args&&... args)
	{
    	static_assert( std::is_constructible<T, Args...>::value, "Can't construct object in allocate_shared" );
		using allocator_type      = _Alloc;
		using ctrl_blk_type       = detail::value_ctrl_blk<element_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(std::forward<_Alloc>(alloc), std::forward<Args>(args)...);
		shared_ptr result{dynamic_cast<detail::ctrl_blk*>(cblk_ptr), cblk_ptr->get_ptr()};
		result.enable_weak_this(result.m_state.ptr(), result.m_state.ptr());
		return result;
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
	shared_ptr() : m_state{} {}

	shared_ptr(element_type* ep) : m_state{ep}
	{
		using allocator_type      = std::allocator<element_type>;
		using deleter_type        = detail::allocator_deleter<allocator_type>;
		using ctrl_blk_type       = detail::ptr_ctrl_blk<element_type, deleter_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		allocator_type alloc{};
		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(ep, deleter_type{alloc, 1}, std::move(alloc));
		// m_cblk_ptr = cblk_ptr;
		m_state.ctrl(cblk_ptr);
		enable_weak_this(ep, ep);
	}

	template<class _Del>
	shared_ptr(element_type* ep, _Del&& del) : m_state{ep}
	{
		using allocator_type      = std::allocator<element_type>;
		using deleter_type        = _Del;
		using ctrl_blk_type       = detail::ptr_ctrl_blk<element_type, deleter_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		allocator_type alloc{};
		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(ep, std::forward<_Del>(del), std::move(alloc));
		// m_cblk_ptr = cblk_ptr;
		m_state.ctrl(cblk_ptr);
		enable_weak_this(ep, ep);
	}

	template<class _Del, class _Alloc>
	shared_ptr(element_type* ep, _Del&& del, _Alloc&& alloc) : m_state{ep}
	{

		using allocator_type      = _Alloc;
		using deleter_type        = _Del;
		using ctrl_blk_type       = detail::ptr_ctrl_blk<element_type, deleter_type, allocator_type>;
		using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

		ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
		new (cblk_ptr) ctrl_blk_type(ep, std::forward<_Del>(del), std::forward<_Alloc>(alloc));
		m_state.ctrl(cblk_ptr);
		enable_weak_this(ep, ep);
	}

	shared_ptr(shared_ptr const& rhs) : m_state{rhs.m_state}
	{
		if (m_state.ctrl())
		{
			m_state.ctrl()->increment_use_count();
		}
	}

	template<class U, class = typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
	shared_ptr(shared_ptr<U> const& rhs) : m_state{rhs.m_state}
	{
		if (m_state.ctrl())
		{
			m_state.ctrl()->increment_use_count();
		}
	}

	shared_ptr(shared_ptr&& rhs) : m_state{std::move(rhs.m_state)} {}

	template<class U, class = typename std::enable_if_t<std::is_convertible<U*, element_type*>::value>>
	shared_ptr(shared_ptr<U>&& rhs) : m_state{std::move(rhs.m_state)}
	{}

	shared_ptr(std::nullptr_t) : m_state{} {}

	template<class U>
	shared_ptr(
			weak_ptr<U> const& wp,
			typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, sig_flag> = sig_flag{});

	template<class U, class _Del>
	shared_ptr(
			std::unique_ptr<U, _Del>&& uptr,
			typename std::enable_if_t<
					!std::is_lvalue_reference<_Del>::value && !std::is_array<U>::value
							&& std::is_convertible<typename std::unique_ptr<U, _Del>::pointer, element_type*>::value,
					sig_flag> = sig_flag{})
		: m_state{uptr.get()}
	{
		if (!m_state.ptr())
		{
			m_state.ctrl(nullptr);
		}
		else
		{
			using uptr_element_type   = U;
			using uptr_deleter_type   = _Del;
			using allocator_type      = std::allocator<uptr_element_type>;
			using ctrl_blk_type       = detail::ptr_ctrl_blk<uptr_element_type, uptr_deleter_type, allocator_type>;
			using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

			allocator_type alloc;
			ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
			new (cblk_ptr) ctrl_blk_type(uptr.get(), uptr.get_deleter(), std::move(alloc));
			m_state.ctrl(cblk_ptr);
			enable_weak_this(uptr.get(), uptr.get());
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
		: m_state{uptr.get()}
	{
		if (!m_state.ptr())
		{
			m_state.ctrl(nullptr);
		}
		else
		{
			using uptr_element_type   = U;
			using uptr_deleter_type   = _Del;
			using allocator_type      = std::allocator<uptr_element_type>;
			using ctrl_blk_type       = detail::ptr_ctrl_blk<uptr_element_type, uptr_deleter_type, allocator_type>;
			using ctrl_blk_alloc_type = typename allocator_type::template rebind<ctrl_blk_type>::other;

			allocator_type alloc;
			ctrl_blk_type* cblk_ptr = ctrl_blk_alloc_type{alloc}.allocate(1);
			new (cblk_ptr) ctrl_blk_type(uptr.get(), std::ref(uptr.get_deleter()), std::move(alloc));
			m_state.ctrl(cblk_ptr);
			enable_weak_this(uptr.get(), uptr.get());
		}
		uptr.release();
	}

    template<class U>
	shared_ptr(shared_ptr<U> const& rhs, element_type* p) noexcept;

	~shared_ptr()
	{
		if (m_state.ctrl())
		{
			assert(m_state.ctrl()->use_count() > 0);
			if (m_state.ctrl()->decrement_use_count() == 0)
			{
				auto cblk_ptr = m_state.ctrl();
				m_state.ctrl(nullptr);
				m_state.ptr(nullptr);
				cblk_ptr->on_zero_use_count();
			}
			else
			{
				m_state.ctrl(nullptr);
				m_state.ptr(nullptr);
			}
		}
	}

	void
	swap(shared_ptr& rhs) noexcept
	{
		m_state.swap(rhs.m_state);
	}

	shared_ptr&
	operator=(shared_ptr const& rhs)
	{
		shared_ptr{rhs}.swap(*this);
		return *this;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, shared_ptr&>
	operator=(shared_ptr<U> const& rhs)
	{
		shared_ptr{rhs}.swap(*this);
		return *this;
	}

	shared_ptr&
	operator=(shared_ptr&& rhs)
	{
		shared_ptr{std::move(rhs)}.swap(*this);
		return *this;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, shared_ptr&>
	operator=(shared_ptr<U>&& rhs)
	{
		shared_ptr{std::move(rhs)}.swap(*this);
		return *this;
	}

	detail::ctrl_blk*
	get_ctrl_blk() const
	{
		return m_state.ctrl();
	}

	void
	reset()
	{
		shared_ptr{}.swap(*this);
	}

	explicit operator bool() const
	{
		return m_state.ptr() != nullptr;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, bool>
	operator==(shared_ptr<U> const& rhs) const
	{
		return m_state.ptr() == rhs.m_state.ptr();
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
		return m_state.ptr();
	}

	element_type* operator->() const
	{
		return m_state.ptr();
	}

	element_type& operator*() const
	{
		return *m_state.ptr();
	}

	long
	use_count() const
	{
		return m_state.ctrl() ? m_state.ctrl()->use_count() : 0;
	}

	long
	weak_count() const noexcept
	{
		return m_state.ctrl() ? m_state.ctrl()->weak_count() : 0;
	}


private:
	shared_ptr(detail::ctrl_blk* p, element_type* ep) : m_state{p, ep} {}

    template <class Y, class U>
        typename std::enable_if_t<std::is_convertible<U*, const enable_shared_from_this<Y>*>::value>
        enable_weak_this(const enable_shared_from_this<Y>* ep,
                           U* p) _NOEXCEPT
        {
            typedef typename std::remove_cv<Y>::type RawY;
            if (ep && ep->m_weak_this.expired())
            {
                ep->m_weak_this = shared_ptr<RawY>(*this,
                    const_cast<RawY*>(static_cast<const Y*>(p)));
            }
        }

    void enable_weak_this(...) noexcept {}


	detail::pstate<element_type> m_state;
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

template<class T>
class weak_ptr
{
public:
	typedef T element_type;

private:
	detail::pstate<T> m_state;

	struct sig_flag
	{
		int iflag;
	};

public:
	weak_ptr() noexcept : m_state{} {}

	template<class U>
	weak_ptr(
			shared_ptr<U> const& sp,
			typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, sig_flag> = sig_flag{}) noexcept
		: m_state{sp.m_state}
	{
		if (m_state.ctrl())
		{
			m_state.ctrl()->increment_weak_count();
		}
	}

	weak_ptr(weak_ptr const& wp) noexcept : m_state{wp.m_state}
	{
		if (m_state.ctrl())
		{
			m_state.ctrl()->increment_weak_count();
		}
	}

	template<class U>
	weak_ptr(
			weak_ptr<U> const& wp,
			typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, sig_flag> = sig_flag{}) noexcept
		: m_state{wp.m_state}
	{
		if (m_state.ctrl())
		{
			m_state.ctrl()->increment_weak_count();
		}
	}

	weak_ptr(weak_ptr&& wp) noexcept : m_state{std::move(wp.m_state)} {}

	template<class U>
	weak_ptr(
			weak_ptr<U>&& wp,
			typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, sig_flag> = sig_flag{}) noexcept
		: m_state{std::move(wp.m_state)}
	{}

	~weak_ptr()
	{
		if (m_state.ctrl())
		{
			assert(m_state.ctrl()->weak_count() > 0);
			if (m_state.ctrl()->decrement_weak_count() == 0)
			{
				assert(m_state.ctrl()->use_count() == 0);
				auto cblk_ptr = m_state.ctrl();
				m_state.ctrl(nullptr);
				m_state.ptr(nullptr);
				cblk_ptr->on_zero_weak_count();
			}
			else
			{
				m_state.ctrl(nullptr);
				m_state.ptr(nullptr);
			}
		}
	}

	weak_ptr&
	operator=(weak_ptr const& rhs) noexcept
	{
		weak_ptr{rhs}.swap(*this);
		return *this;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, weak_ptr&>
	operator=(weak_ptr<U> const& rhs) noexcept
	{
		weak_ptr{rhs}.swap(*this);
		return *this;
	}

	weak_ptr&
	operator=(weak_ptr&& rhs) noexcept
	{
		weak_ptr{std::move(rhs)}.swap(*this);
		return *this;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, weak_ptr&>
	operator=(weak_ptr<U>&& rhs) noexcept
	{
		weak_ptr{std::move(rhs)}.swap(*this);
		return *this;
	}

	template<class U>
	typename std::enable_if_t<std::is_convertible<U*, element_type*>::value, weak_ptr&>
	operator=(shared_ptr<U> const& rhs) noexcept
	{
		weak_ptr{rhs}.swap(*this);
		return *this;
	}

	void
	swap(weak_ptr& rhs) noexcept
	{
		m_state.swap(rhs.m_state);
	}

	void
	reset() noexcept
	{
		weak_ptr{}.swap(*this);
	}

	long
	use_count() const noexcept
	{
		return m_state.ctrl() ? m_state.ctrl()->use_count() : 0;
	}

	long
	weak_count() const noexcept
	{
		return m_state.ctrl() ? m_state.ctrl()->weak_count() : 0;
	}

	bool
	expired() const noexcept
	{
		return m_state.ctrl() == 0 || m_state.ctrl()->use_count() == 0;
	}

	shared_ptr<element_type>
	lock() const noexcept
	{
		shared_ptr<element_type> result;
		if (m_state.ctrl())
		{
			result.m_state.ctrl(m_state.ctrl()->lock());
		}
		else
		{
			result.m_state.ctrl(nullptr);
		}
		if (result.m_state.ctrl())
		{
			result.m_state.ptr(m_state.ptr());
		}
		return result;
	}

	template<class U>
	friend class weak_ptr;
	template<class U>
	friend class shared_ptr;
};

template<class T>
inline void
swap(weak_ptr<T>& x, weak_ptr<T>& y) noexcept
{
	x.swap(y);
}

template<class T>
template<class U>
shared_ptr<T>::shared_ptr(
		weak_ptr<U> const& wp,
		typename std::enable_if_t<std::is_convertible<U*, T*>::value, sig_flag>)
	: m_state{(wp.m_state.ctrl() ? wp.m_state.ctrl()->lock() : nullptr), wp.m_state.ptr()}
{
	if (!m_state.ctrl())
	{
		throw std::bad_weak_ptr{};
	}
}


template<class T>
template<class U>
shared_ptr<T>::shared_ptr(shared_ptr<U> const& rhs, element_type* p) noexcept
: m_state{rhs.m_state.ctrl(), p}
{
    if (m_state.ctrl())
			m_state.ctrl()->increment_use_count();
}


template<class T>
class enable_shared_from_this
{
    mutable weak_ptr<T> m_weak_this;
protected:
    enable_shared_from_this() noexcept {}
    enable_shared_from_this(enable_shared_from_this const&) noexcept {}
    enable_shared_from_this& operator=(enable_shared_from_this const&) noexcept
        {return *this;}
    ~enable_shared_from_this() {}
public:
    shared_ptr<T> shared_from_this()
        {return shared_ptr<T>(m_weak_this);}
    
    shared_ptr<T const> shared_from_this() const
        {return shared_ptr<const T>(m_weak_this);}

    weak_ptr<T> weak_from_this() noexcept
       { return m_weak_this; }

    weak_ptr<const T> weak_from_this() const noexcept
        { return m_weak_this; }

    template <class U> friend class shared_ptr;
};


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