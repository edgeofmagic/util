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

#ifndef LOGICMILL_BUFFER_H
#define LOGICMILL_BUFFER_H

#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <logicmill/types.h>
#include <logicmill/util/macros.h>
#include <logicmill/util/shared_ptr.h>
#include <system_error>

#define USE_STD_SHARED_PTR 0

#if (USE_STD_SHARED_PTR)
#define SHARED_PTR_TYPE std::shared_ptr
#define MAKE_SHARED std::make_shared
#else
#define SHARED_PTR_TYPE util::shared_ptr
#define MAKE_SHARED util::make_shared
#endif

#ifndef NDEBUG

#define ASSERT_MUTABLE_BUFFER_INVARIANTS(_buf_)                                                                        \
	{                                                                                                                  \
		assert((_buf_).m_region != nullptr);                                                                           \
		assert((_buf_).m_data == (_buf_).m_region->data());                                                            \
		assert((_buf_).m_size <= (_buf_).m_region->capacity());                                                        \
		assert((_buf_).m_capacity == (_buf_).m_region->capacity());                                                    \
	}                                                                                                                  \
	/**/

#if 0

#define ASSERT_MUTABLE_BUFFER_INVARIANTS(_buf_)                                                                        \
	{                                                                                                                  \
		if ((_buf_).m_alloc != nullptr)                                                                                \
		{                                                                                                              \
			assert((_buf_).m_data == (_buf_).m_alloc->data());                                                         \
			assert((_buf_).m_size <= (_buf_).m_alloc->capacity());                                                     \
			assert((_buf_).m_capacity == (_buf_).m_alloc->capacity());                                                 \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			assert((_buf_).m_data == nullptr);                                                                         \
			assert((_buf_).m_capacity == 0);                                                                           \
			assert((_buf_).m_size == 0);                                                                               \
		}                                                                                                              \
	}                                                                                                                  \
	/**/

#endif

#define ASSERT_CONST_BUFFER_INVARIANTS(_buf_)                                                                          \
	{                                                                                                                  \
		assert((_buf_).m_region != nullptr);                                                                           \
		assert((_buf_).m_data >= (_buf_).m_region->data());                                                            \
		assert((_buf_).m_data <= ((_buf_).m_region->data() + (_buf_).m_region->capacity()));                           \
		assert(((_buf_).m_data + (_buf_).m_size) <= ((_buf_).m_region->data() + (_buf_).m_region->capacity()));        \
	}                                                                                                                  \
	/**/

#define ASSERT_SHARED_BUFFER_INVARIANTS(_buf_)                                                                         \
	ASSERT_CONST_BUFFER_INVARIANTS(_buf_)                                                                              \
	/**/

#else

#define ASSERT_MUTABLE_BUFFER_INVARIANTS(_buf_)

#define ASSERT_CONST_BUFFER_INVARIANTS(_buf_)

#define ASSERT_SHARED_BUFFER_INVARIANTS(_buf_)

#endif

namespace logicmill
{

template<class T>
struct null_delete
{
	void
	operator()(T*)
	{}
};

template<class T>
struct null_delete<T[]>
{
	void
	operator()(T*)
	{}
};

class const_buffer;

/** \brief Represents and manages a contiguous region of memory.
 * 
 * An instance of buffer represents and manages a contiguous region of memory, as a 2-tuple consisting of 
 * a pointer to the beginning of the region and the size of the region in bytes. These
 * are accessible through the member functions data() and size(). The buffer class is abstract; instances 
 * cannot be constructed directly. It serves are a base class for two concrete derived classes&mdash;mutable_buffer
 * and const_buffer, and declares constructs that are common to both derived classes.
 */
class buffer
{
public:
	/** \brief The type used to represent a checksum value calculated for
	 * the memory region.
	 */
	using checksum_type = std::uint32_t;

protected:
	class region
	{
	public:
		region(byte_type* data, size_type capacity) : m_data{data}, m_capacity{capacity} {}

		virtual ~region() {}

	protected:
		virtual byte_type*
		alloc(size_type capacity)
				= 0;

		virtual void
		dealloc(byte_type* p, size_type capacity)
				= 0;

	public:
		virtual bool
		can_allocate() const = 0;

		byte_type*
		allocate(size_type capacity, std::error_code& err)
		{
			err.clear();
			byte_type* p{nullptr};
			if (!can_allocate())
			{
				err = make_error_code(std::errc::operation_not_supported);
				goto exit;
			}
			p = alloc(capacity);
			if (!p)
			{
				err = make_error_code(std::errc::no_buffer_space);
				goto exit;
			}
			m_data     = p;
			m_capacity = capacity;

		exit:
			return m_data;
		}

		byte_type*
		allocate(size_type capacity)
		{
			std::error_code err;
			auto            result = allocate(capacity, err);
			if (err)
			{
				throw std::system_error{err};
			}
			return result;
		}

		void
		deallocate()
		{
			dealloc(m_data, m_capacity);
			m_data     = nullptr;
			m_capacity = 0;
		}

		byte_type*
		reallocate(size_type new_capacity)
		{
			return reallocate(m_capacity, new_capacity);
		}

		byte_type*
		reallocate(size_type new_capacity, std::error_code& err)
		{
			return reallocate(m_capacity, new_capacity, err);
		}


		byte_type*
		reallocate(size_type current_size, size_type new_capacity)
		{
			std::error_code err;
			auto            result = reallocate(current_size, new_capacity, err);
			if (err)
			{
				throw std::system_error{err};
			}
			return result;
		}


		byte_type*
		reallocate(size_type current_size, size_type new_capacity, std::error_code& err)
		{
			err.clear();
			if (!can_allocate())
			{
				err = make_error_code(std::errc::operation_not_supported);
				goto exit;
			}

			if (!m_data && new_capacity > 0)
			{
				auto p = alloc(new_capacity);
				if (!p)
				{
					err = make_error_code(std::errc::no_buffer_space);
					goto exit;
				}
				m_data     = p;
				m_capacity = new_capacity;
				// m_data = m_broker->allocate( new_capacity );
				// m_capacity = new_capacity;
			}
			else if (new_capacity > m_capacity)
			{
				current_size = (current_size > m_capacity) ? m_capacity : current_size;
				auto p       = alloc(new_capacity);
				if (!p)
				{
					err = make_error_code(std::errc::no_buffer_space);
					goto exit;
				}
				::memcpy(p, m_data, current_size);
				dealloc(m_data, m_capacity);
				m_data     = p;
				m_capacity = new_capacity;
			}
		exit:
			return m_data;
		}

