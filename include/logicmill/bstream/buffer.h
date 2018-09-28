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

#ifndef LOGICMILL_BSTREAM_BUFFER_H
#define LOGICMILL_BSTREAM_BUFFER_H

#include <cstdint>
#include <functional>
#include <limits>
#include <system_error>
#include <assert.h>
#include <iostream>
#include <logicmill/util/macros.h>
#include <logicmill/bstream/types.h>

#ifndef NDEBUG

#define ASSERT_MUTABLE_BUFFER_INVARIANTS( _buf_ )															\
{																											\
	assert( ( _buf_ ).m_alloc != nullptr );																	\
	assert( ( _buf_ ).m_data == ( _buf_ ).m_alloc->data() );												\
	assert( ( _buf_ ).m_size <= ( _buf_ ).m_alloc->capacity() );											\
	assert( ( _buf_ ).m_capacity == ( _buf_ ).m_alloc->capacity() );										\
}																											\
/**/

#if 0

#define ASSERT_MUTABLE_BUFFER_INVARIANTS( _buf_ )															\
{																											\
	if ( ( _buf_ ).m_alloc != nullptr )																		\
	{																										\
		assert( ( _buf_ ).m_data == ( _buf_ ).m_alloc->data() );											\
		assert( ( _buf_ ).m_size <= ( _buf_ ).m_alloc->capacity() );										\
		assert( ( _buf_ ).m_capacity == ( _buf_ ).m_alloc->capacity() );									\
	}																										\
	else																									\
	{																										\
		assert( ( _buf_ ).m_data == nullptr );																\
		assert( ( _buf_ ).m_capacity == 0 );																\
		assert( ( _buf_ ).m_size == 0 );																	\
	}																										\
}																											\
/**/

#endif 

#define ASSERT_CONST_BUFFER_INVARIANTS( _buf_ )																\
{																											\
	assert( ( _buf_ ).m_alloc != nullptr );																	\
	assert( ( _buf_ ).m_data >= ( _buf_ ).m_alloc->data() );												\
	assert( ( _buf_ ).m_data <= ( ( _buf_ ).m_alloc->data() + ( _buf_ ).m_alloc->capacity() ) );				\
	assert( ( ( _buf_ ).m_data + ( _buf_ ).m_size )															\
			<= ( ( _buf_ ).m_alloc->data() + ( _buf_ ).m_alloc->capacity() ) );								\
}																											\
/**/

#define ASSERT_SHARED_BUFFER_INVARIANTS( _buf_ )															\
	ASSERT_CONST_BUFFER_INVARIANTS( _buf_ )																	\
/**/

#else

#define ASSERT_MUTABLE_BUFFER_INVARIANTS( _buf_ )

#define ASSERT_CONST_BUFFER_INVARIANTS( _buf_ )

#define ASSERT_SHARED_BUFFER_INVARIANTS( _buf_ )

#endif

namespace logicmill
{
namespace bstream
{

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

	/** \brief The type used to represent a single byte in the memory region.
	 */
	// using byte_type = std::uint8_t;
	/** \brief The type used to represent the size and capacity of a buffer.
	 */
	// using size_type = std::size_t;
	/** \brief The type used to represent the absolute position of a byte in 
	 * the memory region, as an offset from the beginning of the region.
	 */
	// using position_type = std::uint64_t;

	//	using offset_type = std::int64_t;

	/** \brief The type used to represent a checksum value calculated for
	 * the memory region.
	 */
	using checksum_type = std::uint32_t;

	/** \brief A value representing an invalid size.
	 */
	// static const size_type npos = std::numeric_limits< size_type >::max();

	/** \brief The type of a <i>callable element</i> (a function or function object) that can be invoked to allocate a region of memory.
	 * 
	 * \param size the size of the region to be allocated
	 * \return	a pointer to the allocated region
	 * \see memory_broker
	 * \see default_allocator
	 */
	using allocator = std::function< byte_type* ( size_type size ) >;

	/** \brief The type of a <i>callable element</i> (a function or function object) that can be invoked to reallocate a region of memory.
	 * It is typically used to either expand the size of the current region (if possible) or to allocate a new region
	 * and copy the contents of the existing region into the new region.
	 * 
	 * \param data a pointer to the memory region currently in use by the owning buffer
	 * \param current_size the number of bytes in the current region (data) whose values will be preserved in the reallocated region
	 * \return	a pointer to the reallocated region
	 * \see memory_broker
	 * \see default_reallocator
	 */
	using reallocator = std::function< byte_type* ( byte_type* data, size_type current_size, size_type new_size ) >;

	/** \brief The type of a <i>callable element</i> (a function or function object) that can be invoked to deallocate a region of memory.
	 * 
	 * \param data a pointer to the memory region to be deallocated
	 * \see memory_broker
	 * \see default_deallocator
	 */
	using deallocator = std::function< void ( byte_type* data ) >;

