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

#include <cstdint>
#include <functional>
#include <limits>
#include <system_error>
#include <assert.h>

#ifndef NDEBUG

#define ASSERT_MUTABLE_BUFFER_INVARIANTS( _buf_ )															\
{																											\
	if ( ( _buf_ ).m_alloc_ctrl != nullptr )																\
	{																										\
		assert( ( _buf_ ).m_data == ( _buf_ ).m_alloc_ctrl->data );											\
		assert( ( _buf_ ).m_size <= ( _buf_ ).m_alloc_ctrl->capacity );										\
		assert( ( _buf_ ).m_capacity == ( _buf_ ).m_alloc_ctrl->capacity );									\
		assert( ( _buf_ ).m_alloc_ctrl->ref_count == 1 );													\
	}																										\
	else																									\
	{																										\
		assert( ( _buf_ ).m_data == nullptr );																\
		assert( ( _buf_ ).m_capacity == 0 );																\
		assert( ( _buf_ ).m_size == 0 );																	\
	}																										\
}																											\
/**/

#define ASSERT_BUFFER_INVARIANTS( _buf_ )																	\
{																											\
	if ( ( _buf_ ).m_alloc_ctrl != nullptr )																\
	{																										\
		assert( ( _buf_ ).m_data >= ( _buf_ ).m_alloc_ctrl->data );											\
		assert( ( _buf_ ).m_data < ( ( _buf_ ).m_alloc_ctrl->data + ( _buf_ ).m_alloc_ctrl->capacity ) );	\
		assert( ( ( _buf_ ).m_data + ( _buf_ ).m_size )														\
				<= ( ( _buf_ ).m_alloc_ctrl->data + ( _buf_ ).m_alloc_ctrl->capacity ) );					\
		assert( ( _buf_ ).m_alloc_ctrl->ref_count >= 1 );													\
	}																										\
	else																									\
	{																										\
		assert( ( _buf_ ).m_data == nullptr );																\
		assert( ( _buf_ ).m_size == 0 );																	\
	}																										\
}																											\
/**/

#else

#define ASSERT_MUTABLE_BUFFER_INVARIANTS( _buf_ )

#define ASSERT_BUFFER_INVARIANTS( _buf_ )

#endif

namespace logicmill
{

class buffer
{
public:

	using elem_type = std::uint8_t;
	using size_type = std::size_t;
	using position_type = std::uint64_t;
	using offset_type = std::int64_t;
	using checksum_type = std::uint32_t;

	static const size_type npos = std::numeric_limits< size_type >::max();

	using deallocator_func = std::function< void( elem_type* data ) >;
	using allocator_func = std::function< elem_type*( elem_type* data, size_type current_size, size_type new_size ) >;

	static deallocator_func default_deallocator;
	static allocator_func default_allocator;

	friend class const_buffer;

	const elem_type*
	data() const
	{
		return m_data;
	}

	size_type
	size() const
	{
		return m_size;
	}

	std::size_t
	ref_count() const
	{
		if ( m_alloc_ctrl )
		{
			return m_alloc_ctrl->ref_count;
		}
		else
		{
			return 0;
		}
	}

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

	bool
	empty() const
	{
		return m_size == 0;
	}

	checksum_type
	checksum() const;

	void
	dump( std::ostream& os ) const;


protected:

	void
	static clear_error( std::error_code& err )
	{
		static const std::error_code no_error;
		err = no_error;
	}

	struct adopt_memory {};
	struct copy_memory {};

	struct alloc_ctrl
	{
		alloc_ctrl()
		:
		data{ nullptr },
		capacity{ 0 },
		ref_count{ 1 },
		deallocator{ default_deallocator },
		allocator{ default_allocator }
		{}

		alloc_ctrl( adopt_memory, void* src, size_type cap, deallocator_func dealloc, allocator_func alloc )
		:
		data{ ( ( cap > 0 ) ? static_cast< elem_type* >( src ) : nullptr ) },
		capacity{ ( data ? cap : 0 ) },
		ref_count{ 1 },
		deallocator{ dealloc },
		allocator{ alloc }
		{}

		alloc_ctrl( copy_memory, const void* src, size_type cap, deallocator_func dealloc, allocator_func alloc )
		:
		data{ ( ( cap > 0 ) ? alloc( nullptr, 0, cap ) : nullptr ) },
		capacity{ cap },
		ref_count{ 1 },
		deallocator{ dealloc },
		allocator{ alloc }
		{
			if ( src && data && cap > 0 )
			{
				::memcpy( data, src, cap );
			}
		}