		byte_type*
		data()
		{
			return m_data;
		}

		size_type
		capacity()
		{
			return m_capacity;
		}

	protected:
		byte_type* m_data;
		size_type  m_capacity;
	};

	template<class Del>
	class del_region : protected Del, public region
	{
	public:
		using deleter_base = Del;
		using deleter_type = Del;

		template<class _Del>
		del_region(byte_type* data, size_type capacity, _Del&& del)
			: deleter_base{std::forward<_Del>(del)}, region{data, capacity}
		{}

		del_region(byte_type* data, size_type capacity) : deleter_base{deleter_type{}}, region{data, capacity} {}

		virtual ~del_region()
		{
			dealloc(m_data, m_capacity);
			m_data = nullptr;
		}

		virtual bool
		can_allocate() const override
		{
			return false;
		}

	protected:
		virtual byte_type* alloc(size_type) override
		{
			return nullptr;
		}

		virtual void
		dealloc(byte_type* p, size_type) override
		{
			deleter_base::operator()(p);
		}
	};


	template<class Alloc = std::allocator<byte_type>>
	class alloc_region : protected Alloc, public region
	{
	public:
		using sptr = SHARED_PTR_TYPE<alloc_region>;
		using uptr = std::unique_ptr<alloc_region>;

		using allocator_type = Alloc;
		using allocator_base = Alloc;


		alloc_region() : allocator_base{allocator_type{}}, region{nullptr, 0} {}

		template<
				class _Alloc,
				class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
		alloc_region(_Alloc&& alloc) : allocator_base{std::forward<_Alloc>(alloc)}, region{nullptr, 0}
		{}

		template<
				class _Alloc,
				class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
		alloc_region(size_type capacity, _Alloc&& alloc)
			: allocator_base{std::forward<_Alloc>(alloc)},
			  region{((capacity > 0) ? allocator_base::allocate(capacity) : nullptr), capacity}
		{}

		alloc_region(size_type capacity)
			: allocator_base{allocator_type{}},
			  region{((capacity > 0) ? allocator_base::allocate(capacity) : nullptr), capacity}
		{}

		template<
				class _Alloc,
				class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
		alloc_region(const byte_type* data, size_type size, _Alloc&& alloc)
			: allocator_base{std::forward<_Alloc>(alloc)}, region{allocator_base::allocate(size), size}
		{
			if (m_data && data && size > 0)
			{
				::memcpy(m_data, data, size);
			}
		}

		alloc_region(const byte_type* data, size_type size)
			: allocator_base{allocator_type{}}, region{allocator_base::allocate(size), size}
		{
			if (m_data && data && size > 0)
			{
				::memcpy(m_data, data, size);
			}
		}

		template<
				class _Alloc,
				class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
		alloc_region(byte_type* data, size_type capacity, _Alloc&& alloc)
			: allocator_base{std::forward<_Alloc>(alloc)}, region{data, capacity}
		{}

		alloc_region(byte_type* data, size_type capacity) : allocator_base{allocator_type{}}, region{data, capacity} {}

		virtual ~alloc_region()
		{
			dealloc(m_data, m_capacity);
			m_data     = nullptr;
			m_capacity = 0;
		}

		virtual bool
		can_allocate() const override
		{
			return true;
		}

	protected:
		virtual byte_type*
		alloc(size_type capacity) override
		{
			return allocator_base::allocate(capacity);
		}

		virtual void
		dealloc(byte_type* p, size_type capacity) override
		{
			allocator_base::deallocate(p, capacity);
		}
	};

	class region_factory
	{
	public:
		virtual std::unique_ptr<region>
		create(size_type capacity) = 0;
	};

	template<class Alloc = std::allocator<byte_type>, class Enable = void>
	class alloc_region_factory;

	template<class Alloc>
	class alloc_region_factory<
			Alloc,
			typename std::enable_if_t<std::is_same<typename Alloc::pointer, byte_type*>::value>>
		: public region_factory, public Alloc
	{
	public:
		template<class _Alloc>
		alloc_region_factory(_Alloc&& alloc) : Alloc{std::forward<_Alloc>(alloc)}
		{}

		alloc_region_factory() : Alloc{} {}

		virtual std::unique_ptr<region>
		create(size_type capacity) override
		{
			return std::make_unique<alloc_region<Alloc>>(capacity, static_cast<Alloc>(*this));
		}
	};

public:
	friend class const_buffer;

	~buffer()
	{
		m_data = nullptr;
		m_size = 0;
	}

	/** \brief Pointer to the first byte in the memory region.
	 * 
	 * \return a const pointer to the first byte in the region
	 */
	const byte_type*
	data() const
	{
		return m_data;
	}

	/** \brief Size of the memory region.
	 * 
	 * \return the size of the memory region in bytes
	 */
	size_type
	size() const
	{
		return m_size;
	}

	/** \brief Test for equality with another buffer.
	 * 
	 * Two buffers are considered equal if they are of equal size, and the byte sequences from data() .. data() + size() - 1 in 
	 * both buffers have equal values.
	 * 
	 * \param rhs the buffer to be compared with this instance
	 * \return true if the buffers are equal, false otherwise
	 */
	bool
	operator==(buffer const& rhs) const
	{
		bool result = true;

		if (this != &rhs)
		{
			if (m_size != rhs.m_size)
			{
				result = false;
			}
			else
			{
				if (m_size != 0)
				{
					result = (std::memcmp(m_data, rhs.m_data, m_size) == 0);
				}
			}
		}
		return result;
	}