	/** \brief The default implementation of allocator used by buffer instances to allocate memory regions.
	 * This implementation calls the malloc() standard library function.
	 */
	struct default_allocator
	{
		/** \brief Allocates a memory region of the specified capacity.
		 * 
		 * \param capacity the number of bytes to allocate
		 * \return a pointer to the allocated region
		 * \see memory_broker
		 * \see allocator
		 */
		byte_type* operator()( size_type capacity ) const
		{
			return reinterpret_cast< byte_type* >( ::malloc( capacity ) );
		}
	};

LGCML_UTIL_START_DISABLE_UNUSED_VALUE_WARNING()

	/** \brief The default implementation of reallocator used by buffer instances to reallocate memory regions.
	 * This implementation calls the realloc() standard library function.
	 */
	struct default_reallocator
	{
		/** \brief Rellocates a memory region, preserving the values in the current buffer.
		 * 
		 * \param data a pointer to the current region
		 * \param size the number of bytes in the current region whose values will be preserved
		 * \param new_cap the capacity of the resulting region
		 * \return a pointer to the reallocated region
		 * \see memory_broker
		 * \see reallocator
		 */
		byte_type* operator()( byte_type* data, size_type size, size_type new_cap ) const
		{
			return reinterpret_cast< byte_type* >( ::realloc( data, new_cap ) );
		}
	};

LGCML_UTIL_END_DISABLE_UNUSED_VALUE_WARNING()

	/** \brief The default implementation of deallocator used by buffer instances to release memory regions.
	 * This implementation calls the free() standard library function.
	 */
	struct default_deallocator
	{
		/** \brief Deallocates a memory region.
		 * 
		 * \param data a pointer to the region to be deallocated
		 * \see memory_broker
		 * \see deallocator
		 */
		void operator()( byte_type* data ) const
		{
			::free( data );
		}
	};

	/** \brief An implementation of deallocator that does nothing.
	 * 
	 * This implementation is useful in situations where the calling environment
	 * assumes responsibility for managing memory region lifecycles, to prevent
	 * deallocation by the buffer object.
	 */
	struct null_deallocator
	{
		void operator()( byte_type* ) const
		{}
	};

	class memory_broker
	{
	public:

		using ptr = std::shared_ptr< memory_broker >;

		virtual ~memory_broker() {}

		virtual byte_type* 
		allocate( size_type capacity ) = 0;

		virtual byte_type*
		reallocate( byte_type* data, size_type preserve, size_type new_capacity ) = 0;

		virtual void
		deallocate( byte_type* data ) = 0;

		virtual bool
		can_allocate() const = 0;

		virtual bool
		can_reallocate() const = 0;

		virtual bool
		can_deallocate() const = 0;

	};

	class null_broker : public memory_broker
	{
	public:

		using ptr = std::shared_ptr< null_broker >;

		static memory_broker::ptr
		get()
		{
			static memory_broker::ptr broker = std::make_shared< null_broker >();
			return broker;
		}

		virtual byte_type* 
		allocate( size_type capacity ) override
		{
			throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
		}

		virtual byte_type*
		reallocate( byte_type* buf, size_type preserve, size_type new_capacity ) override
		{
			throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
		}

		virtual void
		deallocate( byte_type* buf ) override
		{
			throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
		}

		virtual bool
		can_allocate() const override
		{
			return false;
		}

		virtual bool
		can_reallocate() const override
		{
			return false;
		}

		virtual bool
		can_deallocate() const override
		{
			return false;
		}

	};

	class default_broker : public memory_broker
	{
	public:

		using ptr = std::shared_ptr< default_broker >;

		static memory_broker::ptr
		get()
		{
			static memory_broker::ptr broker = std::make_shared< default_broker >();
			return broker;
		}

		virtual byte_type* 
		allocate( size_type capacity ) override
		{
			return ( capacity > 0 ) ? reinterpret_cast< byte_type* >( ::malloc( capacity ) ) : nullptr;
		}

		virtual byte_type*
		reallocate( byte_type* data, size_type, size_type new_capacity ) override
		{
			return ( new_capacity > 0 ) ? reinterpret_cast< byte_type* >( ::realloc( data, new_capacity ) ) : nullptr;
		}

		virtual void
		deallocate( byte_type* data ) override
		{
			::free( data );
		}

		virtual bool
		can_allocate() const override
		{
			return true;
		}

		virtual bool
		can_reallocate() const override
		{
			return true;
		}

		virtual bool
		can_deallocate() const override
		{
			return true;
		}
	};

	class no_realloc_broker : public default_broker
	{
	public:

		using ptr = std::shared_ptr< no_realloc_broker >;

		static memory_broker::ptr
		get()
		{
			static memory_broker::ptr broker = std::make_shared< no_realloc_broker >();
			return broker;
		}

		virtual byte_type*
		reallocate( byte_type* buf, size_type preserve, size_type new_capacity ) override
		{
			throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
		}

		virtual bool
		can_reallocate() const override
		{
			return false;
		}

	};

