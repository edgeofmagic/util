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

#include <logicmill/bstream/obmembuf.h>

using namespace logicmill;
using namespace bstream;

void
obmembuf::really_flush( std::error_code& err )
{
    err.clear();
    assert( m_dirty && m_pnext > m_dirty_start );
}

void
obmembuf::really_jump( std::error_code& err )
{
	err.clear();
	assert( m_did_jump );
	assert( is_valid_position( m_jump_to ) );

	if ( m_dirty )
	{
		flush( err );
	}

    auto hwm = get_high_watermark();

    if ( hwm < m_jump_to )
    {
		auto gap = m_jump_to - hwm;

		really_fill( 0, gap );
		// set_high_watermark();

        assert( ppos() == m_jump_to );
        // assert( get_high_watermark() == m_jump_to );
    }
    else
    {
		m_pnext = m_pbase + m_jump_to;
        assert( ppos() == m_jump_to );
    }
	m_did_jump = false;
}

bool
obmembuf::is_valid_position( position_type pos ) const
{
    return pos >= 0;
}

void
obmembuf::really_overflow( size_type n, std::error_code& err )
{
    err.clear();
    assert( std::less_equal< byte_type * >()( m_pnext, m_pend ) );
    assert( ( m_pnext - m_pbase ) + n > m_buf.size() );
    auto pos = ppos();
    size_type required = ( m_pnext - m_pbase ) + n;
    resize( required );
	// assert( m_buf.is_mutable() );
    auto new_base = m_buf.data();
    set_ptrs( new_base, new_base + pos, new_base + m_buf.capacity() );
}

obmembuf& 
obmembuf::clear() noexcept
{
	reset_ptrs();
	reset_high_water_mark();
	m_did_jump = false;
	m_dirty = false;
	return *this;
}

const_buffer
obmembuf::get_buffer()
{
	if ( m_dirty )
	{
		flush();
	}
    m_buf.size( get_high_watermark() );
	return const_buffer{ m_buf };
}

mutable_buffer&
obmembuf::get_buffer_ref()
{
	if ( m_dirty )
	{
		flush();
	}
    m_buf.size( get_high_watermark() );
	return m_buf;
}

const_buffer
obmembuf::release_buffer()
{
	if ( m_dirty )
	{
		flush();
	}
    m_buf.size( get_high_watermark() );
    reset_high_water_mark();
	m_did_jump = false;
    m_dirty = false;
    set_ptrs( nullptr, nullptr, nullptr );
    return const_buffer{ std::move( m_buf ) };
}