	bool
	operator!=(buffer const& rhs) const
	{
		return !(*this == rhs);
	}

	/** \brief Construct a std::string value from the contents of this instance's memory region.
	 * 
	 * The contents of the memory region from data() to data() + size() - 1 are copied into the
	 * resulting string value. The byte values from the buffer are copied exactly, with no 
	 * interpretation or transformation.
	 * 
	 * \return a std::string value constructed from the buffer contents.
	 */
	std::string
	to_string() const
	{
		std::string result;
		if (m_size > 0)
		{
			result = std::string(reinterpret_cast<const char*>(m_data), m_size);
		}
		return result;
	}

	/** \brief Construct a std::string_view value that refers to the contents of this buffer's memory region.
	 * 
	 * The resulting std::string_view is equivalent to having been constructed as follows:
	 * \code
	 * std::string_view result( reinterpret_cast< char* >( data() ), size() );
	 * \endcode 
	 * \return a std::string_view value referring to this buffer's memory region
	 */
	std::string_view
	as_string() const
	{
		return std::string_view(reinterpret_cast<char*>(m_data), m_size);
	}

	/** Test this buffer for size() == 0
	 * 
	 * \return true if size() == 0, false otherwise
	 */
	bool
	empty() const
	{
		return m_size == 0;
	}

	checksum_type
	checksum() const
	{
		return checksum(0, size());
	}

	checksum_type
	checksum(size_type length) const
	{
		return checksum(0, std::min(length, size()));
	}

	checksum_type
	checksum(size_type offset, size_type length) const;

	void
	dump(std::ostream& os) const;

protected:
	buffer() : m_data{nullptr}, m_size{0} {}

	buffer(byte_type* data, size_type size) : m_data{data}, m_size{size} {}

	byte_type* m_data;
	size_type  m_size;
};

/** \brief Represents and manages a contiguous region of memory that can be modified. 
 * 
 * The region of memory managed by an instance of mutable_buffer is represented as a 3-tuple
 * consisting of a pointer, a size in bytes, and a capacity in bytes. The conceptual distinction between
 * size and capacity is important. The capacity of a mutable_buffer instance is the number of contiguous bytes
 * in the memory region that are available for use. The size is an indication of the number of bytes (as 
 * an offset from the buffer start) that have meaningful contents, from the perspective of the buffer owner.
 * The capacity of a mutable_buffer instance is initially set on construction, and may potentially be changed by calling
 * the expand() member function. The value of size is set by calling the size( size_type ) mutator member function. 
 * The value of size is meaningful at certain points in the lifecycle of the owning buffer:
 * 
 * <ul>
 * <li>
 * When a mutable_buffer is constructed by move or assigned by move from another mutable_buffer, the size is preserved.
 * </li>
 * <li>
 * When an instance of const_buffer is contructed by move or copy from a mutable_buffer, the size of the const_buffer is
 * set to the value of the mutable_buffer's size (not capacity).
 * </li>
 * 
 * <li>
 * When a mutable_buffer is externalized as a string or string_view (to_string() or as_string()) the size of the mutable_buffer
 * determines the length of the region used to create the string (and the size of the resulting string).
 * </li>
 * 
 * <li>
 * When a mutable_buffer is compared for equality with a mutable_buffer or a const_buffer, size determines the number of bytes compared.
 * </li>
 * 
 * <li>
 * When a mutable_buffer is expanded, size determines the number of bytes whose values are guaranteed to be preserved from the original 
 * memory region to the resulting region.
 * </li>
 * </ul>
 * 
 * As a general pattern, the code that allocates a mutable_buffer and deposits values into its memory region should be 
 * thought of as the <i>producer</i>. The producer is responsible for determining the appropriate value of size (based on how much
 * of the buffer's capacity was modified), and setting the value of size before passing the buffer to a consumer, or converting
 * it to a const_buffer for consumption.
 * 
 */
class mutable_buffer : public buffer
{
protected:
	std::unique_ptr<region> m_region;
	size_type               m_capacity;

public:
	using buffer::data;
	using buffer::size;

	using default_alloc = std::allocator<byte_type>;
	using default_del   = std::default_delete<byte_type>;

	friend class const_buffer;
	friend class shared_buffer;

