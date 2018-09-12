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

#ifndef LOGICMILL_BSTREAM_IBSTREAMBUF_H
#define LOGICMILL_BSTREAM_IBSTREAMBUF_H

#include <logicmill/buffer.h>
#include <logicmill/bstream/types.h>

namespace logicmill 
{
namespace bstream 
{

#ifndef DOCTEST_CONFIG_DISABLE
namespace detail
{
    class ibs_test_probe;
}
#endif

class ibstreambuf
{

public:

#ifndef DOCTEST_CONFIG_DISABLE
	friend class detail::ibs_test_probe;
#endif

    ibstreambuf( const byte_type * buf, size_type size )
    :
    m_gbase_offset{ 0 },
    m_gbase{ buf },
    m_gnext{ buf },
    m_gend{ buf + size },
	m_end_position{ static_cast< position_type >( size ) }
    {}

    ibstreambuf( ibstreambuf const& ) = delete;
    ibstreambuf& operator=( ibstreambuf&& ) = delete;
    ibstreambuf& operator=( ibstreambuf const& ) = delete;

protected:

    ibstreambuf()
    : 
	ibstreambuf{ nullptr, 0 }
    {}

	ibstreambuf( ibstreambuf&& rhs )
	:
    m_gbase_offset{ rhs.m_gbase_offset },
    m_gbase{ rhs.m_gbase },
    m_gnext{ rhs.m_gnext },
    m_gend{ rhs.m_gend },
	m_end_position{ rhs.m_end_position }
	{
		rhs.m_gbase_offset = 0;
		rhs.m_gbase = nullptr;
		rhs.m_gnext = nullptr;
		rhs.m_gend = nullptr;
		rhs.m_end_position = 0;
	}

public:

    virtual ~ibstreambuf() {}

    byte_type 
    get( std::error_code& err );

    byte_type
    get();

    byte_type
    peek( std::error_code& err );

    byte_type
    peek();

    virtual const_buffer
    getn( size_type n, std::error_code& err );

    virtual const_buffer
    getn( size_type n );

    size_type 
    getn( byte_type* dst, size_type n, std::error_code& err );

    size_type 
    getn( byte_type* dst, size_type n );

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

	position_type
	size() const
	{
		return m_end_position;
	}

protected:

    const byte_type*
    gbump( offset_type offset )
    {
        m_gnext += offset;
        return m_gnext;
    }

    position_type
    gpos() const
    {
        return m_gbase_offset + ( m_gnext - m_gbase );
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
        m_gbase =  base;
        m_gnext = next;
        m_gend = end;
    }

	position_type
	new_position( offset_type offset, seek_anchor where ) const;

    virtual position_type
    really_seek( position_type pos, std::error_code& err );

    virtual size_type
    really_underflow( std::error_code& err );

    position_type						m_gbase_offset;
    const byte_type*					m_gbase;
    const byte_type*					m_gnext;
    const byte_type*					m_gend;
	position_type						m_end_position;
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_IBSTREAMBUF_H