	template< class Alloc, class Realloc, class Dealloc, class = std::enable_if_t< 
		std::is_convertible_v< Alloc, allocator >
		&& std::is_convertible_v< Realloc, reallocator >
		&& std::is_convertible_v< Dealloc, deallocator > > >
	static memory_broker::ptr
	create_broker( Alloc&& a, Realloc&& r, Dealloc&& d )
	{

		class ard_broker : public memory_broker
		{
		public:

			ard_broker( Alloc&& a, Realloc&& r, Dealloc&& d )
			:
			m_alloc{ std::forward< Alloc >( a ) },
			m_realloc{ std::forward< Realloc >( r ) },
			m_dealloc{ std::forward< Dealloc >( d ) }
			{}

			virtual byte_type* 
			allocate( size_type capacity ) override
			{
				return m_alloc( capacity );
			}

			virtual byte_type*
			reallocate( byte_type* data, size_type preserve, size_type new_capacity ) override
			{
				return m_realloc( data, preserve, new_capacity );
			}

			virtual void
			deallocate( byte_type* data ) override
			{
				m_dealloc( data );
			}

			virtual bool
			can_allocate() const override
			{
				return true;
			}

			virtual bool
			can_reallocate() const override
			{
				return true;
			}

			virtual bool
			can_deallocate() const override
			{
				return true;
			}

		private:
			Alloc			m_alloc;
			Realloc			m_realloc;
			Dealloc			m_dealloc;
		};

		return std::make_shared< ard_broker >(
			std::forward< Alloc>( a ), 
			std::forward< Realloc>( r ), 
			std::forward< Dealloc >( d  ) );
	}

	template< class Alloc, class Dealloc, class = std::enable_if_t< 
		std::is_convertible_v< Alloc, allocator >
		&& std::is_convertible_v< Dealloc, deallocator > > >
	static memory_broker::ptr
	create_broker( Alloc&& a, Dealloc&& d )
	{

		class ad_broker : public memory_broker
		{
		public:

			ad_broker( Alloc&& a, Dealloc&& d )
			:
			m_alloc{ std::forward< Alloc >( a ) },
			m_dealloc{ std::forward< Dealloc >( d ) }
			{}

			virtual byte_type* 
			allocate( size_type capacity ) override
			{
				return m_alloc( capacity );
			}

			virtual byte_type*
			reallocate( byte_type* data, size_type preserve, size_type new_capacity ) override
			{
				throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
			}

			virtual void
			deallocate( byte_type* data ) override
			{
				m_dealloc( data );
			}

			virtual bool
			can_allocate() const override
			{
				return true;
			}

			virtual bool
			can_reallocate() const override
			{
				return false;
			}

			virtual bool
			can_deallocate() const override
			{
				return true;
			}

		private:
			Alloc			m_alloc;
			Dealloc			m_dealloc;
		};

		return std::make_shared< ad_broker >( std::forward< Alloc>( a ), std::forward< Dealloc >( d  ) );
	}

	template< class Dealloc, class = std::enable_if_t< std::is_convertible_v< Dealloc, deallocator > > >
	static memory_broker::ptr
	create_broker( Dealloc&& d )
	{

		class d_broker : public memory_broker
		{
		public:

			d_broker( Dealloc&& d )
			:
			m_dealloc{ std::forward< Dealloc >( d ) }
			{}

			virtual byte_type* 
			allocate( size_type capacity ) override
			{
				throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
			}

			virtual byte_type*
			reallocate( byte_type* data, size_type preserve, size_type new_capacity ) override
			{
				throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
			}

			virtual void
			deallocate( byte_type* data ) override
			{
				m_dealloc( data );
			}

			virtual bool
			can_allocate() const override
			{
				return false;
			}

			virtual bool
			can_reallocate() const override
			{
				return false;
			}

			virtual bool
			can_deallocate() const override
			{
				return true;
			}

		private:
			Dealloc			m_dealloc;
		};

		return std::make_shared< d_broker >( std::forward< Dealloc >( d  ) );
	}

protected:

	class allocation
	{
	public:
		using sptr = std::shared_ptr< allocation >;
		using uptr = std::unique_ptr< allocation >;

		allocation( memory_broker::ptr broker = default_broker::get() )
		:
		m_broker{ broker },
		m_capacity{ 0 },
		m_data{ nullptr }
		{}

		allocation( size_type capacity, memory_broker::ptr broker = default_broker::get() )
		:
		m_broker{ broker },
		m_capacity{ capacity },
		m_data{ m_broker->allocate( capacity ) }
		{}

		allocation( const byte_type* data, size_type size, memory_broker::ptr broker = default_broker::get() )
		:
		m_broker{ broker },
		m_capacity{ ( ( data != nullptr ) ? size : 0 ) },
		m_data{ ( ( data != nullptr ) ? m_broker->allocate( size ) : nullptr ) }
		{
			if ( m_data && data && size > 0 )
			{
				::memcpy( m_data, data, size );
			}
		}

		template< class Dealloc, class = std::enable_if_t< std::is_convertible_v< Dealloc, deallocator > > >
		allocation( byte_type* data, size_type size, Dealloc&& dealloc_functor )
		:
		m_broker{ create_broker( std::forward< Dealloc >( dealloc_functor ) ) },
		m_capacity{ size },
		m_data{ data }
		{}

