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

#ifndef LOGICMILL_UTIL_BUFFER_H
#define LOGICMILL_UTIL_BUFFER_H

#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <logicmill/types.h>
#include <logicmill/util/buffer_traits.h>
#include <logicmill/util/macros.h>
#include <logicmill/util/shared_ptr.h>
#include <stdexcept>
#include <system_error>

#ifndef NDEBUG

#define ASSERT_MUTABLE_BUFFER_INVARIANTS(_buf_)                                                                        \
	{                                                                                                                  \
		if ((_buf_).m_region)                                                                                          \
		{                                                                                                              \
			assert((_buf_).m_region != nullptr);                                                                       \
			assert((_buf_).m_data == (_buf_).m_region->data());                                                        \
			assert((_buf_).m_size <= (_buf_).m_region->capacity());                                                    \
			assert((_buf_).m_capacity == (_buf_).m_region->capacity());                                                \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			assert((_buf_).m_data == nullptr);                                                                         \
			assert((_buf_).m_capacity == 0);                                                                           \
			assert((_buf_).m_size == 0);                                                                               \
		}                                                                                                              \
	}
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
namespace util
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

class buffer;
class mutable_buffer;
class const_buffer;
class shared_buffer;

template<class T>
struct is_buffer_type : public std::false_type
{};

template<>
struct is_buffer_type<buffer> : public std::true_type
{};

template<>
struct is_buffer_type<mutable_buffer> : public std::true_type
{};

template<>
struct is_buffer_type<const_buffer> : public std::true_type
{};

template<>
struct is_buffer_type<shared_buffer> : public std::true_type
{};

template<class Buffer>
inline size_type
total_size(std::deque<Buffer> const& bufs);


