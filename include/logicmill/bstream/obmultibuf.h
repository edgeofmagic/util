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

#ifndef LOGICMILL_BSTREAM_OBMULTIBUF_H
#define LOGICMILL_BSTREAM_OBMULTIBUF_H

#include <logicmill/bstream/obstreambuf.h>
#include <logicmill/buffer.h>

#ifndef LOGICMILL_BSTREAM_DEFAULT_OBMULTIBUF_SIZE
#define LOGICMILL_BSTREAM_DEFAULT_OBMULTIBUF_SIZE  65536UL
#endif

namespace logicmill 
{
namespace bstream 
{

class obmultibuf : public obstreambuf
{
public:

	using buffer_list = std::deque< mutable_buffer >

    obmultibuf( size_type size = LOGICMILL_BSTREAM_DEFAULT_OBMULTIBUF_SIZE )
    {
		// TODO: impose minimum size ?
		m_bufs.emplace_back( mutable_buffer{ size } );
		m_offsets.push_back( 0 );
		m_current_buf = 0;
		m_alloc_size = size;
        reset_ptrs();
    }

    obmultibuf( mutable_buffer&& buf )
    {
		size_type cap = buf.capacity();
		m_bufs.emplace_back( std::move( buf ) );
		m_offsets.push_back( 0 );
		m_current_buf = 0;
		m_alloc_size = cap;
        reset_ptrs();
    }

    obmultibuf( obmultibuf&& rhs )
	:
	obstreambuf{ std::move( rhs ) },
	m_bufs{ std::move( rhs.m_bufs ) },
	m_offsets{ std::move( rhs.m_offsets ) },
	m_current_buf{ rhs.m_current_buf },
	m_alloc_size{ rhs.m_alloc_size }
	{}

	obmultibuf( buffer_list&& bufs )
	:
	m_bufs{ std::move( bufs ) }
	{
		m_current_buf = 0;
		size_type total_size = 0;
		size_type max_cap = 0;
		for ( auto i = 0; i < m_bufs.size(); ++i )
		{
			m_offsets.push_back( total_size );
			total_size += m_bufs[ i ].size();
			size_type cap = m_bufs[ i ].capacity();
			if ( cap > max_cap )
			{
				max_cap = cap;
			}
		}
		m_alloc_size = max_cap; // TODO: impose minimum alloc_size?
		reset_ptrs();
	}

    obmultibuf( obmultibuf const& ) = delete;
    obmultibuf& operator=( obmultibuf&& ) = delete;
    obmultibuf& operator=( obmultibuf const& ) = delete;
    
	obmultibuf& 
	clear() noexcept;

	buffer_list&
	get_buffers();

	std::deque< position_type >&
	get_offsets()

	buffer_list
	release_buffer_list();

	std::deque< position_type >
	release_offsets();

protected:

	virtual bool
	is_valid_position( position_type pos ) const override
	{
		return pos >= 0;
	}

	virtual void
	really_jump( std::error_code& err ) override;

    virtual void
    really_flush( std::error_code& err ) override;

    virtual void
    really_overflow( size_type n, std::error_code& err ) override;

    void
    reset_ptrs()
    {
		assert( m_current_buf < m_bufs.size() );

		m_pbase_offset = m_offsets[ m_current_buf ];
		auto& buf = m_bufs[ m_current_buf ];
        auto base = buf.data();
		if ( m_current_buf == m_bufs.size() - 1 )
		{
			// the last buffer in list, use capacity, not size !! PROBABLY NOT....
	        set_ptrs( base, base, base + buf.capacity() );
		}
		else
		{
	        set_ptrs( base, base, base + buf.size() );
		}
    }

    buffer_list							m_bufs;
	std::deque< position_type >			m_offsets;
	std::size_t							m_current_buf;
	size_type							m_alloc_size;
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_OBMULTIBUF_H