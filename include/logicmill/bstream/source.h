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

#ifndef LOGICMILL_BSTREAM_SOURCE_H
#define LOGICMILL_BSTREAM_SOURCE_H

#include <logicmill/bstream/buffer.h>
#include <logicmill/bstream/types.h>

namespace logicmill 
{
namespace bstream 
{

namespace detail
{
    class source_test_probe;
}

class source
{

public:

	friend class detail::source_test_probe;

    source( const byte_type * buf, size_type size )
    :
    m_base_offset{ 0 },
    m_base{ buf },
    m_next{ buf },
    m_end{ buf + size }
    {}

    source( source const& ) = delete;
    source& operator=( source&& ) = delete;
    source& operator=( source const& ) = delete;

protected:

    source()
    : 
	source{ nullptr, 0 }
    {}

	source( source&& rhs )
	:
    m_base_offset{ rhs.m_base_offset },
    m_base{ rhs.m_base },
    m_next{ rhs.m_next },
    m_end{ rhs.m_end }
	{
		rhs.m_base_offset = 0;
		rhs.m_base = nullptr;
		rhs.m_next = nullptr;
		rhs.m_end = nullptr;
	}

public:

    virtual ~source() {}

    byte_type 
    get( std::error_code& err );

    byte_type
    get();

    byte_type
    peek( std::error_code& err );

    byte_type
    peek();

    virtual shared_buffer
    get_shared_slice( size_type n, std::error_code& err );

    virtual shared_buffer
    get_shared_slice( size_type n );

    virtual const_buffer
    get_slice( size_type n, std::error_code& err );

    virtual const_buffer
    get_slice( size_type n );

    size_type 
    getn( byte_type* dst, size_type n, std::error_code& err );

    size_type 
    getn( byte_type* dst, size_type n );

	size_type
	size() const
	{
		return really_get_size();
	}

	void
	rewind()
	{
		really_rewind();
	}

	size_type
	available() const
	{
		assert( m_end >= m_next );
		return static_cast< size_type >( m_end - m_next );
	}

	/*
	 * random-access methods (seek/tell)
	 */

    position_type
    position( position_type position, std::error_code& err );

    position_type
    position( position_type position );

    position_type
    position( offset_type offset, seek_anchor where, std::error_code& err );

    position_type
    position( offset_type offset, seek_anchor where );

    position_type
    position() const;

protected:

    const byte_type*
    gbump( offset_type offset )
    {
        m_next += offset;
        return m_next;
    }

    position_type
    gpos() const
    {
        return m_base_offset + ( m_next - m_base );
    }

    size_type
    underflow( std::error_code& err )
    {
        return really_underflow( err );
    }

    size_type
    underflow();

    void
    set_ptrs( const byte_type * base, const byte_type * next, const byte_type * end )
    {
        m_base =  base;
        m_next = next;
        m_end = end;
    }

    virtual size_type
    really_underflow( std::error_code& err );

	position_type
	new_position( offset_type offset, seek_anchor where ) const;

    virtual position_type
    really_seek( position_type pos, std::error_code& err );

	// TODO: is this necessary with the removal of source_base ?
	virtual position_type
	really_get_position() const;

	// TODO: is this necessary with the removal of source_base ?
	virtual size_type
	really_get_size() const;

	// TODO: is this necessary with the removal of source_base ?
	virtual void
	really_rewind();

    position_type						m_base_offset;
    const byte_type*					m_base;
    const byte_type*					m_next;
    const byte_type*					m_end;
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_SOURCE_H