		alloc_ctrl( size_type cap, deallocator_func dealloc, allocator_func alloc )
		: 
		data{ ( cap > 0 ) ? alloc( nullptr, 0, cap ) : nullptr },
		capacity{ cap },
		ref_count{ 1 },
		deallocator{ dealloc },
		allocator{ alloc }
		{}

		alloc_ctrl( alloc_ctrl const& rhs ) = delete;

		alloc_ctrl( alloc_ctrl&& rhs ) = delete;

		~alloc_ctrl()
		{
			assert( ref_count == 0 );

			if ( data )
			{
				if ( deallocator )
				{
					deallocator( data );
				}
			}
		}

		void
		reallocate( size_type new_cap )
		{
			if ( ! allocator )
			{
				throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
			}

			data = allocator( data, capacity, new_cap ) ;
			if ( data )
			{
				capacity = new_cap;
			}
			else
			{
				capacity = 0;
				throw std::system_error{ make_error_code( std::errc::no_buffer_space ) };
			}
		}

		void
		reallocate( size_type new_cap, std::error_code& err )
		{
			clear_error( err );
			if ( ! allocator )
			{
				err =  make_error_code( std::errc::operation_not_supported );
				goto exit;
			}

			data = allocator( data, capacity,  new_cap ) ;

			if ( data )
			{
				capacity = new_cap;
			}
			else
			{
				capacity = 0;
				err = make_error_code( std::errc::no_buffer_space );
				goto exit;
			}

		exit:
			return;
		}

		void
		reallocate( size_type new_cap, size_type preserve )
		{
			if ( ! allocator )
			{
				throw std::system_error{ make_error_code( std::errc::operation_not_supported ) };
			}

			data = allocator( data, preserve, new_cap ) ;
			if ( data )
			{
				capacity = new_cap;
			}
			else
			{
				capacity = 0;
				throw std::system_error{ make_error_code( std::errc::no_buffer_space ) };
			}
		}

		void
		reallocate( size_type new_cap, size_type preserve, std::error_code& err )
		{
			clear_error( err );
			if ( ! allocator )
			{
				err =  make_error_code( std::errc::operation_not_supported );
				goto exit;
			}

			data = allocator( data, preserve,  new_cap ) ;

			if ( data )
			{
				capacity = new_cap;
			}
			else
			{
				capacity = 0;
				err = make_error_code( std::errc::no_buffer_space );
				goto exit;
			}

		exit:
			return;
		}

		elem_type* 			data;
		size_type 			capacity;
		std::size_t 		ref_count;
		deallocator_func	deallocator;
		allocator_func		allocator;
	};

public:

	~buffer()
	{
		ASSERT_BUFFER_INVARIANTS( *this );
		if ( m_alloc_ctrl )
		{
			assert( m_alloc_ctrl->ref_count > 0 );

			if ( --( m_alloc_ctrl->ref_count ) == 0 )
			{
				delete m_alloc_ctrl;
			}
			m_alloc_ctrl = nullptr;
			m_data = nullptr;
			m_size = 0;
		}
	}

protected:

	buffer()
	:
	m_alloc_ctrl{ nullptr },
	m_data{ nullptr },
	m_size{ 0 }
	{}

	buffer( alloc_ctrl* ctrl, elem_type* data, size_type size ) 
	:
	m_alloc_ctrl{ ctrl },
	m_data{ data },
	m_size{ size }
	{
		ASSERT_BUFFER_INVARIANTS( *this );
	}

	buffer( alloc_ctrl* ctrl ) 
	:
	m_alloc_ctrl{ ctrl },
	m_data{ m_alloc_ctrl ? m_alloc_ctrl->data : nullptr },
	m_size{ m_alloc_ctrl ? m_alloc_ctrl->capacity : 0 }
	{
		ASSERT_BUFFER_INVARIANTS( *this );
	}

	buffer( buffer&& rhs )
	:
	m_alloc_ctrl{ nullptr },
	m_data{ nullptr },
	m_size{ 0 }
	{
		swap( rhs );
		ASSERT_BUFFER_INVARIANTS( *this );
	}

