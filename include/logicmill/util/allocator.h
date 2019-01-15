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
 * This code is based on http://jrruethe.github.io/blog/2015/11/22/allocators/,
 * from the blog of Joseph Ruether.
 */

#ifndef LOGICMILL_UTIL_ALLOCATOR_H
#define LOGICMILL_UTIL_ALLOCATOR_H

#include <cstddef>
#include <new>

#define UTIL_ALLOCATOR_ALIAS(_alias_name_, _policy_name_, ...)                                                         \
	using _alias_name_ = logicmill::util::allocator<__VA_ARGS__, _policy_name_<__VA_ARGS__>>;
/**/

#define UTIL_ALLOCATOR_POLICY(_policy_name_, _alloc_func_name_, _free_func_name_)                                      \
	template<typename T>                                                                                               \
	class _policy_name_                                                                                                \
	{                                                                                                                  \
	public:                                                                                                            \
		UTIL_ALLOCATOR_TRAITS(T)                                                                                       \
		template<class U>                                                                                              \
		struct rebind                                                                                                  \
		{                                                                                                              \
			using other = _policy_name_<U>;                                                                            \
		};                                                                                                             \
		_policy_name_(void) {}                                                                                         \
		template<class U>                                                                                              \
		_policy_name_(_policy_name_<U> const& other)                                                                   \
		{}                                                                                                             \
		pointer                                                                                                        \
		allocate(size_type count, const_pointer /* hint */ = 0)                                                        \
		{                                                                                                              \
			if (count > max_size())                                                                                    \
			{                                                                                                          \
				throw std::bad_alloc();                                                                                \
			}                                                                                                          \
			return static_cast<pointer>(_alloc_func_name_(count * sizeof(type)));                                      \
		}                                                                                                              \
		void                                                                                                           \
		deallocate(pointer ptr, size_type count)                                                                       \
		{                                                                                                              \
			_free_func_name_(ptr, count * sizeof(type));                                                               \
		}                                                                                                              \
		size_type                                                                                                      \
		max_size(void) const                                                                                           \
		{                                                                                                              \
			return max_allocations<T>::value;                                                                          \
		}                                                                                                              \
	};                                                                                                                 \
	template<typename T, typename TraitsT, typename U, typename TraitsU>                                               \
	bool operator==(                                                                                                   \
			allocator<T, _policy_name_<T>, TraitsT> const& left, allocator<U, _policy_name_<U>, TraitsU> const& right) \
	{                                                                                                                  \
		return true;                                                                                                   \
	}                                                                                                                  \
	template<typename T, typename TraitsT, typename U, typename TraitsU>                                               \
	bool operator!=(                                                                                                   \
			allocator<T, _policy_name_<T>, TraitsT> const& left, allocator<U, _policy_name_<U>, TraitsU> const& right) \
	{                                                                                                                  \
		return !(left == right);                                                                                       \
	}
/**/

#define UTIL_ALLOCATOR_TRAITS(T)                                                                                       \
	using type            = T;                                                                                         \
	using value_type      = type;                                                                                      \
	using pointer         = value_type*;                                                                               \
	using const_pointer   = value_type const*;                                                                         \
	using reference       = value_type&;                                                                               \
	using const_reference = value_type const&;                                                                         \
	using size_type       = std::size_t;                                                                               \
	using difference_type = std::ptrdiff_t;
/**/

namespace logicmill
{
namespace util
{

template<class T>
class allocated_object_traits
{
public:
	using type = T;

	template<typename U>
	struct rebind
	{
		typedef allocated_object_traits<U> other;
	};

	allocated_object_traits(void) {}

	template<typename U>
	allocated_object_traits(allocated_object_traits<U> const& other)
	{}

	type*
	address(type& obj) const
	{
		return &obj;
	}
	type const*
	address(type const& obj) const
	{
		return &obj;
	}

	void
	construct(type* ptr, type const& ref) const
	{
		// In-place copy construct
		new (ptr) type(ref);
	}

