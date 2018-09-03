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

// bool
// obmembuf::really_force_mutable()
// {
//     if ( ! m_buf.is_mutable() )
//     {
// 		set_high_watermark();
//         position_type pos = m_pnext - m_pbase;
//         m_buf.force_unique( get_high_watermark() );
//         auto new_base = m_buf.data();
//         set_ptrs( new_base, new_base + pos, new_base + m_buf.size() );
//     }
//     return true;
// }

void
obmembuf::really_flush( std::error_code& err )
{
    clear_error( err );
    assert( m_dirty && m_pnext > m_dirty_start );
}

void
obmembuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto hwm = get_high_watermark();
    auto pos = ppos();
    assert( hwm < m_pend - m_pbase && pos <= m_pend - m_pbase );
    assert( m_last_touched != pos );

    if ( hwm < pos )
    {
        m_dirty_start = m_pbase + hwm;
        size_type n = static_cast< size_type >( pos - hwm );
        ::memset( m_dirty_start, 0, n );
        m_dirty = true;

        flush( err );
        if ( err ) goto exit;

        assert( ppos() == pos );
        assert( get_high_watermark() == pos );
        assert( m_last_touched == pos );
    }
    else
    {
        m_last_touched = pos;
        assert( hwm >= pos );
    }
    
exit:
    return;
}

position_type
obmembuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    flush( err );
    if ( err ) goto exit;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ppos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            auto end_pos = get_high_watermark(); // high watermark is current from flush() above
            result = end_pos + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 )
    {
        err = make_error_code( std::errc::invalid_argument );
        result = invalid_position;
    }
    else if ( result >= static_cast< position_type >( m_pend - m_pbase ) )
    {
        resize( result );
        auto new_base = m_buf.data();
        set_ptrs( new_base, new_base + result, new_base + m_buf.size() );
    }
    else
    {
        m_pnext = m_pbase + result;
    }

exit:
    return result;
}

void
obmembuf::really_overflow( size_type n, std::error_code& err )
{
    clear_error( err );
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
	m_last_touched = 0UL;
	m_dirty = false;
	return *this;
}

const_buffer
obmembuf::get_buffer()
{
	set_high_watermark();
	return const_buffer{ m_buf, 0, static_cast< buffer::size_type >( get_high_watermark() ) };
	// return m_buf.slice( 0, get_high_watermark() );
}

mutable_buffer&
obmembuf::get_buffer_ref()
{
    set_high_watermark();
    m_buf.size( get_high_watermark() );
	return m_buf;
}

const_buffer
obmembuf::release_buffer()
{
    set_high_watermark();
    m_buf.size( get_high_watermark() );
    reset_high_water_mark();
    m_last_touched = 0UL;
    m_dirty = false;
    set_ptrs( nullptr, nullptr, nullptr );
    return const_buffer{ std::move( m_buf ) };
}