	void
	swap( buffer& rhs )
	{
		ASSERT_BUFFER_INVARIANTS( *this );
		ASSERT_BUFFER_INVARIANTS( rhs );
		alloc_ctrl *tmp_alloc_ctrl = m_alloc_ctrl;
		elem_type *tmp_data = m_data;
		size_type tmp_size = m_size;
		m_alloc_ctrl = rhs.m_alloc_ctrl;
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_alloc_ctrl = tmp_alloc_ctrl;
		rhs.m_data = tmp_data;
		rhs.m_size = tmp_size;
	}

	void
	disown()
	{
		ASSERT_BUFFER_INVARIANTS( *this );
		if ( m_alloc_ctrl )
		{
			assert( m_alloc_ctrl->ref_count > 0 );

			if ( --( m_alloc_ctrl->ref_count ) == 0 )
			{
				delete m_alloc_ctrl;
			}
			m_alloc_ctrl = nullptr;
			m_data = nullptr;
			m_size = 0;
		}
		ASSERT_BUFFER_INVARIANTS( *this );
	}

	void
	adopt( alloc_ctrl* other )
	{
		m_alloc_ctrl = other;
		if ( m_alloc_ctrl )
		{
			++( m_alloc_ctrl->ref_count );
		}
	}

	void
	assign_by_adoption( buffer const& rhs )
	{
		ASSERT_BUFFER_INVARIANTS( *this );
		ASSERT_BUFFER_INVARIANTS( rhs );

		disown();
		adopt( rhs.m_alloc_ctrl );
		m_data = rhs.m_data;
		m_size = rhs.m_size;

		ASSERT_BUFFER_INVARIANTS( *this )
		ASSERT_BUFFER_INVARIANTS( rhs );
	}

	void
	assign_by_move( buffer&& rhs )
	{
		ASSERT_BUFFER_INVARIANTS( *this );
		ASSERT_BUFFER_INVARIANTS( rhs );

		disown();
		swap( rhs );

		ASSERT_BUFFER_INVARIANTS( *this );
		ASSERT_BUFFER_INVARIANTS( rhs );
	}

	void
	assign_by_copy( buffer const& buf )
	{
		ASSERT_BUFFER_INVARIANTS( *this )
		ASSERT_BUFFER_INVARIANTS( buf )

		disown();
		m_alloc_ctrl = new alloc_ctrl{ copy_memory{}, buf.m_data, buf.m_size, default_deallocator, default_allocator };
		m_data = m_alloc_ctrl->data;
		m_size = buf.m_size;

		ASSERT_BUFFER_INVARIANTS( *this )
	}

	alloc_ctrl*				m_alloc_ctrl;
	elem_type*				m_data;
	size_type				m_size;

};

class mutable_buffer : public buffer
{
public:

	using buffer::data;
	using buffer::size;

	friend class const_buffer;

	mutable_buffer( size_type cap, deallocator_func dealloc = default_deallocator, allocator_func alloc = default_allocator )
	: 
	buffer{ new alloc_ctrl{ cap, dealloc, alloc } },
	m_capacity{ ( m_alloc_ctrl ? m_alloc_ctrl->capacity : 0 ) }
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	mutable_buffer( void* data, size_type size, deallocator_func dealloc, allocator_func alloc )
	:
	buffer{ new alloc_ctrl{ adopt_memory{}, static_cast< elem_type* >( data ), size, dealloc, alloc } },
	m_capacity{ ( m_alloc_ctrl ? m_alloc_ctrl->capacity : 0 ) }
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	mutable_buffer( std::string const& str )
	:
	buffer{ new alloc_ctrl{ copy_memory{}, reinterpret_cast< const elem_type* >( str.data() ), str.size(), default_deallocator, default_allocator } },
	m_capacity{ ( m_alloc_ctrl ? m_alloc_ctrl->capacity : 0 ) }
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	mutable_buffer( const char*  str )
	:
	buffer{ new alloc_ctrl{ copy_memory{}, reinterpret_cast< const elem_type* >( str ), strlen( str ), default_deallocator, default_allocator } },
	m_capacity{ ( m_alloc_ctrl ? m_alloc_ctrl->capacity : 0 ) }
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	mutable_buffer()
	:
	buffer{ /* new alloc_ctrl{} */ },
	m_capacity{ ( m_alloc_ctrl ? m_alloc_ctrl->capacity : 0 ) }
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	mutable_buffer( mutable_buffer&& rhs )
	: 
	buffer{ std::move( rhs ) },
	m_capacity{ ( m_alloc_ctrl ? m_alloc_ctrl->capacity : 0 ) }
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	mutable_buffer( mutable_buffer const& ) = delete;