	/** \brief Default constructor.
	 * 
	 * The constructed instance owns a copy of the default memory_broker, and no allocated memory region, such that
	 * data() == nullptr, capacity() == 0, size() == 0.
	 */
	mutable_buffer() : m_region{std::make_unique<alloc_region<default_alloc>>()}, m_capacity{0}
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	/** \brief Construct with the specified memory_broker.
	 * 
	 * \param broker a shared pointer to the memory_broker to be used by the allocation
	 */
	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	mutable_buffer(_Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(std::forward<_Alloc>(alloc))}, m_capacity{0}
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	mutable_buffer(size_type capacity)
		: m_region{std::make_unique<alloc_region<default_alloc>>(capacity)}, m_capacity{capacity}
	{
		m_data = m_region->data();
		m_size = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	mutable_buffer(size_type capacity, _Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(capacity, std::forward<_Alloc>(alloc))}, m_capacity{capacity}
	{
		m_data = m_region->data();
		m_size = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	/** \brief Construct with the provided memory region and specified deallocator callable object.
	 * 
	 * This constructor form is useful in cases where memory regions are imported from other
	 * facilities that manage their own buffers. If a deallocator function is provided, it will be
	 * invoked when the mutable_buffer instance is destroyed. The constructed buffer, in effect, adopts
	 * the provided memory region (specified by arguments of the data and cap parameters). In such cases, 
	 * the specified deallocator 
	 * would typically be used to notify the facility that originally allocated the memory region that 
	 * the memory region is no longer referenced by the mutable_buffer, and may be deallocated.
	 * The data() and capacity() of the resulting instance reflect the arguments of the data and cap
	 * parameters; size() will be zero.
	 * 
	 * \param data a pointer to the memory region to be referenced by the constucted instance
	 * \param cap the size of the region
	 * \param dealloc_f a callable object convertible to type deallocator, to be invoked when this instance is destroyed
	 */
	template<class _Del>
	mutable_buffer(void* data, size_type capacity, _Del&& del)
		: m_region{std::make_unique<del_region<_Del>>(
				  reinterpret_cast<byte_type*>(data),
				  capacity,
				  std::forward<_Del>(del))},
		  m_capacity{capacity}
	{
		m_data = m_region->data();
		m_size = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

#if 1

	mutable_buffer(const void* data, size_type capacity)    // TODO: mostly here for testing, consider removing
		: m_region{std::make_unique<alloc_region<default_alloc>>(reinterpret_cast<const byte_type*>(data), capacity)},
		  m_capacity{capacity}
	{
		m_data = m_region->data();
		m_size = capacity;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	// mutable_buffer( const void* data, size_type capacity ) // TODO: mostly here for testing, consider removing
	// :
	// m_region{ std::make_unique< allocation >( reinterpret_cast< const byte_type* >( data ), capacity ) },
	// m_capacity{ capacity }
	// {
	// 	m_data = m_region->data();
	// 	m_size = capacity;
	// }

	mutable_buffer(std::string const& s)
		: m_region{std::make_unique<alloc_region<default_alloc>>(
				  reinterpret_cast<const byte_type*>(s.data()),
				  s.size())},
		  m_capacity{s.size()}
	{
		m_data = m_region->data();
		m_size = s.size();
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	// mutable_buffer( std::string const& s )
	// :
	// m_region{ std::make_unique< allocation >( reinterpret_cast< const byte_type* >( s.data() ), s.size() ) },
	// m_capacity{ s.size() }
	// {
	// 	m_data = m_region->data();
	// 	m_size = s.size();
	// }

#endif

	mutable_buffer(mutable_buffer const&) = delete;

	mutable_buffer(mutable_buffer&& rhs) : m_region{std::move(rhs.m_region)}, m_capacity{rhs.m_capacity}
	{
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.pclear();
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

private:
	void
	ctor_body(byte_type* data, size_type size, position_type offset, size_type length, std::error_code& err)
	{
		if (offset + length > size)
		{
			throw std::system_error{make_error_code(std::errc::invalid_argument)};
		}
		m_data     = data + offset;
		m_size     = length;
		m_capacity = (m_region->data() + m_region->capacity()) - m_data;
	}

	void
	pclear()
	{
		m_data     = nullptr;
		m_size     = 0;
		m_capacity = 0;
	}

public:
	mutable_buffer(mutable_buffer&& rhs, size_type offset, size_type length) : m_region{std::move(rhs.m_region)}
	{
		std::error_code err;
		ctor_body(rhs.m_data, rhs.m_size, offset, length, err);
		rhs.pclear();
		if (err)
		{
			throw std::system_error{err};
		}
	}

	mutable_buffer(mutable_buffer&& rhs, size_type offset, size_type length, std::error_code& err)
		: m_region{std::move(rhs.m_region)}
	{
		ctor_body(rhs.m_data, rhs.m_size, offset, length, err);
		rhs.pclear();
	}

	mutable_buffer(const_buffer&& rhs, size_type offset, size_type length);

	mutable_buffer(const_buffer&& rhs, size_type offset, size_type length, std::error_code& err);

	mutable_buffer(const_buffer&& cbuf);

	mutable_buffer&
	operator=(mutable_buffer&& rhs)
	{
		// don't assert invariants, this (lhs of assign) may be victim of a move
		// ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );

		ASSERT_MUTABLE_BUFFER_INVARIANTS(rhs);

		m_region   = std::move(rhs.m_region);
		m_data     = rhs.m_data;
		m_size     = rhs.m_size;
		m_capacity = rhs.m_capacity;

		rhs.m_data     = nullptr;
		rhs.m_size     = 0;
		rhs.m_capacity = 0;

		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);

		return *this;
	}

	mutable_buffer&
	operator=(mutable_buffer const&)
			= delete;

	byte_type*
	data()
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		return m_data;
	}

	void
	size(size_type new_size, std::error_code& err)
	{
		err.clear();

		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		if (new_size > m_capacity)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}

		m_size = new_size;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	exit:
		return;
	}

	void
	size(size_type new_size)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		if (new_size > m_capacity)
		{
			throw std::system_error{make_error_code(std::errc::invalid_argument)};
		}
		m_size = new_size;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	size_type
	capacity() const
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		return m_capacity;
	}

	bool
	is_expandable() const
	{
		return m_region->can_allocate();
	}

	mutable_buffer&
	expand(size_type new_cap, std::error_code& err)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);

		err.clear();

		if (new_cap > m_capacity)
		{
			if (!m_data)
			{
				assert(m_capacity == 0);
				assert(m_size == 0);
				assert(m_region->data() == nullptr);
				assert(m_region->capacity() == 0);
				if (m_region->can_allocate())
				{
					m_data = m_region->allocate(new_cap, err);
					if (err)
						goto exit;

					m_capacity = m_region->capacity();
				}
				else
				{
					err = make_error_code(std::errc::operation_not_supported);
					goto exit;
				}
			}
			else if (m_region->can_allocate())
			{
				assert(m_capacity > 0);
				m_data = m_region->reallocate(m_size, new_cap, err);
				if (err)
					goto exit;

				m_capacity = m_region->capacity();
			}
			else
			{
				err = make_error_code(std::errc::operation_not_supported);
				goto exit;
			}
			m_data     = m_region->data();
			m_capacity = m_region->capacity();
		}

	exit:
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		return *this;
	}

	mutable_buffer&
	expand(size_type new_capacity)
	{
		std::error_code err;
		expand(new_capacity, err);
		if (err)
		{
			throw std::system_error{err};
		}

	exit:
		return *this;
	}

	mutable_buffer&
	fill(position_type offset, size_type length, byte_type value, std::error_code& err)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		if (offset + length > m_capacity)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}

		if (length > 0)
		{
			::memset(m_data, value, length);
		}