		~allocation()
		{
			if ( m_broker->can_deallocate() && m_data )
			{
				m_broker->deallocate( m_data );
			}
			m_data = nullptr;
			m_capacity = 0;
			m_broker.reset();
		}

		byte_type* data()
		{
			return m_data;
		}

		size_type capacity()
		{
			return m_capacity;
		}

		bool can_allocate() const
		{
			return m_broker->can_allocate();
		}

		bool can_reallocate() const
		{
			return m_broker->can_reallocate();
		}

		bool can_deallocate() const
		{
			return m_broker->can_deallocate();
		}

		byte_type* allocate( size_type capacity )
		{
			if ( m_data )
			{
				deallocate();
			}
			assert( m_data == nullptr );
			if ( capacity > 0 )
			{
				m_data = m_broker->allocate( capacity );
			}
			m_capacity = capacity;
			return m_data;
		}

		byte_type* reallocate( size_type new_capacity )
		{
			return reallocate( m_capacity, new_capacity );
		}

		byte_type* reallocate( size_type current_size, size_type new_capacity )
		{
			if ( ! m_broker->can_reallocate() )
			{
				throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
			}
			if ( ! m_data && new_capacity > 0 )
			{
				m_data = m_broker->allocate( new_capacity );
				m_capacity = new_capacity;
			}
			else if ( new_capacity > m_capacity )
			{
				current_size = ( current_size > m_capacity ) ?  m_capacity : current_size;
				m_data = m_broker->reallocate( m_data, current_size, new_capacity );
				m_capacity = new_capacity;
			}
			return m_data;
		}

		void deallocate()
		{
			if ( m_data && m_broker->can_deallocate() )
			{
				m_broker->deallocate( m_data );
				m_data = nullptr;
				m_capacity = 0;
			}
		}

	private:
		memory_broker::ptr			m_broker;
		size_type 					m_capacity;
		byte_type* 					m_data;
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
	operator==( buffer const& rhs ) const
	{
		bool result = true;

		if ( this != &rhs )
		{
			if ( m_size != rhs.m_size )
			{
				result = false;
			}
			else
			{
				if ( m_size != 0 )
				{
					result = ( std::memcmp( m_data, rhs.m_data, m_size ) == 0 );
				}
			}
		}
		return result;
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
		if ( m_size > 0 )
		{
			result = std::string( reinterpret_cast< const char* >( m_data ), m_size );
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
		return std::string_view( reinterpret_cast< char* >( m_data ), m_size );
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
		return checksum( 0, size() );
	}

	checksum_type
	checksum( size_type length ) const
	{
		return checksum( 0, std::min( length, size() ) );
	}

	checksum_type
	checksum( size_type offset, size_type length ) const;

	void
	dump( std::ostream& os ) const;

protected:

	buffer()
	:
	m_data{ nullptr },
	m_size{ 0 }
	{}

	buffer( byte_type* data, size_type size )
	:
	m_data{ data },
	m_size{ size }
	{}

	byte_type*				m_data;
	size_type				m_size;

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
public:

	using buffer::data;
	using buffer::size;

	friend class const_buffer;
	friend class shared_buffer;

	/** \brief Default constructor.
	 * 
	 * The constructed instance owns a copy of the default memory_broker, and no allocated memory region, such that
	 * data() == nullptr, capacity() == 0, size() == 0.
	 */
	// mutable_buffer()
	// :
	// m_alloc{ std::make_unique< allocation >() },
	// m_capacity{ 0 }
	// {
	// 	ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	// }

	/** \brief Construct with the specified memory_broker.
	 * 
	 * \param broker a shared pointer to the memory_broker to be used by the allocation
	 */
	mutable_buffer( memory_broker::ptr broker = default_broker::get() )
	:
	m_alloc{ std::make_unique< allocation >( broker ) },
	m_capacity{ 0 }
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	/** \brief Construct with the specified capacity.
	 * 
	 * The consructed instance uses the default memory_broker to allocate a region with the specified capacity.
	 * The size of the resulting instance is set to zero.
	 * 
	 * \param capacity the capacity of the allocated memory region
	 */
	// mutable_buffer( size_type capacity )
	// :
	// m_alloc{ std::make_unique< buffer::allocation >( capacity ) },
	// m_capacity{ capacity }
	// {
	// 	// if ( capacity > 0 )
	// 	// {
	// 	// 	m_data = m_alloc->allocate( capacity );
	// 	// 	assert( m_data != nullptr );
	// 	// 	m_capacity = capacity;
	// 	// 	m_size = 0;
	// 	// }
	// 	m_data = m_alloc->data();
	// 	m_size = 0;
	// 	ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	// }

	/** \brief Construct with the specified memory_broker and capacity
	 * 
	 * The consructed instance uses the provided memory_broker to allocate a region with the specified capacity.
	 * The size of the resulting instance is set to zero.
	 * 
	 * \param capacity the capacity of the allocated memory region
	 * \param alloc a universal reference to an allocation_manager that is forwarded to the constructed instance
	 */
	mutable_buffer( size_type capacity, memory_broker::ptr broker = default_broker::get() )
	:
	m_alloc{ std::make_unique< allocation >( capacity, broker ) },
	m_capacity{ capacity }
	{
		// if ( m_alloc->can_allocate() )
		// {
		// 	m_data = m_alloc->allocate( capacity );
		// 	assert( m_data != nullptr );
		// 	m_capacity = capacity;
		// 	m_size = 0;
		// }
		m_data = m_alloc->data();
		m_size = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
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

	template< class Dealloc, class = std::enable_if_t< std::is_convertible_v< Dealloc, deallocator > > >
	mutable_buffer( void* data, size_type capacity, Dealloc&& dealloc_f )
	:
	m_alloc{ std::make_unique< allocation >( reinterpret_cast< byte_type* >( data ), capacity, std::forward< Dealloc >( dealloc_f ) ) },
	m_capacity{ capacity }
	{
		m_data = reinterpret_cast< byte_type* >( data );
		m_size = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

#if 0
	mutable_buffer( const void* data, size_type capacity ) // TODO: mostly here for testing, consider removing
	:
	m_alloc{ std::make_unique< allocation >( reinterpret_cast< const byte_type* >( data ), capacity ) },
	m_capacity{ capacity }
	{
		m_data = m_alloc->data();
		m_size = capacity;
	}

	mutable_buffer( std::string const& s )
	:
	m_alloc{ std::make_unique< allocation >( reinterpret_cast< const byte_type* >( s.data() ), s.size() ) },
	m_capacity{ s.size() }
	{
		m_data = m_alloc->data();
		m_size = s.size();
	}

#endif

	mutable_buffer( mutable_buffer const& ) = delete;

	mutable_buffer( mutable_buffer&& rhs )
	:
	// buffer{ rhs.m_data, rhs.m_size },
	m_alloc{ std::move( rhs.m_alloc ) },
	m_capacity{ rhs.m_capacity }
	{
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		rhs.m_capacity = 0;
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	mutable_buffer&
	operator=( mutable_buffer&& rhs )
	{
		// don't assert invariants, this (lhs of assign) may be victim of a move
		// ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		ASSERT_MUTABLE_BUFFER_INVARIANTS( rhs );

		m_alloc = std::move( rhs.m_alloc );
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		m_capacity = rhs.m_capacity;

		rhs.m_data = nullptr;
		rhs.m_size = 0;
		rhs.m_capacity = 0;

		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );

		return *this;
	}

	mutable_buffer&
	operator=( mutable_buffer const& ) = delete;

	byte_type*
	data()
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return m_data;
	}

	void
	size( size_type new_size, std::error_code& err )
	{
		err.clear();

		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		if (new_size > m_capacity ) 
		{ 
			err = make_error_code( std::errc::invalid_argument );
			goto exit;
		}

		m_size = new_size;
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	exit:
		return;
	}

	void
	size( size_type new_size )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		if (new_size > m_capacity )
		{ 
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) }; 
		}
		m_size = new_size;
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	size_type
	capacity() const
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return m_capacity;
	}