	mutable_buffer&
	operator=( mutable_buffer&& rhs )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		ASSERT_MUTABLE_BUFFER_INVARIANTS( rhs );

		assign_by_move( std::move( rhs ) );
		m_capacity = m_alloc_ctrl ? m_alloc_ctrl->capacity : 0;

		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );

		return *this;
	}

	mutable_buffer&
	operator=( mutable_buffer const& ) = delete;

	elem_type*
	data()
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return m_data;
	}

	void
	size(size_type new_size)
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		if (new_size > m_capacity ) { throw std::system_error{make_error_code(std::errc::invalid_argument)}; }
		m_size = new_size;
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
	}

	size_type
	capacity() const
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );
		return m_capacity;
	}

	mutable_buffer&
	capacity( size_type new_cap, std::error_code& err )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );

		clear_error( err );

		if ( new_cap > m_capacity )
		{
			if ( m_alloc_ctrl )
			{
				m_alloc_ctrl->reallocate( new_cap, m_size, err );
				if ( err ) goto exit;
			}
			else
			{
				assert( m_size == 0 && m_capacity == 0 );
				m_alloc_ctrl = new alloc_ctrl{ new_cap, default_deallocator, default_allocator };
			}
			m_data = m_alloc_ctrl ? m_alloc_ctrl->data : nullptr;
			m_capacity = m_alloc_ctrl ? m_alloc_ctrl->capacity : 0;
		}

		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );

	exit:
		return *this;
	}

	mutable_buffer&
	capacity( size_type new_cap )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );

		if ( new_cap > capacity() )
		{
			if ( m_alloc_ctrl )
			{
				m_alloc_ctrl->reallocate( new_cap, m_size );
			}
			else
			{
				assert( m_size == 0 && m_capacity == 0 );
				m_alloc_ctrl = new alloc_ctrl{ new_cap, default_deallocator, default_allocator };
			}
			m_data = m_alloc_ctrl ? m_alloc_ctrl->data : nullptr;
			m_capacity = m_alloc_ctrl ? m_alloc_ctrl->capacity : 0;
		}

		ASSERT_MUTABLE_BUFFER_INVARIANTS( *this );

		return *this;
	}

	mutable_buffer&
	fill(elem_type value = 0)
	{
		if ( m_alloc_ctrl && m_data && m_size > 0 )
		{
			::memset( m_data, value, m_size );
		}
		return *this;
	}

	mutable_buffer&
	put( position_type offset, elem_type value )
	{
		if ( ! m_alloc_ctrl || ! m_data || ( offset >= m_capacity ) )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}
		*( m_data + offset ) = value;
		return *this;
	}

	mutable_buffer&
	put( position_type offset, elem_type value, std::error_code& err )
	{
		clear_error( err );

		if ( ! m_alloc_ctrl || ! m_data || ( offset >= m_capacity ) )
		{
			err = make_error_code( std::errc::invalid_argument );
			goto exit;
		}

		*( m_data + offset ) = value;

	exit:
		return *this;
	}

	mutable_buffer&
	putn( position_type offset, const void* src, size_type length )
	{
		if ( length > 0 )
		{
			if ( ! src || ! m_alloc_ctrl || ! m_data || ( ( offset + length ) > m_capacity ) )
			{
				throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
			}
			::memcpy( m_data + offset, src, length );
		}
		return *this;
	}

	mutable_buffer&
	putn( position_type offset, const void* src, size_type length, std::error_code& err )
	{
		clear_error( err );

		if ( length > 0 )
		{
			if ( ! src || ! m_alloc_ctrl || ! m_data || ( ( offset + length ) > m_capacity ) )
			{
				err = make_error_code( std::errc::invalid_argument );
				goto exit;
			}
			::memcpy( m_data + offset, src, length );
		}

	exit:
		return *this;
	}

private:

	size_type		m_capacity;
};

class const_buffer  : public buffer
{
public:

	const_buffer()
	:
	buffer{}
	{}

	const_buffer( const void* data, size_type size )
	:
	buffer{ new alloc_ctrl{ copy_memory{}, data, size, default_deallocator, default_allocator } }
	{}