	exit:
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		return *this;
	}

	mutable_buffer&
	fill(position_type offset, size_type length, byte_type value)
	{
		std::error_code err;
		fill(offset, length, value, err);
		if (err)
		{
			throw std::system_error{err};
		}
		return *this;
	}

	mutable_buffer&
	fill(byte_type value)
	{
		return fill(0, m_capacity, value);
	}

	mutable_buffer&
	put(position_type offset, byte_type value)
	{
		std::error_code err;
		put(offset, value, err);
		if (err)
		{
			throw std::system_error{err};
		}
		return *this;
	}

	mutable_buffer&
	put(position_type offset, byte_type value, std::error_code& err)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		err.clear();

		if (offset >= m_capacity)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}

		*(m_data + offset) = value;

	exit:
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		return *this;
	}

	mutable_buffer&
	putn(position_type offset, const void* src, size_type length)
	{
		std::error_code err;
		putn(offset, src, length, err);
		if (err)
		{
			throw std::system_error{err};
		}
		return *this;
	}

	mutable_buffer&
	putn(position_type offset, const void* src, size_type length, std::error_code& err)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		err.clear();

		if (length > 0)
		{
			if (!src || offset + length > m_capacity)
			{
				err = make_error_code(std::errc::invalid_argument);
				goto exit;
			}
			::memcpy(m_data + offset, src, length);
		}

	exit:
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
		return *this;
	}

	mutable_buffer&
	putn(position_type offset, std::string const& s, std::error_code& err)
	{
		return putn(offset, s.data(), s.size(), err);
	}

	mutable_buffer&
	putn(position_type offset, std::string const& s)
	{
		return putn(offset, s.data(), s.size());
	}

	mutable_buffer&
	putn(position_type offset, const char* s, std::error_code& err)
	{
		return putn(offset, s, ::strlen(s), err);
	}

	mutable_buffer&
	putn(position_type offset, const char* s)
	{
		return putn(offset, s, ::strlen(s));
	}
};

class mutable_buffer_factory
{
public:
	virtual ~mutable_buffer_factory() {}
	virtual mutable_buffer
	create(size_type size)
			= 0;
};

template<class Alloc = std::allocator<byte_type>, class Enable = void>
class mutable_buffer_alloc_factory;

template<class Alloc>
class mutable_buffer_alloc_factory<
		Alloc,
		typename std::enable_if_t<std::is_same<typename Alloc::pointer, byte_type*>::value>>
	: public Alloc, public mutable_buffer_factory
{
public:
	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	mutable_buffer_alloc_factory(_Alloc&& alloc) : Alloc{std::forward<_Alloc>(alloc)}
	{}

	mutable_buffer_alloc_factory() : Alloc{} {}

	virtual mutable_buffer
	create(size_type size) override
	{
		return mutable_buffer{size, static_cast<Alloc>(*this)};
	}
};

class const_buffer : public buffer
{
protected:
	std::unique_ptr<region> m_region;

public:
	using default_alloc = std::allocator<byte_type>;
	using default_del   = std::default_delete<byte_type>;

	friend class mutable_buffer;
	friend class shared_buffer;