	bool is_expandable() const
	{
		return m_alloc->can_reallocate();
	}

	mutable_buffer&
	expand( size_type new_cap, std::error_code& err )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );

		err.clear();

		if ( new_cap > m_capacity )
		{
			if ( ! m_data )
			{
				assert( m_capacity == 0 );
				assert( m_size == 0 );
				assert( m_alloc->data() == nullptr );
				assert( m_alloc->capacity() == 0 );
				if ( m_alloc->can_allocate() )
				{
					m_data = m_alloc->allocate( new_cap );
					m_capacity = m_alloc->capacity();
				}
				else
				{
					err = make_error_code( std::errc::operation_not_supported );
					goto exit;
				}
			}
			else if ( m_alloc->can_reallocate() )
			{
				assert( m_capacity > 0 );
				m_alloc->reallocate( m_size, new_cap );
			}
			else
			{
				err = make_error_code( std::errc::operation_not_supported );
				goto exit;
			}
			m_data = m_alloc->data();
			m_capacity = m_alloc->capacity();
		}

	exit:
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return *this;
	}

	mutable_buffer&
	expand( size_type new_capacity )
	{
		std::error_code err;
		expand( new_capacity, err );
		if ( err )
		{
			throw std::system_error{ err };
		}
		
	exit:
		return *this;
	}

	mutable_buffer&
	fill( position_type offset, size_type length, byte_type value, std::error_code& err )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		if ( offset + length > m_capacity )
		{
			err = make_error_code( std::errc::invalid_argument );
			goto exit;
		}

		if ( length > 0 )
		{
			::memset( m_data, value, length );
		}

	exit:
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return *this;
	}

	mutable_buffer&
	fill( position_type offset, size_type length, byte_type value )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		if ( offset + length > m_capacity )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}

		if ( length > 0 )
		{
			::memset( m_data, value, length );
		}

		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return *this;
	}

	mutable_buffer&
	fill( byte_type value )
	{
		return fill( 0, m_capacity, value );
	}

	mutable_buffer&
	put( position_type offset, byte_type value )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		if ( offset >= m_capacity )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}
		*( m_data + offset ) = value;
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return *this;
	}

	mutable_buffer&
	put( position_type offset, byte_type value, std::error_code& err )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		err.clear();

		if ( offset >= m_capacity )
		{
			err = make_error_code( std::errc::invalid_argument );
			goto exit;
		}

		*( m_data + offset ) = value;

	exit:
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return *this;
	}

	mutable_buffer&
	putn( position_type offset, const void* src, size_type length )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		if ( length > 0 )
		{
			if ( ! src || offset + length > m_capacity )
			{
				throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
			}
			::memcpy( m_data + offset, src, length );
		}
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return *this;
	}

	mutable_buffer&
	putn( position_type offset, const void* src, size_type length, std::error_code& err )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		err.clear();

		if ( length > 0 )
		{
			if ( ! src || offset + length > m_capacity )
			{
				err = make_error_code( std::errc::invalid_argument );
				goto exit;
			}
			::memcpy( m_data + offset, src, length );
		}

	exit:
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return *this;
	}

	mutable_buffer&
	putn( position_type offset, std::string const& s, std::error_code& err )
	{
		return putn( offset, s.data(), s.size(), err );
	}

	mutable_buffer&
	putn( position_type offset, std::string const& s )
	{
		return putn( offset, s.data(), s.size() );
	}

	mutable_buffer&
	putn( position_type offset, const char *s, std::error_code& err )
	{
		return putn( offset, s, ::strlen( s ), err );
	}

	mutable_buffer&
	putn( position_type offset, const char *s )
	{
		return putn( offset, s, ::strlen( s ) );
	}