	void
	destroy(type* ptr) const
	{
		// Call destructor
		ptr->~type();
	}
};

template<class T>
struct max_allocations
{
	enum
	{
		value = static_cast<std::size_t>(-1) / sizeof(T)
	};
};

template<typename T>
class heap_allocator_policy
{
public:
	UTIL_ALLOCATOR_TRAITS(T)

	template<class U>
	struct rebind
	{
		using other = heap_allocator_policy<U>;
	};

	// Default Constructor
	heap_allocator_policy(void) {}

	// Copy Constructor
	template<class U>
	heap_allocator_policy(heap_allocator_policy<U> const& other)
	{}

	// Allocate memory
	pointer
	allocate(size_type count, const_pointer /* hint */ = 0)
	{
		if (count > max_size())
		{
			throw std::bad_alloc();
		}
		return static_cast<pointer>(::operator new(count * sizeof(type), ::std::nothrow));
	}

	// Delete memory
	void
	deallocate(pointer ptr, size_type /* count */)
	{
		::operator delete(ptr);
	}

	// Max number of objects that can be allocated in one call
	size_type
	max_size(void) const
	{
		return max_allocations<T>::value;
	}
};

#define UTIL_FORWARD_ALLOCATOR_TRAITS(C)                                                                               \
	using value_type      = typename C::value_type;                                                                    \
	using pointer         = typename C::pointer;                                                                       \
	using const_pointer   = typename C::const_pointer;                                                                 \
	using reference       = typename C::reference;                                                                     \
	using const_reference = typename C::const_reference;                                                               \
	using size_type       = typename C::size_type;                                                                     \
	using difference_type = typename C::difference_type;
/**/

template<typename T, typename PolicyT = heap_allocator_policy<T>, typename TraitsT = allocated_object_traits<T>>
class allocator : public PolicyT, public TraitsT
{
public:
	// Template parameters
	using Policy = PolicyT;
	using Traits = TraitsT;

	UTIL_FORWARD_ALLOCATOR_TRAITS(Policy)

	template<typename U>
	struct rebind
	{
		typedef allocator<U, typename Policy::template rebind<U>::other, typename Traits::template rebind<U>::other>
				other;
	};

	// Constructor
	allocator(void) {}

	// Copy Constructor
	template<typename U, typename PolicyU, typename TraitsU>
	allocator(allocator<U, PolicyU, TraitsU> const& other) : Policy(other), Traits(other)
	{}
};

// Two allocators are not equal unless a specialization says so
template<typename T, typename PolicyT, typename TraitsT, typename U, typename PolicyU, typename TraitsU>
bool
operator==(allocator<T, PolicyT, TraitsT> const& left, allocator<U, PolicyU, TraitsU> const& right)
{
	return false;
}

// Also implement inequality
template<typename T, typename PolicyT, typename TraitsT, typename U, typename PolicyU, typename TraitsU>
bool
operator!=(allocator<T, PolicyT, TraitsT> const& left, allocator<U, PolicyU, TraitsU> const& right)
{
	return !(left == right);
}

// Comparing an allocator to anything else should not show equality
template<typename T, typename PolicyT, typename TraitsT, typename OtherAllocator>
bool
operator==(allocator<T, PolicyT, TraitsT> const& left, OtherAllocator const& right)
{
	return false;
}

// Also implement inequality
template<typename T, typename PolicyT, typename TraitsT, typename OtherAllocator>
bool
operator!=(allocator<T, PolicyT, TraitsT> const& left, OtherAllocator const& right)
{
	return !(left == right);
}

// Specialize for the heap policy
template<typename T, typename TraitsT, typename U, typename TraitsU>
bool
operator==(
		allocator<T, heap_allocator_policy<T>, TraitsT> const& left,
		allocator<U, heap_allocator_policy<U>, TraitsU> const& right)
{
	return true;
}

// Also implement inequality
template<typename T, typename TraitsT, typename U, typename TraitsU>
bool
operator!=(
		allocator<T, heap_allocator_policy<T>, TraitsT> const& left,
		allocator<U, heap_allocator_policy<U>, TraitsU> const& right)
{
	return !(left == right);
}

}    // namespace util
}    // namespace logicmill

#endif    // LOGICMILL_UTIL_ALLOCATOR_H