	const_buffer( const void* data, size_type size, deallocator_func dealloc )
	:
	buffer{ new alloc_ctrl { adopt_memory{}, const_cast< void* >( data ), size, dealloc, nullptr } }
	{}

	const_buffer( const_buffer const& rhs )
	:
	buffer{ rhs.m_alloc_ctrl, rhs.m_data, rhs.m_size }
	{
		ASSERT_BUFFER_INVARIANTS( rhs );

		if ( m_alloc_ctrl )
		{
			++( m_alloc_ctrl->ref_count );
			assert( m_alloc_ctrl->ref_count > 1 );
		}

		ASSERT_BUFFER_INVARIANTS( *this );
		ASSERT_BUFFER_INVARIANTS( rhs );
	}

	const_buffer( const_buffer&& rhs )
	: 
	buffer{ std::move( rhs ) }
	{}

	const_buffer( mutable_buffer const& rhs )
	:
	buffer{ new alloc_ctrl{ copy_memory{}, rhs.m_data, rhs.m_size, default_deallocator, default_allocator } }
	{
		ASSERT_BUFFER_INVARIANTS( *this );
		ASSERT_MUTABLE_BUFFER_INVARIANTS( rhs );
	}

	const_buffer( mutable_buffer&& rhs )
	:
	buffer{ std::move( rhs ) }
	{}

	const_buffer( const_buffer const& buf, position_type offset, size_type length )
	:
	buffer{}
	{
		ASSERT_BUFFER_INVARIANTS( buf );

		if ( ! buf.m_alloc_ctrl || offset + length > buf.m_size || length == 0 )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}
		adopt( buf.m_alloc_ctrl );
		m_data = buf.m_data + offset;
		m_size = length;

		ASSERT_BUFFER_INVARIANTS( *this );
	}

	const_buffer( mutable_buffer const& buf, position_type offset, size_type length )
	:
	buffer{}
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( buf );

		if ( ! buf.m_alloc_ctrl || offset + length > buf.m_size || length == 0 )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}

		m_alloc_ctrl = new alloc_ctrl{ copy_memory{}, buf.m_data + offset, length, default_deallocator, default_allocator };
		m_data = m_alloc_ctrl->data;
		m_size = m_alloc_ctrl->capacity;

		ASSERT_BUFFER_INVARIANTS( *this );
	}

	const_buffer( buffer const& rhs )
	:
	buffer{ new alloc_ctrl{ copy_memory{}, rhs.m_data, rhs.m_size, default_deallocator, default_allocator } }
	{
		ASSERT_BUFFER_INVARIANTS( *this );
		ASSERT_BUFFER_INVARIANTS( rhs );
	}


	const_buffer
	slice( position_type offset, size_type length ) const
	{
		ASSERT_BUFFER_INVARIANTS( *this );
		return const_buffer{ *this, offset, length };
	}

	const_buffer&
	operator=( const_buffer const& rhs )
	{
		assign_by_adoption( rhs );
		return *this;
	}

	const_buffer&
	operator=( const_buffer&& rhs )
	{
		assign_by_move( std::move( rhs ) );
		return *this;
	}

	const_buffer&
	operator=( mutable_buffer const& buf )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( buf );

		assign_by_copy( buf );
		return *this;
	}

	const_buffer&
	operator=( mutable_buffer&& buf )
	{
		ASSERT_MUTABLE_BUFFER_INVARIANTS( buf );

		assign_by_move( std::move( buf ) );
		return *this;
	}
};

class string_alias
{
	public:
	string_alias() : m_buf{} {}

	string_alias(const_buffer const& buf) : m_buf{ buf } {}

	string_alias( const_buffer const& buf, std::size_t offset, std::size_t size ) : m_buf{ buf.slice( offset, size ) } {}

	string_alias( string_alias const& rhs ) : m_buf{ rhs.m_buf } {}

	string_alias( string_alias&& rhs ) : m_buf{ std::move( rhs.m_buf ) } {}

	string_alias&
	operator=( string_alias const& rhs )
	{
		if ( this != &rhs ) { m_buf = rhs.m_buf; }
		return *this;
	}

	string_alias&
	operator=( string_alias&& rhs )
	{
		if ( this != &rhs ) { m_buf = std::move( rhs.m_buf ); }
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
	const_buffer m_buf;
};

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
			m_buf.capacity( cushioned_size );
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

}    // namespace logicmill

#endif    // LOGICMILL_BUFFER_H