protected:

	allocation::uptr	m_alloc;
	size_type			m_capacity;
};

class const_buffer : public buffer
{
protected:
	buffer::allocation::uptr	m_alloc;

public:

	friend class shared_buffer;

	const_buffer()
	:
	m_alloc{ std::make_unique< allocation >( null_broker::get() ) }
	{
		m_data = nullptr;
		m_size = 0;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
	}

	// const_buffer( const void* data, size_type size )
	// :
	// m_alloc{ std::make_unique< allocation >( reinterpret_cast< const byte_type* >( data ), size ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = size;
	// }

	template< class Dealloc, class = std::enable_if_t< std::is_convertible_v< Dealloc, deallocator > > >
	const_buffer( void* data, size_type size, Dealloc&& dealloc_f )
	:
	m_alloc{ std::make_unique< buffer::allocation >( 
		reinterpret_cast< byte_type* >( data ), size, 
		create_broker( std::forward< Dealloc >( dealloc_f ) ) ) }
	{
		m_data = reinterpret_cast< byte_type* >( data );
		m_size = size;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
	}

	const_buffer( const void* data, size_type size, memory_broker::ptr broker = buffer::default_broker::get() )
	:
	m_alloc{ std::make_unique< allocation >( reinterpret_cast< const byte_type* >( data ), size, broker ) }
	{
		m_data = m_alloc->data();
		m_size = size;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
	}

	const_buffer( buffer const& rhs, memory_broker::ptr broker = buffer::default_broker::get() )
	:
	m_alloc{ std::make_unique< allocation >( rhs.m_data, rhs.m_size, broker ) }
	{
		m_data = m_alloc->data();
		m_size = rhs.m_size;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
		// ASSERT_CONST_BUFFER_INVARIANTS( rhs );
	}

	const_buffer( const_buffer const& rhs, memory_broker::ptr broker = buffer::default_broker::get() )
	:
	m_alloc{ std::make_unique< allocation >( rhs.m_data, rhs.m_size, broker ) }
	{
		m_data = m_alloc->data();
		m_size = rhs.m_size;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
		ASSERT_CONST_BUFFER_INVARIANTS( rhs );
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

	const_buffer( const_buffer&& rhs )
	:
	m_alloc{ std::move( rhs.m_alloc ) } 
	{
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
	}

	// const_buffer( mutable_buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	// :
	// m_alloc{ std::make_shared< allocation >( rhs.data(), rhs.size(), broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.size();
	// }

	const_buffer( mutable_buffer&& rhs )
	:
	m_alloc{ std::move( rhs.m_alloc ) }
	{
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		rhs.m_capacity = 0;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
	}

	const_buffer( buffer const& rhs, position_type offset, size_type length, memory_broker::ptr broker = default_broker::get() )
	:
	m_alloc{ std::make_unique< allocation >( broker ) }
	{
		// ASSERT_BUFFER_INVARIANTS( buf );

		if ( offset + length > rhs.m_size )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}
		m_data = m_alloc->allocate( length );
		::memcpy( m_data, rhs.m_data + offset, length );
		m_size = length;

		ASSERT_CONST_BUFFER_INVARIANTS( *this );
	}