/** \brief Represents and manages a contiguous region of memory.
 * 
 * An instance of buffer represents and manages a contiguous region of memory, as a 2-tuple consisting of 
 * a pointer to the beginning of the region and the size of the region in bytes. These
 * are accessible through the member functions data() and size(). The buffer class is abstract; instances 
 * cannot be constructed directly. It serves are a base class for concrete derived classes&mdash;mutable_buffer,
 * const_buffer, and shared_buffer&mdash;and declares constructs that are common to all derived classes.
 * 
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
	protected:
		region(byte_type* data, size_type capacity) : m_data{data}, m_capacity{capacity} {}

	public:
		virtual ~region() {}

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

	class dynamic_region : public region
	{
	protected:
		dynamic_region(byte_type* data, size_type capacity) : region{data, capacity} {}

		virtual byte_type*
		alloc(size_type capacity)
				= 0;

		virtual void
		dealloc(byte_type* p, size_type capacity)
				= 0;

	public:
		byte_type*
		allocate(size_type capacity, std::error_code& err)
		{
			err.clear();
			byte_type* p{nullptr};
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
	};

	template<class Del>
	class del_region : protected Del, public region
	{
	public:
		using deleter_base = Del;
		using deleter_type = Del;

		del_region(byte_type* data, size_type capacity, deleter_type const& del)
			: deleter_base{del}, region{data, capacity}
		{}

		del_region(byte_type* data, size_type capacity, deleter_type&& del)
			: deleter_base{std::move(del)}, region{data, capacity}
		{}

		template<class = std::enable_if_t<std::is_default_constructible<deleter_type>::value>>
		del_region(byte_type* data, size_type capacity) : deleter_base{deleter_type{}}, region{data, capacity}
		{}

		virtual ~del_region()
		{
			destroy();
			m_data = nullptr;
		}

	protected:
		void
		destroy()
		{
			deleter_base::operator()(m_data);
		}
	};


	template<class Alloc = std::allocator<byte_type>>
	class alloc_region : protected Alloc, public dynamic_region
	{
	public:
		using sptr = util::shared_ptr<alloc_region>;
		using uptr = std::unique_ptr<alloc_region>;

		using allocator_type = Alloc;
		using allocator_base = Alloc;


		alloc_region() : allocator_base{allocator_type{}}, dynamic_region{nullptr, 0} {}

		template<
				class _Alloc,
				class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
		alloc_region(_Alloc&& alloc) : allocator_base{std::forward<_Alloc>(alloc)}, dynamic_region{nullptr, 0}
		{}

		template<
				class _Alloc,
				class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
		alloc_region(size_type capacity, _Alloc&& alloc)
			: allocator_base{std::forward<_Alloc>(alloc)},
			  dynamic_region{((capacity > 0) ? allocator_base::allocate(capacity) : nullptr), capacity}
		{}

		alloc_region(size_type capacity)
			: allocator_base{allocator_type{}},
			  dynamic_region{((capacity > 0) ? allocator_base::allocate(capacity) : nullptr), capacity}
		{}

		template<
				class _Alloc,
				class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
		alloc_region(const byte_type* data, size_type size, _Alloc&& alloc)
			: allocator_base{std::forward<_Alloc>(alloc)}, dynamic_region{allocator_base::allocate(size), size}
		{
			if (m_data && data && size > 0)
			{
				::memcpy(m_data, data, size);
			}
		}

		alloc_region(const byte_type* data, size_type size)
			: allocator_base{allocator_type{}}, dynamic_region{allocator_base::allocate(size), size}
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
			: allocator_base{std::forward<_Alloc>(alloc)}, dynamic_region{data, capacity}
		{}

		alloc_region(byte_type* data, size_type capacity)
			: allocator_base{allocator_type{}}, dynamic_region{data, capacity}
		{}

		virtual ~alloc_region()
		{
			dealloc(m_data, m_capacity);
			m_data     = nullptr;
			m_capacity = 0;
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


	template<size_type Size>
	class fixed_region : public region
	{
	public:
		using sptr = util::shared_ptr<fixed_region>;
		using uptr = std::unique_ptr<fixed_region>;

		fixed_region() : region{m_bytes, Size} {}

	private:
		byte_type m_bytes[Size];
	};

	template<class Alloc = std::allocator<byte_type>, class Enable = void>
	class alloc_region_factory;

	template<class Alloc>
	class alloc_region_factory<
			Alloc,
			typename std::enable_if_t<
					traits::is_allocator<Alloc>::value && std::is_same<typename Alloc::pointer, byte_type*>::value>>
		: public Alloc
	{
	public:
		alloc_region_factory(Alloc&& alloc) : Alloc{std::move(alloc)} {}

		alloc_region_factory(Alloc const& alloc) : Alloc{alloc} {}

		alloc_region_factory() : Alloc{} {}

		std::unique_ptr<region>
		create(size_type capacity)
		{
			return std::make_unique<alloc_region<Alloc>>(capacity, static_cast<Alloc>(*this));
		}
	};

	static dynamic_region*
	get_dynamic_region(region* rptr)
	{
		return dynamic_cast<dynamic_region*>(rptr);
	}

public:
	class binned_fixed_region_factory
	{
	public:
		binned_fixed_region_factory() {}

		std::unique_ptr<region>
		create(size_type size) const
		{
			size = (size < 16) ? 16 : size;
			std::size_t   index{0};
			auto          blog = std::ilogb(size - 1);
			std::uint64_t mask = (size - 1) >> (blog - 1);
			if (mask == 3)
				index = blog * 2 - 6;
			else if (mask == 2)
				index = blog * 2 - 7;
			else
				throw std::invalid_argument{"unexpected mask result"};

			switch (index)
			{
				case 0:
					return std::make_unique<fixed_region<16>>();
				case 1:
					return std::make_unique<fixed_region<24>>();
				case 2:
					return std::make_unique<fixed_region<32>>();
				case 3:
					return std::make_unique<fixed_region<48>>();
				case 4:
					return std::make_unique<fixed_region<64>>();
				case 5:
					return std::make_unique<fixed_region<96>>();
				case 6:
					return std::make_unique<fixed_region<128>>();
				case 7:
					return std::make_unique<fixed_region<192>>();
				case 8:
					return std::make_unique<fixed_region<256>>();
				case 9:
					return std::make_unique<fixed_region<384>>();
				case 10:
					return std::make_unique<fixed_region<512>>();
				case 11:
					return std::make_unique<fixed_region<768>>();
				case 12:
					return std::make_unique<fixed_region<1024>>();
				case 13:
					return std::make_unique<fixed_region<1536>>();
				case 14:
					return std::make_unique<fixed_region<2048>>();
				case 15:
					return std::make_unique<fixed_region<3072>>();
				case 16:
					return std::make_unique<fixed_region<4096>>();
				case 17:
					return std::make_unique<fixed_region<6144>>();
				case 18:
					return std::make_unique<fixed_region<8192>>();
				case 19:
					return std::make_unique<fixed_region<12288>>();
				case 20:
					return std::make_unique<fixed_region<16384>>();
				case 21:
					return std::make_unique<fixed_region<24576>>();
				case 22:
					return std::make_unique<fixed_region<32768>>();
				case 23:
					return std::make_unique<fixed_region<49152>>();
				case 24:
					return std::make_unique<fixed_region<65536>>();
				case 25:
					return std::make_unique<fixed_region<98304>>();
				case 26:
					return std::make_unique<fixed_region<131072>>();
				case 27:
					return std::make_unique<fixed_region<196608>>();
				case 28:
					return std::make_unique<fixed_region<262144>>();
				case 29:
					return std::make_unique<fixed_region<393216>>();
				case 30:
					return std::make_unique<fixed_region<524288>>();
				case 31:
					return std::make_unique<fixed_region<786432>>();
				case 32:
					return std::make_unique<fixed_region<1048576>>();
				case 33:
					return std::make_unique<fixed_region<1572864>>();
				case 34:
					return std::make_unique<fixed_region<2097152>>();
				case 35:
					return std::make_unique<fixed_region<3145728>>();
				case 36:
					return std::make_unique<fixed_region<4194304>>();
				case 37:
					return std::make_unique<fixed_region<6291456>>();
				case 38:
					return std::make_unique<fixed_region<8388608>>();
				case 39:
					return std::make_unique<fixed_region<12582912>>();
				case 40:
					return std::make_unique<fixed_region<16777216>>();
				default:
					throw std::invalid_argument{"size exceeds maximum"};
			};
		}
	};

	template<size_type Size>
	class fixed_region_factory
	{
	public:
		using region_type = fixed_region<Size>;

		fixed_region_factory() {}

		std::unique_ptr<region>
		create() const
		{
			return std::make_unique<region_type>();
		}
	};

	friend class const_buffer;

	/** \brief Destructor.
	 * 
	 */
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

	/** \brief Test for inequality with another buffer.
	 * 
	 * Negation of bool operator==(buffer const&).
	 * 
	 * \param rhs the buffer to be compared with this instance
	 * \return true if the buffers are not equal, false otherwise
	 */
	bool
	operator!=(buffer const& rhs) const
	{
		return !(*this == rhs);
	}

	/** \brief Construct a std::string value from the contents of this instance's memory region.
	 * 
	 * The contents of the memory region from data() to data() + size() - 1, or until the first null character
	 * is encountered, are copied into the
	 * resulting string value. The byte values from the buffer are copied with no 
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

	/** \brief Test this buffer for size() == 0
	 * 
	 * \return true if size() == 0, false otherwise
	 */
	bool
	empty() const
	{
		return m_size == 0;
	}

	/** \brief Calculate the CRC32 value for the contents of this buffer.
	 * 
	 * \return CRC32 value of buffer contents.
	 */
	checksum_type
	checksum() const
	{
		return checksum(0, size());
	}

	/** \brief Calculate the CRC32 value for the specified number of bytes in this buffer.
	 * 
	 * Calculate the CRC32 value for the first \e n bytes in this buffer, where n == min(size(), length).
	 * 
	 * \param length the number of bytes used to calculate the checksum value.
	 * \return CRC32 value of specied buffer contents.
	 */
	checksum_type
	checksum(size_type length) const
	{
		return checksum(0, std::min(length, size()));
	}

	/** \brief Calculate the CRC32 value for the specified sub-region of this buffer.
	 * 
	 * Calculate the CRC32 value for a sub-region of this buffer, begnning with byte
	 * at the specified \e offset from the beginning of the buffer, including \e n bytes,
	 * where n == min(length, size() - offset). If offset >= size(), the result is zero. 
	 * 
	 * \param length the number of bytes used to calculate the checksum value.
	 * \return CRC32 value of specied buffer contents.
	 */
	checksum_type
	checksum(size_type offset, size_type length) const;

	/** \brief Generate a hex/ASCII dump of the buffer contents on
	 * the specified output stream.
	 * 
	 * \param os ostream instance on which the dump will be generated.
	 */
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
 * it to a const_buffer or shared_buffer for consumption.
 * 
 * Valid instances of mutable_buffer hold a unique reference to an internal allocation object (an instance of 
 * the protected member class \e region) that owns either an allocator or a deleter (depending on how it was constructed).
 * If the region owns an allocator, it will be used to allocate and deallocate memory resources for the buffer as needed.
 * The internal allocation object may be moved to other instances of buffer types, via move constructors of move assignment
 * operators. If so, the allocator is moved with the associated memory resources; it is not owned by the buffer instance.
 * 
 * If an instance of mutable_buffer is created with a deleter (rather than an allocator), that deleter will similarly
 * be associated with the internal allocation object and memory resources. The deleter will be invoked to deallocate
 * memory resources at the appropriate time.
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
	 * The constructed instance has no allocated memory, such that
	 * data() == nullptr, capacity() == 0, size() == 0. The internal allocation
	 * object owns an instance of std::allocator<byte_type>, which will be used for subsequent
	 * allocation or deallocation operations.
	 */
	mutable_buffer() : m_region{std::make_unique<alloc_region<default_alloc>>()}, m_capacity{0}
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	/** \brief Construct with the specified allocator.
	 * 
	 * The provided allocator is associated with an internal allocation object (an instance
	 * of the protected member class \e region). Initially, no buffer space is allocated for the
	 * created instance (i.e., data() == nullptr, size() == 0, capacity() == 0). Subsequent calls
	 * to expand(size_type) will use the allocator to obtain memory resources, and to deallocate
	 * memory sources at the appropriate time.
	 * 
	 * \param alloc an instance of a class that satisfies the requirements for allocators, 
	 * in particular, an allocator with value_type logicmill::byte_type.
	 */
	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	mutable_buffer(_Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(std::forward<_Alloc>(alloc))}, m_capacity{0}
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	/** \brief Construct with the specified capacity.
	 * 
	 * An instance of the default allocator (std::allocator<byte_type>) is 
	 * associated with an internal allocation object (an instance
	 * of the protected member class \e region). The allocator is used to allocate a region of the
	 * specified capacity. The value of size() is set to zero.
	 * 
	 * \param capacity the number of bytes in the initial allocation.
	 */
	mutable_buffer(size_type capacity)
		: m_region{std::make_unique<alloc_region<default_alloc>>(capacity)}, m_capacity{capacity}
	{
		m_data = m_region->data();
		m_size = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	/** \brief Construct with the specified allocator and capacity.
	 * 
	 * The provided allocator is associated with an internal allocation object (an instance
	 * of the protected member class \e region). The allocator is used to allocate a region of the
	 * specified capacity. The value of size() is set to zero.
	 * 
	 * \param alloc an instance of a class that satisfies the requirements for allocators, 
	 * in particular, an allocator with value_type logicmill::byte_type.
	 * \param capacity the number of bytes in the initial allocation.
	 */
	template<
			class _Alloc,
			class = typename std::enable_if_t<
					traits::is_allocator<_Alloc>::value && std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	mutable_buffer(size_type capacity, _Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(capacity, std::forward<_Alloc>(alloc))}, m_capacity{capacity}
	{
		m_data = m_region->data();
		m_size = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	/** \brief Construct with the provided memory region and specified deleter.
	 * 
	 * This constructor form is useful in cases where memory regions are imported from other
	 * facilities that manage their own buffers. If a deleter is provided, it will be
	 * invoked when the internal allocation is destroyed. The constructed buffer, in effect, adopts
	 * the provided memory region (specified by arguments of the data and cap parameters). In such cases, 
	 * the specified deleter 
	 * would typically be used to notify the facility that originally allocated the memory region that 
	 * the memory region is no longer referenced by the mutable_buffer, and may be deallocated.
	 * The data() and capacity() of the resulting instance reflect the arguments of the \e data and \e capacity
	 * parameters; size() will be zero.
	 * 
	 * The deleter must be a callable object that is convertible to the type std::function<void(byte_type*)>,
	 * which will deallocate the object appropriately when invoked.
	 * 
	 * The internal allocation object is incapable of allocation new memory resources. If expand()
	 * is invoked on the contructed instance of mutable_buffer (or any instance of mutable_buffer that
	 * comes to posses the allocation object), an error condition will result (see expand() for details).
	 * 
	 * \param data a pointer to the memory region to be referenced by the constucted instance.
	 * \param capacity the size of the region, in bytes.
	 * \param del a callable deleter object.
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

	// this is probably evil:
	// mutable_buffer(void* data, size_type size)
	// 	: m_region{std::make_unique<del_region<null_delete<byte_type>>>(
	// 			  reinterpret_cast<byte_type*>(data),
	// 			  size)},
	// 	  m_capacity{size}
	// {
	// 	assert(false);
	// 	m_data = m_region->data();
	// 	m_size = 0;
	// 	ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	// }

	template<size_type Size>
	mutable_buffer(fixed_region_factory<Size> const& factory) : m_region{factory.create()}, m_capacity{Size}
	{
		m_data = m_region->data();
		m_size = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
	}

	template<class SizeType, SizeType Size>
	mutable_buffer(std::integral_constant<SizeType, Size> const&)
		: m_region{std::make_unique<fixed_region<Size>>()}, m_capacity{Size}
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

#endif

	mutable_buffer(mutable_buffer const&) = delete;


	/** Move constructor.
	 * 
	 * The ownership of the internal allocation object is transferred from the argument to the constructed
	 * instance. The values of data(), size(), and capacity() for the constructed instance are identitical
	 * to the corresponding values for \e rhs prior to construction. After construction, the state of rhs
	 * is such that data() == nullptr, size == 0, and capacity == 0. Also, since rhs has no internal allocation
	 * object (and thus, no allocator), an invocation of expand() will fail. It may subsequently come into 
	 * possession of a valid allocation object via move assignment.
	 * 
	 * \param rhs an instance of mutable_buffer, the contents of which are moved to the constructed instance.
	 * 
	 */
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
	/** Construct (by move) from another instance of mutable_buffer, as a sub-region of the moved instance.
	 * 
	 * The constucted instance has values such that data() == rhs.data() + offset, size() == length, 
	 * and capacity() == rhs.capacity() - offset. If (offset + length) > rhs.size(), an exception of 
	 * type std::system_error is thrown, with an encapsulated error code value std::errc::invalid_argument.
	 * 
	 * \param rhs an instance of mutable buffer whose internal allocation object is moved to the constructed instance.
	 * \param offset the offset of the value of data() in the constructed instance from rhs.data().
	 * \param length the size of the constructed instance.
	 */
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

	/** Construct (by move) from another instance of mutable_buffer, as a sub-region of the moved instance.
	 * 
	 * The constucted instance has values such that data() == rhs.data() + offset, size() == length, 
	 * and capacity() == rhs.capacity() - offset. If (offset + length) > rhs.size(),
	 * the \e err parameter is side-effected with the value std::errc::invalid_argument.
	 * 
	 * \param rhs an instance of mutable buffer whose internal allocation object is moved to the constructed instance.
	 * \param offset the offset of the value of data() in the constructed instance from rhs.data().
	 * \param length the size of the constructed instance.
	 * \param err reference to a std::error_code, side-effected to reflect any error condition occurring during construction.
	 */
	mutable_buffer(mutable_buffer&& rhs, size_type offset, size_type length, std::error_code& err)
		: m_region{std::move(rhs.m_region)}
	{
		ctor_body(rhs.m_data, rhs.m_size, offset, length, err);
		rhs.pclear();
	}

	mutable_buffer(const_buffer&& rhs, size_type offset, size_type length);

	mutable_buffer(const_buffer&& rhs, size_type offset, size_type length, std::error_code& err);

	mutable_buffer(const_buffer&& cbuf);

	template<
			class Buffer,
			class _Alloc,
			class = typename std::enable_if_t<
					is_buffer_type<Buffer>::value && std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	mutable_buffer(std::deque<Buffer> const& bufs, _Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(total_size(bufs), std::forward<_Alloc>(alloc))}
	{
		auto p = m_region->data();
		for (auto const& buf : bufs)
		{
			::memcpy(p, buf.data(), buf.size());
			p += buf.size();
		}
		std::error_code err;
		ctor_body(m_region->data(), m_region->capacity(), 0, m_region->capacity(), err);
	}

	template<class Buffer, class = typename std::enable_if_t<is_buffer_type<Buffer>::value>>
	mutable_buffer(std::deque<Buffer> const& bufs)
		: m_region{std::make_unique<alloc_region<default_alloc>>(total_size(bufs))}
	{
		auto p = m_region->data();
		for (auto const& buf : bufs)
		{
			::memcpy(p, buf.data(), buf.size());
			p += buf.size();
		}
		std::error_code err;
		ctor_body(m_region->data(), m_region->capacity(), 0, m_region->capacity(), err);
	}


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
		return (get_dynamic_region(m_region.get()) != nullptr);
	}

	mutable_buffer&
	expand(size_type new_cap, std::error_code& err)
	{
		if (!m_region)
		{
			*this = std::move(mutable_buffer{new_cap});    // TODO: WTF?
		}

		auto dyn_region = get_dynamic_region(m_region.get());
		if (!dyn_region)
		{
			err = make_error_code(std::errc::operation_not_supported);
			goto exit;
		}


		ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);

		err.clear();

		if (new_cap > m_capacity)
		{
			if (!m_data)
			{
				assert(m_capacity == 0);
				assert(m_size == 0);
				assert(dyn_region->data() == nullptr);
				assert(dyn_region->capacity() == 0);
				m_data = dyn_region->allocate(new_cap, err);
				if (err)
					goto exit;

				m_capacity = dyn_region->capacity();
			}
			else
			{
				assert(m_capacity > 0);
				m_data = dyn_region->reallocate(m_size, new_cap, err);
				if (err)
					goto exit;

				m_capacity = dyn_region->capacity();
			}
			// m_data     = m_region->data();
			// m_capacity = m_region->capacity();
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
	virtual std::unique_ptr<mutable_buffer_factory>
	dup() const = 0;
	virtual size_type
	size() const = 0;
	virtual mutable_buffer
	create() = 0;
};

template<size_type Size>
class mutable_buffer_fixed_factory : private buffer::fixed_region_factory<Size>, public mutable_buffer_factory
{
public:
	using factory_type = buffer::fixed_region_factory<Size>;

	virtual std::unique_ptr<mutable_buffer_factory>
	dup() const override
	{
		return std::make_unique<mutable_buffer_fixed_factory>();
	}

	virtual mutable_buffer
	create() override
	{
		return mutable_buffer{static_cast<factory_type&>(*this)};
	}

	virtual size_type
	size() const override
	{
		return Size;
	}
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
	mutable_buffer_alloc_factory(size_type size, Alloc&& alloc) : Alloc{std::move(alloc)}, m_alloc_size{size} {}

	mutable_buffer_alloc_factory(size_type size, Alloc const& alloc) : Alloc{alloc}, m_alloc_size{size} {}

	template<class = std::enable_if_t<std::is_default_constructible<Alloc>::value>>
	mutable_buffer_alloc_factory(size_type size) : Alloc{Alloc{}}, m_alloc_size{size}
	{}

	mutable_buffer_alloc_factory(mutable_buffer_alloc_factory const& rhs)
		: Alloc{static_cast<Alloc>(rhs)}, m_alloc_size{rhs.m_alloc_size}
	{}

	virtual std::unique_ptr<mutable_buffer_factory>
	dup() const override
	{
		return std::make_unique<mutable_buffer_alloc_factory>(*this);
	}

	virtual mutable_buffer
	create() override
	{
		return mutable_buffer{m_alloc_size, static_cast<Alloc>(*this)};
	}

	virtual size_type
	size() const override
	{
		return m_alloc_size;
	}

private:
	size_type m_alloc_size;
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

	const_buffer(const_buffer&& rhs) : m_region{std::move(rhs.m_region)}
	{
		m_data     = rhs.m_data;
		m_size     = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

	const_buffer(mutable_buffer&& rhs) : m_region{std::move(rhs.m_region)}
	{
		m_data         = rhs.m_data;
		m_size         = rhs.m_size;
		rhs.m_data     = nullptr;
		rhs.m_size     = 0;
		rhs.m_capacity = 0;
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

	template<
			class Buffer,
			class _Alloc,
			class = typename std::enable_if_t<
					is_buffer_type<Buffer>::value && std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	const_buffer(std::deque<Buffer> const& bufs, _Alloc&& alloc)
		: m_region{std::make_unique<alloc_region<_Alloc>>(total_size(bufs), std::forward<_Alloc>(alloc))}
	{
		auto p = m_region->data();
		for (auto const& buf : bufs)
		{
			::memcpy(p, buf.data(), buf.size());
			p += buf.size();
		}
		m_data = m_region->data();
		m_size = m_region->capacity();
		ASSERT_CONST_BUFFER_INVARIANTS(*this);
	}

	template<class Buffer, class = typename std::enable_if_t<is_buffer_type<Buffer>::value>>
	const_buffer(std::deque<Buffer> const& bufs)
		: m_region{std::make_unique<alloc_region<default_alloc>>(total_size(bufs))}
	{
		auto p = m_region->data();
		for (auto const& buf : bufs)
		{
			::memcpy(p, buf.data(), buf.size());
			p += buf.size();
		}
		m_data = m_region->data();
		m_size = m_region->capacity();
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

		{
			auto dyn_region = get_dynamic_region(m_region.get());
			if (!dyn_region)
			{
				err = make_error_code(std::errc::operation_not_supported);
				goto exit;
			}

			m_data = dyn_region->allocate(length, err);
			if (err)
				goto exit;

			::memcpy(m_data, data + offset, length);
			m_size = length;

			ASSERT_CONST_BUFFER_INVARIANTS(*this);
		}


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
	util::shared_ptr<region> m_region;

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

	shared_buffer() : m_region{util::make_shared<alloc_region<default_alloc>>()}
	{
		m_data = nullptr;
		m_size = 0;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	template<class _Del>
	shared_buffer(void* data, size_type size, _Del&& del)
		: m_region{util::make_shared<del_region<_Del>>(
				  reinterpret_cast<byte_type*>(data),
				  size,
				  std::forward<_Del>(del))}
	{
		m_data = reinterpret_cast<byte_type*>(data);
		m_size = size;
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	shared_buffer(const void* data, size_type size, _Alloc&& alloc)
		: m_region{util::make_shared<alloc_region<_Alloc>>(
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
		: m_region{util::make_shared<alloc_region<default_alloc>>(reinterpret_cast<const byte_type*>(data), size)}
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
		: m_region{util::make_shared<alloc_region<_Alloc>>(rhs.data(), rhs.size(), std::forward<_Alloc>(alloc))}
	{
		m_data = m_region->data();
		m_size = rhs.size();
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	template<
			class Buffer,
			class _Alloc,
			class = typename std::enable_if_t<
					is_buffer_type<Buffer>::value && std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	shared_buffer(std::deque<Buffer> const& bufs, _Alloc&& alloc)
		: m_region{util::make_shared<alloc_region<_Alloc>>(total_size(bufs), std::forward<_Alloc>(alloc))}
	{
		auto p = m_region->data();
		for (auto const& buf : bufs)
		{
			::memcpy(p, buf.data(), buf.size());
			p += buf.size();
		}
		m_data = m_region->data();
		m_size = m_region->capacity();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	template<class Buffer, class = typename std::enable_if_t<is_buffer_type<Buffer>::value>>
	shared_buffer(std::deque<Buffer> const& bufs)
		: m_region{util::make_shared<alloc_region<default_alloc>>(total_size(bufs))}
	{
		auto p = m_region->data();
		for (auto const& buf : bufs)
		{
			::memcpy(p, buf.data(), buf.size());
			p += buf.size();
		}
		m_data = m_region->data();
		m_size = m_region->capacity();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

	shared_buffer(buffer const& rhs) : m_region{util::make_shared<alloc_region<default_alloc>>(rhs.data(), rhs.size())}
	{
		m_data = m_region->data();
		m_size = rhs.size();
		// announce();
		ASSERT_SHARED_BUFFER_INVARIANTS(*this);
	}

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
		auto dyn_region = get_dynamic_region(m_region.get());
		assert(dyn_region != nullptr);
		m_data = dyn_region->allocate(length);
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
		auto dyn_region = get_dynamic_region(m_region.get());
		assert(dyn_region != nullptr);
		m_data = dyn_region->allocate(length);
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
		{
			auto dyn_region = get_dynamic_region(m_region.get());
			assert(dyn_region != nullptr);
			m_data = dyn_region->allocate(length, err);
			if (err)
				goto exit;
			::memcpy(m_data, rhs.data() + offset, length);
			m_size = length;
			// announce();
			ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		}
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
		{
			auto dyn_region = get_dynamic_region(m_region.get());
			assert(dyn_region != nullptr);
			m_data = dyn_region->allocate(length, err);
			if (err)
				goto exit;
			::memcpy(m_data, rhs.data() + offset, length);
			m_size = length;
			// announce();
			ASSERT_SHARED_BUFFER_INVARIANTS(*this);
		}
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
		m_region = util::make_shared<alloc_region<default_alloc>>(rhs.data(), rhs.size());
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

}    // namespace util
}    // namespace logicmill

template<>
logicmill::size_type inline logicmill::util::total_size<logicmill::util::buffer>(
		std::deque<logicmill::util::buffer> const& bufs)
{
	size_type result{0};
	for (auto const& buf : bufs)
	{
		result += buf.size();
	}
	return result;
}

template<>
inline logicmill::size_type
logicmill::util::total_size<logicmill::util::mutable_buffer>(std::deque<logicmill::util::mutable_buffer> const& bufs)
{
	size_type result{0};
	for (auto const& buf : bufs)
	{
		result += buf.size();
	}
	return result;
}
template<>
inline logicmill::size_type
logicmill::util::total_size<logicmill::util::const_buffer>(std::deque<logicmill::util::const_buffer> const& bufs)
{
	size_type result{0};
	for (auto const& buf : bufs)
	{
		result += buf.size();
	}
	return result;
}

template<>
inline logicmill::size_type
logicmill::util::total_size<logicmill::util::shared_buffer>(std::deque<logicmill::util::shared_buffer> const& bufs)
{
	size_type result{0};
	for (auto const& buf : bufs)
	{
		result += buf.size();
	}
	return result;
}

template<>
struct logicmill::util::buf::traits<logicmill::util::mutable_buffer>
{
	using buffer_type            = mutable_buffer;
	using buffer_ref_type        = buffer_type&;
	using buffer_rvalue_ref_type = buffer_type&&;
	using buffer_const_ref_type  = buffer_type const&;
	using element_type           = std::uint8_t;
	using pointer_type           = std::uint8_t*;
	using size_type              = std::size_t;
	using const_pointer_type     = void;

	struct is_mutable : public std::true_type
	{};
	struct has_capacity : public std::true_type
	{};
	struct can_set_size : public std::true_type
	{};
	struct can_realloc : public std::true_type
	{};
	struct owns_memory : public std::true_type
	{};
	struct scoped_release : public std::true_type
	{};
	struct data_may_return_null : public std::true_type
	{};
	struct is_copy_constructible : public std::false_type
	{};
	struct is_move_constructible : public std::true_type
	{};
	struct is_copy_assignable : public std::false_type
	{};
	struct is_move_assignable : public std::true_type
	{};
	struct has_explicit_release : public std::false_type
	{};

	template<class... Args>
	struct is_constructible_from : public std::is_constructible<buffer_type, Args...>
	{};

	static std::enable_if_t<is_mutable::value, pointer_type>
	data(buffer_ref_type buf)
	{
		return buf.data();
	}

	// static std::enable_if_t<!is_mutable::value, const_pointer_type> data(buffer_const_ref_type)
	// {

	// }
	static size_type
	size(buffer_const_ref_type buf)
	{
		return buf.size();
	}

	static std::enable_if_t<has_capacity::value, size_type>
	capacity(buffer_const_ref_type buf)
	{
		return buf.capacity();
	}

	static std::enable_if_t<can_set_size::value, void>
	set_size(buffer_ref_type buf, size_type new_size)
	{
		buf.size(new_size);
	}

	static std::enable_if_t<can_realloc::value, void>
	realloc(buffer_ref_type buf, size_type new_cap)
	{
		buf.expand(new_cap);
	}

	static_assert(!(!is_mutable::value && can_realloc::value), "non-mutable buffer types can't realloc");
	static_assert(!(!is_mutable::value && can_set_size::value), "non-mutable buffer types can't set_size");
	static_assert(!(!owns_memory::value && scoped_release::value), "non-mutable buffer types can't set_size");
	static_assert(!(!owns_memory::value && has_explicit_release::value), "non-mutable buffer types can't set_size");
};

#endif    // LOGICMILL_UTIL_BUFFER_H