	const_buffer() : m_region{std::make_unique<alloc_region<default_alloc>>()}
	{
		m_data = nullptr;
		m_size = 0;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

	// const_buffer( const void* data, size_type size )
	// :
	// m_alloc{ std::make_unique< allocation >( reinterpret_cast< const byte_type* >( data ), size ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = size;
	// }

	template<class _Del>
	const_buffer(void* data, size_type size, _Del&& del)
		: m_region{
				  std::make_unique<del_region<_Del>>(reinterpret_cast<byte_type*>(data), size, std::forward<_Del>(del))}
	{
		m_data = reinterpret_cast<byte_type*>(data);
		m_size = size;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

	const_buffer(const void* data, size_type size)
		: m_region{std::make_unique<alloc_region<default_alloc>>(reinterpret_cast<const byte_type*>(data), size)}
	{
		m_data = m_region->data();
		m_size = size;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	const_buffer(const void* data, size_type size, _Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(
				  reinterpret_cast<const byte_type*>(data),
				  size,
				  std::forward<_Alloc>(alloc))}
	{
		m_data = m_region->data();
		m_size = size;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	const_buffer(buffer const& rhs, _Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(rhs.data(), rhs.size(), std::forward<_Alloc>(alloc))}
	{
		m_data = m_region->data();
		m_size = rhs.size();
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		// ASSERT_CONST_BUFFER_INVARIANTS( rhs );
	}

	const_buffer(buffer const& rhs) : m_region{std::make_unique<alloc_region<default_alloc>>(rhs.data(), rhs.size())}
	{
		m_data = m_region->data();
		m_size = rhs.size();
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		// ASSERT_CONST_BUFFER_INVARIANTS( rhs );
	}

	const_buffer(const_buffer const& rhs)
		: m_region{std::make_unique<alloc_region<default_alloc>>(rhs.data(), rhs.size())}
	{
		m_data = m_region->data();
		m_size = rhs.size();
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		ASSERT_CONST_BUFFER_INVARIANTS(rhs);
	}

	// const_buffer( mutable_buffer const& rhs, memory_broker::ptr broker = buffer::default_broker::get() )
	// :
	// m_alloc{ std::make_unique< allocation >( rhs.m_data, rhs.m_size, broker ) }
	// {
	// 	ASSERT_MUTABLE_BUFFER_INVARIANTS( rhs );
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.m_size;
	// 	ASSERT_CONST_BUFFER_INVARIANTS( *this );
	// }

	// const_buffer( const_buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	// :
	// m_alloc{ std::make_unique< allocation >( rhs.m_data, rhs.m_size, broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.m_size;
	// 	// ASSERT_BUFFER_INVARIANTS( *this );
	// 	// ASSERT_BUFFER_INVARIANTS( rhs );
	// }

	const_buffer(const_buffer&& rhs) : m_region{std::move(rhs.m_region)}
	{
		m_data     = rhs.m_data;
		m_size     = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

	// const_buffer( mutable_buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	// :
	// m_alloc{ MAKE_SHARED< allocation >( rhs.data(), rhs.size(), broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.size();
	// }

	const_buffer(mutable_buffer&& rhs) : m_region{std::move(rhs.m_region)}
	{
		m_data         = rhs.m_data;
		m_size         = rhs.m_size;
		rhs.m_data     = nullptr;
		rhs.m_size     = 0;
		rhs.m_capacity = 0;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

private:
	void
	ctor_body(byte_type* data, size_type size, position_type offset, size_type length, std::error_code& err)
	{
		err.clear();
		if (offset + length > size)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}

		m_data = m_region->allocate(length, err);
		if (err)
			goto exit;

		::memcpy(m_data, data + offset, length);
		m_size = length;

		ASSERT_CONST_BUFFER_INVARIANTS(*this);

	exit:
		return;
	}

public:
	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	const_buffer(buffer const& rhs, position_type offset, size_type length, _Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(std::forward<_Alloc>(alloc))}
	{
		std::error_code err;
		ctor_body(rhs.m_data, rhs.m_size, offset, length, err);
		if (err)
		{
			throw std::system_error{err};
		}
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	const_buffer(buffer const& rhs, position_type offset, size_type length, _Alloc&& alloc, std::error_code& err)
		: m_region{std::make_unique<alloc_region<_Alloc>>(std::forward<_Alloc>(alloc))}
	{
		ctor_body(rhs.m_data, rhs.m_size, offset, length, err);
	}

	const_buffer(buffer const& rhs, position_type offset, size_type length)
		: m_region{std::make_unique<alloc_region<default_alloc>>()}
	{
		std::error_code err;
		ctor_body(rhs.m_data, rhs.m_size, offset, length, err);
		if (err)
		{
			throw std::system_error{err};
		}
	}

	const_buffer(buffer const& rhs, position_type offset, size_type length, std::error_code& err)
		: m_region{std::make_unique<alloc_region<default_alloc>>()}
	{
		ctor_body(rhs.m_data, rhs.m_size, offset, length, err);
	}


	const_buffer(const_buffer&& rhs, size_type offset, size_type length) : m_region{std::move(rhs.m_region)}
	{
		if (offset + length > rhs.m_size)
		{
			throw std::system_error{make_error_code(std::errc::invalid_argument)};
		}
		m_data     = rhs.m_data + offset;
		m_size     = length;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
	}

	const_buffer(mutable_buffer&& rhs, size_type offset, size_type length, std::error_code& err)
		: m_region{std::move(rhs.m_region)}
	{
		if (offset + length > rhs.m_size)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}
		m_data     = rhs.m_data + offset;
		m_size     = length;
		rhs.m_data = nullptr;
		rhs.m_size = 0;

	exit:
		return;
	}

	// const_buffer( buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	// :
	// m_alloc{ std::make_unique< allocation >( rhs.m_data, rhs.m_size, broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.m_size;
	// }

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	const_buffer
	slice(position_type offset, size_type length, _Alloc&& alloc) const
	{
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		return const_buffer{*this, offset, length, std::forward<_Alloc>(alloc)};
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	const_buffer
	slice(position_type offset, size_type length, _Alloc&& alloc, std::error_code& err) const
	{
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		err.clear();
		return const_buffer{*this, offset, length, std::forward<_Alloc>(alloc), err};
	}

	const_buffer
	slice(position_type offset, size_type length) const
	{
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		return const_buffer{*this, offset, length};
	}

	const_buffer
	slice(position_type offset, size_type length, std::error_code& err) const
	{
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		err.clear();
		return const_buffer{*this, offset, length, err};
	}

	const_buffer&
	operator=(const_buffer const& rhs)
	{
		ASSERT_CONST_BUFFER_INVARIANTS(rhs);
		m_region = std::make_unique<alloc_region<default_alloc>>(rhs.data(), rhs.size());
		m_data   = m_region->data();
		m_size   = rhs.size();
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		return *this;
	}

	const_buffer&
	operator=(mutable_buffer const& rhs)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(rhs);
		m_region = std::make_unique<alloc_region<default_alloc>>(rhs.buffer::data(), rhs.size());    // force const
		m_data   = m_region->data();
		m_size   = rhs.size();
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		return *this;
	}

	const_buffer&
	operator=(const_buffer&& rhs)
	{
		ASSERT_CONST_BUFFER_INVARIANTS(rhs);
		m_region   = std::move(rhs.m_region);
		m_data     = rhs.m_data;
		m_size     = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		return *this;
	}

	// shared_buffer&
	// operator=( mutable_buffer const& rhs )
	// {
	// 	m_alloc = MAKE_SHARED< allocation >( rhs.data(), rhs.size() );
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.size();
	// 	// ASSERT_MUTABLE_BUFFER_INVARIANTS( buf );
	// 	return *this;
	// }

	const_buffer&
	operator=(mutable_buffer&& rhs)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(rhs);
		m_region       = std::move(rhs.m_region);
		m_data         = rhs.m_data;
		m_size         = rhs.m_size;
		rhs.m_data     = nullptr;
		rhs.m_size     = 0;
		rhs.m_capacity = 0;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
		return *this;
	}
};

class shared_buffer : public buffer
{
protected:
	SHARED_PTR_TYPE<region> m_region;

public:
	~shared_buffer()
	{
		// std::cout << "in shared_buffer dtor, region use count is " << m_region.use_count();
		// if (!m_region)
		// {
		// 	std::cout << "in shared_buffer dtor, region is null" << std::endl;
		// }
		// else
		// {
		// 	if (m_region->can_allocate())
		// 	{
		// 		std::cout << ", region is alloc_region" << std::endl;
		// 	}
		// 	else
		// 	{
		// 		std::cout << ", region is del_region" << std::endl;
		// 	}
		// }
	}

	using default_alloc = std::allocator<byte_type>;
	using default_del   = std::default_delete<byte_type>;

	// void announce()
	// {
	// 	std::cout << "in shared_buffer ctor, region use_count is " << m_region.use_count();
	// 	if (!m_region)
	// 	{
	// 		std::cout << ", region is null" << std::endl;
	// 	}
	// 	else
	// 	{
	// 		if (m_region->can_allocate())
	// 		{
	// 			std::cout << ", region is alloc_region" << std::endl;
	// 		}
	// 		else
	// 		{
	// 			std::cout << ", region is del_region" << std::endl;
	// 		}
	// 	}
	// }

	shared_buffer() : m_region{MAKE_SHARED<alloc_region<default_alloc>>()}
	{
		m_data = nullptr;
		m_size = 0;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	// shared_buffer( const void* data, size_type size )
	// :
	// m_alloc{ MAKE_SHARED< allocation >( reinterpret_cast< const byte_type* >( data ), size ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = size;
	// }

	template<class _Del>
	shared_buffer(void* data, size_type size, _Del&& del)
		: m_region{MAKE_SHARED<del_region<_Del>>(reinterpret_cast<byte_type*>(data), size, std::forward<_Del>(del))}
	{
		m_data = reinterpret_cast<byte_type*>(data);
		m_size = size;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}
	/*
	shared_buffer( void* data, size_type size)
	:
	m_region{ MAKE_SHARED< del_region<default_del> >( 
		reinterpret_cast< byte_type* >( data ), size ) }
	{
		m_data = reinterpret_cast< byte_type* >( data );
		m_size = size;
		announce();
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}
*/
	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	shared_buffer(const void* data, size_type size, _Alloc&& alloc)
		: m_region{MAKE_SHARED<alloc_region<_Alloc>>(
				  reinterpret_cast<const byte_type*>(data),
				  size,
				  std::forward<_Alloc>(alloc))}
	{
		m_data = m_region->data();
		m_size = size;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(const void* data, size_type size)
		: m_region{MAKE_SHARED<alloc_region<default_alloc>>(reinterpret_cast<const byte_type*>(data), size)}
	{
		m_data = m_region->data();
		m_size = size;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(shared_buffer const& rhs) : m_region{rhs.m_region}
	{
		ASSERT_CONST_BUFFER_INVARIANTS(rhs);
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(shared_buffer&& rhs) : m_region{std::move(rhs.m_region)}
	{
		m_data     = rhs.m_data;
		m_size     = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	shared_buffer(buffer const& rhs, _Alloc&& alloc)
		: m_region{MAKE_SHARED<alloc_region<_Alloc>>(rhs.data(), rhs.size(), std::forward<_Alloc>(alloc))}
	{
		m_data = m_region->data();
		m_size = rhs.size();
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(buffer const& rhs) : m_region{MAKE_SHARED<alloc_region<default_alloc>>(rhs.data(), rhs.size())}
	{
		m_data = m_region->data();
		m_size = rhs.size();
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	// shared_buffer( mutable_buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	// :
	// m_alloc{ MAKE_SHARED< allocation >( rhs.data(), rhs.size(), broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.size();
	// }

	// shared_buffer( const_buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	// :
	// m_alloc{ MAKE_SHARED< allocation >( rhs.data(), rhs.size(), broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.size();
	// }

	shared_buffer(mutable_buffer&& rhs) : m_region{std::move(rhs.m_region)}
	{
		m_data         = rhs.m_data;
		m_size         = rhs.m_size;
		rhs.m_data     = nullptr;
		rhs.m_size     = 0;
		rhs.m_capacity = 0;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(const_buffer&& rhs) : m_region{std::move(rhs.m_region)}
	{
		m_data     = rhs.m_data;
		m_size     = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(shared_buffer const& rhs, position_type offset, size_type length) : m_region{rhs.m_region}
	{
		ASSERT_SHARED_BUFFER_INVARIANTS(rhs);
		if (offset + length > rhs.m_size)
		{
			throw std::system_error{make_error_code(std::errc::invalid_argument)};
		}
		m_data = rhs.m_data + offset;
		m_size = length;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(shared_buffer const& rhs, position_type offset, size_type length, std::error_code& err)
		: m_region{rhs.m_region}
	{
		err.clear();
		ASSERT_SHARED_BUFFER_INVARIANTS(rhs);
		if (offset + length > rhs.m_size)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}
		m_data = rhs.m_data + offset;
		m_size = length;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	exit:
		return;
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	shared_buffer(buffer const& rhs, position_type offset, size_type length, _Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(std::forward<_Alloc>(alloc))}
	{
		if (offset + length > rhs.size())
		{
			throw std::system_error{make_error_code(std::errc::invalid_argument)};
		}
		m_data = m_region->allocate(length);
		::memcpy(m_data, rhs.data() + offset, length);
		m_size = length;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(buffer const& rhs, position_type offset, size_type length)
		: m_region{std::make_unique<alloc_region<default_alloc>>()}
	{
		if (offset + length > rhs.size())
		{
			throw std::system_error{make_error_code(std::errc::invalid_argument)};
		}
		m_data = m_region->allocate(length);
		::memcpy(m_data, rhs.data() + offset, length);
		m_size = length;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	shared_buffer(buffer const& rhs, position_type offset, size_type length, _Alloc&& alloc, std::error_code& err)
		: m_region{std::make_unique<alloc_region<_Alloc>>(std::forward<_Alloc>(alloc))}
	{
		err.clear();
		if (offset + length > rhs.size())
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}
		m_data = m_region->allocate(length, err);
		if (err)
			goto exit;
		::memcpy(m_data, rhs.data() + offset, length);
		m_size = length;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	exit:
		return;
	}

	shared_buffer(buffer const& rhs, position_type offset, size_type length, std::error_code& err)
		: m_region{std::make_unique<alloc_region<default_alloc>>()}
	{
		if (offset + length > rhs.size())
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}
		m_data = m_region->allocate(length, err);
		if (err)
			goto exit;
		::memcpy(m_data, rhs.data() + offset, length);
		m_size = length;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	exit:
		return;
	}

	shared_buffer(mutable_buffer&& rhs, position_type offset, size_type length, std::error_code& err)
		: m_region{std::move(rhs.m_region)}
	{
		err.clear();
		if (offset + length > rhs.m_size)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}
		m_data         = rhs.m_data + offset;
		m_size         = length;
		rhs.m_data     = nullptr;
		rhs.m_size     = 0;
		rhs.m_capacity = 0;

	exit:
		return;
	}

	shared_buffer(const_buffer&& rhs, position_type offset, size_type length, std::error_code& err)
		: m_region{std::move(rhs.m_region)}
	{
		err.clear();
		if (offset + length > rhs.m_size)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}
		m_data     = rhs.m_data + offset;
		m_size     = length;
		rhs.m_data = nullptr;
		rhs.m_size = 0;

	exit:
		return;
	}

	shared_buffer(shared_buffer&& rhs, position_type offset, size_type length, std::error_code& err)
		: m_region{std::move(rhs.m_region)}
	{
		err.clear();
		if (offset + length > rhs.m_size)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}
		m_data     = rhs.m_data + offset;
		m_size     = length;
		rhs.m_data = nullptr;
		rhs.m_size = 0;

	exit:
		return;
	}

	shared_buffer&
	pare(position_type offset, size_type length, std::error_code& err)
	{
		err.clear();
		if (offset + length > m_size)
		{
			err = make_error_code(std::errc::invalid_argument);
			goto exit;
		}
		m_data = m_data + offset;
		m_size = length;
	exit:
		return *this;
	}

	shared_buffer&
	pare(position_type offset, size_type length)
	{
		if (offset + length > m_size)
		{
			throw std::system_error{make_error_code(std::errc::invalid_argument)};
		}
		m_data = m_data + offset;
		m_size = length;
		return *this;
	}


	// shared_buffer( buffer const& rhs, memory_broker::ptr broker )
	// :
	// m_alloc{ MAKE_SHARED< allocation >( rhs.m_data, rhs.m_size, broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.m_size;
	// }

	shared_buffer
	slice(position_type offset, size_type length) const
	{
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		return shared_buffer{*this, offset, length};
	}

	shared_buffer
	slice(position_type offset, size_type length, std::error_code& err) const
	{
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		return shared_buffer{*this, offset, length, err};
	}

	shared_buffer&
	operator=(shared_buffer const& rhs)
	{
		ASSERT_SHARED_BUFFER_INVARIANTS(rhs);
		m_region = rhs.m_region;
		m_data   = rhs.m_data;
		m_size   = rhs.m_size;
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		return *this;
	}

	shared_buffer&
	operator=(shared_buffer&& rhs)
	{
		ASSERT_SHARED_BUFFER_INVARIANTS(rhs);
		m_region   = std::move(rhs.m_region);
		m_data     = rhs.m_data;
		m_size     = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		return *this;
	}

	shared_buffer&
	operator=(buffer const& rhs)
	{
		m_region = MAKE_SHARED<alloc_region<default_alloc>>(rhs.data(), rhs.size());
		m_data   = m_region->data();
		m_size   = rhs.size();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		return *this;
	}

	shared_buffer&
	operator=(mutable_buffer&& rhs)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(rhs);
		m_region       = std::move(rhs.m_region);
		m_data         = rhs.m_data;
		m_size         = rhs.m_size;
		rhs.m_data     = nullptr;
		rhs.m_size     = 0;
		rhs.m_capacity = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		return *this;
	}

	shared_buffer&
	operator=(const_buffer&& rhs)
	{
		ASSERT_CONST_BUFFER_INVARIANTS(rhs);
		m_region   = std::move(rhs.m_region);
		m_data     = rhs.m_data;
		m_size     = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		return *this;
	}

	std::size_t
	ref_count() const
	{
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		return m_region.use_count();
	}
};

class string_alias
{
public:
	string_alias() : m_buf{} {}

	string_alias(shared_buffer const& buf) : m_buf{buf} {}

	string_alias(shared_buffer const& buf, std::size_t offset, std::size_t size) : m_buf{buf.slice(offset, size)} {}

	string_alias(string_alias const& rhs) : m_buf{rhs.m_buf} {}

	string_alias(string_alias&& rhs) : m_buf{std::move(rhs.m_buf)} {}

	string_alias&
	operator=(string_alias const& rhs)
	{
		if (this != &rhs)
		{
			m_buf = rhs.m_buf;
		}
		return *this;
	}

	string_alias&
	operator=(string_alias&& rhs)
	{
		if (this != &rhs)
		{
			m_buf = std::move(rhs.m_buf);
		}
		return *this;
	}

	operator std::string_view() const noexcept
	{
		return std::string_view{reinterpret_cast<const char*>(m_buf.data()), m_buf.size()};
	}

	std::string_view
	view() const noexcept
	{
		return std::string_view{reinterpret_cast<const char*>(m_buf.data()), m_buf.size()};
	}

private:
	shared_buffer m_buf;
};

#if 1
// Ghetto streambuf to provide support for msgpack::packer and unpacker

class bufwriter
{
public:
	bufwriter(std::size_t size) : m_buf{size}, m_pos{0} {}

	void
	reset()
	{
		m_pos = 0;
	}

	std::size_t
	position() const
	{
		return m_pos;
	}

	void
	putn(const void* src, std::size_t n)
	{
		accommodate(n);
		m_buf.putn(m_pos, src, n);
		m_pos += n;
	}

	void
	put(std::uint8_t b)
	{
		accommodate(1);
		m_buf.put(m_pos, b);
		++m_pos;
	}

	std::uint8_t*
	accommodate(std::size_t n)
	{
		auto remaining = m_buf.capacity() - m_pos;
		if (n > remaining)
		{
			auto required       = m_pos + n;
			auto cushioned_size = (3 * required) / 2;
			m_buf.expand(cushioned_size);
		}
		return m_buf.data() + m_pos;
	}

	void
	advance(std::size_t n)
	{
		m_pos += n;
	}

	const_buffer
	get_buffer()
	{
		m_buf.size(m_pos);
		return const_buffer{m_buf};
	}

	void
	write(const char* src, std::size_t len)
	{
		putn(src, len);
	}

private:
	mutable_buffer m_buf;
	std::size_t    m_pos;
};

#endif

}    // namespace logicmill

#endif    // LOGICMILL_BUFFER_H