	const_buffer( buffer const& rhs, position_type offset, size_type length, std::error_code& err, memory_broker::ptr broker = default_broker::get() )
	:
	m_alloc{ std::make_unique< allocation >( broker ) }
	{
		// ASSERT_BUFFER_INVARIANTS( buf );
		err.clear();
		if ( offset + length > rhs.m_size )
		{
			err = make_error_code( std::errc::invalid_argument );
			goto exit;
		}
		m_data = m_alloc->allocate( length );
		::memcpy( m_data, rhs.m_data + offset, length );
		m_size = length;

		ASSERT_CONST_BUFFER_INVARIANTS( *this );
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

	const_buffer
	slice( position_type offset, size_type length, memory_broker::ptr broker = default_broker::get() ) const
	{
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
		return const_buffer{ *this, offset, length, broker };
	}

	const_buffer
	slice( position_type offset, size_type length, std::error_code& err, memory_broker::ptr broker = default_broker::get() ) const
	{
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
		return const_buffer{ *this, offset, length, err, broker };
	}

	const_buffer&
	operator=( const_buffer const& rhs )
	{
		ASSERT_CONST_BUFFER_INVARIANTS( rhs );
		m_alloc = std::make_unique< allocation >( rhs.m_data, rhs.m_size, default_broker::get() );
		m_data = m_alloc->data();
		m_size = rhs.m_size;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
		return *this;
	}

	const_buffer&
	operator=( mutable_buffer const& rhs )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( rhs );
		m_alloc = std::make_unique< allocation >( rhs.m_data, rhs.m_size, default_broker::get() );
		m_data = m_alloc->data();
		m_size = rhs.m_size;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
		return *this;
	}

	const_buffer&
	operator=( const_buffer&& rhs )
	{
		ASSERT_CONST_BUFFER_INVARIANTS( rhs );
		m_alloc = std::move( rhs.m_alloc );
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
		return *this;
	}

	// shared_buffer&
	// operator=( mutable_buffer const& rhs )
	// {
	// 	m_alloc = std::make_shared< allocation >( rhs.data(), rhs.size() );
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.size();
	// 	// ASSERT_MUTABLE_BUFFER_INVARIANTS( buf );
	// 	return *this;
	// }

	const_buffer&
	operator=( mutable_buffer&& rhs )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( rhs );
		m_alloc = std::move( rhs.m_alloc );
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		rhs.m_capacity = 0;
		ASSERT_CONST_BUFFER_INVARIANTS( *this );
		return *this;
	}

};

class shared_buffer  : public buffer
{
protected:

	allocation::sptr	m_alloc;

public:

	shared_buffer()
	:
	m_alloc{ std::make_shared< allocation >( null_broker::get() ) }
	{
		m_data = nullptr;
		m_size = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	// shared_buffer( const void* data, size_type size )
	// :
	// m_alloc{ std::make_shared< allocation >( reinterpret_cast< const byte_type* >( data ), size ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = size;
	// }

	template< class Dealloc, class = std::enable_if_t< std::is_convertible_v< Dealloc, deallocator > > >
	shared_buffer( void* data, size_type size, Dealloc&& dealloc_f )
	:
	m_alloc{ std::make_shared< buffer::allocation >( 
		reinterpret_cast< byte_type* >( data ), size, 
		create_broker( std::forward< Dealloc >( dealloc_f ) ) ) }
	{
		m_data = reinterpret_cast< byte_type* >( data );
		m_size = size;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	shared_buffer( const void* data, size_type size, memory_broker::ptr broker = default_broker::get() )
	:
	m_alloc{ std::make_shared< allocation >( reinterpret_cast< const byte_type* >( data ), size, broker ) }
	{
		m_data = m_alloc->data();
		m_size = size;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	shared_buffer( shared_buffer const& rhs )
	:
	m_alloc{ rhs.m_alloc }
	{
		ASSERT_CONST_BUFFER_INVARIANTS( rhs );
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	shared_buffer( shared_buffer&& rhs )
	:
	m_alloc{ std::move( rhs.m_alloc ) } 
	{
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	shared_buffer( buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	:
	m_alloc{ std::make_shared< allocation >( rhs.data(), rhs.size(), broker ) }
	{
		m_data = m_alloc->data();
		m_size = rhs.size();
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	// shared_buffer( mutable_buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	// :
	// m_alloc{ std::make_shared< allocation >( rhs.data(), rhs.size(), broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.size();
	// }

	// shared_buffer( const_buffer const& rhs, memory_broker::ptr broker = default_broker::get() )
	// :
	// m_alloc{ std::make_shared< allocation >( rhs.data(), rhs.size(), broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.size();
	// }

	shared_buffer( mutable_buffer&& rhs )
	:
	m_alloc{ std::move( rhs.m_alloc ) }
	{
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		rhs.m_capacity = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	shared_buffer( const_buffer&& rhs )
	:
	m_alloc{ std::move( rhs.m_alloc ) }
	{
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	shared_buffer( shared_buffer const& rhs, position_type offset, size_type length )
	:
	m_alloc{ rhs.m_alloc }
	{
		ASSERT_SHARED_BUFFER_INVARIANTS( rhs );
		if ( offset + length > rhs.m_size )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}
		m_data = rhs.m_data + offset;
		m_size = length;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	shared_buffer( shared_buffer const& rhs, position_type offset, size_type length, std::error_code& err )
	:
	m_alloc{ rhs.m_alloc }
	{
		err.clear();
		ASSERT_SHARED_BUFFER_INVARIANTS( rhs );
		if ( offset + length > rhs.m_size )
		{
			err = make_error_code( std::errc::invalid_argument );
			goto exit;
		}
		m_data = rhs.m_data + offset;
		m_size = length;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	exit:
		return;
	}

	shared_buffer( buffer const& rhs, position_type offset, size_type length, memory_broker::ptr broker = default_broker::get() )
	:
	m_alloc{ std::make_unique< allocation >( broker ) }
	{
		if ( offset + length > rhs.size() )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}
		m_data = m_alloc->allocate( length );
		::memcpy( m_data, rhs.data() + offset, length );
		m_size = length;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
	}

	// shared_buffer( buffer const& rhs, memory_broker::ptr broker )
	// :
	// m_alloc{ std::make_shared< allocation >( rhs.m_data, rhs.m_size, broker ) }
	// {
	// 	m_data = m_alloc->data();
	// 	m_size = rhs.m_size;
	// }

	shared_buffer
	slice( position_type offset, size_type length ) const
	{
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
		return shared_buffer{ *this, offset, length };
	}

	shared_buffer
	slice( position_type offset, size_type length, std::error_code& err ) const
	{
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
		return shared_buffer{ *this, offset, length, err };
	}

	shared_buffer&
	operator=( shared_buffer const& rhs )
	{
		ASSERT_SHARED_BUFFER_INVARIANTS( rhs );
		m_alloc = rhs.m_alloc;
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
		return *this;
	}

	shared_buffer&
	operator=( shared_buffer&& rhs )
	{
		ASSERT_SHARED_BUFFER_INVARIANTS( rhs );
		m_alloc = std::move( rhs.m_alloc );
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
		return *this;
	}

	shared_buffer&
	operator=( buffer const& rhs )
	{
		m_alloc = std::make_shared< allocation >( rhs.data(), rhs.size(), default_broker::get() );
		m_data = m_alloc->data();
		m_size = rhs.size();
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
		return *this;
	}

	shared_buffer&
	operator=( mutable_buffer&& rhs )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( rhs );
		m_alloc = std::move( rhs.m_alloc );
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		rhs.m_capacity = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
		return *this;
	}

	shared_buffer&
	operator=( const_buffer&& rhs )
	{
		ASSERT_CONST_BUFFER_INVARIANTS( rhs );
		m_alloc = std::move( rhs.m_alloc );
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
		return *this;
	}

	std::size_t
	ref_count() const
	{
		ASSERT_SHARED_BUFFER_INVARIANTS( *this );
		return m_alloc.use_count();
	}
};

class string_alias
{
	public:
	string_alias() : m_buf{} {}

	string_alias( shared_buffer const& buf) : m_buf{ buf } {}

	string_alias( shared_buffer const& buf, std::size_t offset, std::size_t size ) : m_buf{ buf.slice( offset, size ) } {}

	string_alias( string_alias const& rhs ) : m_buf{ rhs.m_buf } {}

	string_alias( string_alias&& rhs ) : m_buf{ std::move( rhs.m_buf ) } {}

	string_alias&
	operator=( string_alias const& rhs )
	{
		if ( this != &rhs ) 
		{ 
			m_buf = rhs.m_buf; 
		}
		return *this;
	}

	string_alias&
	operator=( string_alias&& rhs )
	{
		if ( this != &rhs ) 
		{ 
			m_buf = std::move( rhs.m_buf );
		}
		return *this;
	}

	operator std::string_view() const noexcept
	{
		return std::string_view{ reinterpret_cast< const char* > ( m_buf.data() ), m_buf.size() };
	}

	std::string_view
	view() const noexcept
	{
		return std::string_view{ reinterpret_cast< const char* >( m_buf.data() ), m_buf.size() };
	}

private:
	shared_buffer m_buf;
};

#if 1
// Ghetto streambuf to provide support for msgpack::packer and unpacker

class bufwriter
{
	public:
	bufwriter( std::size_t size ) : m_buf{ size }, m_pos{ 0 } {}

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
	putn( const void* src, std::size_t n )
	{
		accommodate( n );
		m_buf.putn( m_pos, src, n );
		m_pos += n;
	}

	void
	put( std::uint8_t b )
	{
		accommodate( 1 );
		m_buf.put( m_pos, b );
		++m_pos;
	}

	std::uint8_t*
	accommodate( std::size_t n )
	{
		auto remaining = m_buf.capacity() - m_pos;
		if (n > remaining)
		{
			auto required = m_pos + n;
			auto cushioned_size = (3 * required) / 2;
			m_buf.expand( cushioned_size );
		}
		return m_buf.data() + m_pos;
	}

	void
	advance( std::size_t n )
	{
		m_pos += n;
	}

	const_buffer
	get_buffer()
	{
		m_buf.size( m_pos );
		return const_buffer{ m_buf };
	}

	void
	write( const char* src, std::size_t len )
	{
		putn(src, len);
	}

private:

	mutable_buffer m_buf;
	std::size_t m_pos;
};

#endif


} // namespace bstream
} // namespace logicmill


#endif    // LOGICMILL_BSTREAM_BUFFER_H
