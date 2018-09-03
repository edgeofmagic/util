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

#include <logicmill/bstream/obstreambuf.h>

using namespace logicmill;
using namespace bstream;

obstreambuf::obstreambuf( byte_type* data, size_type size )
:
m_pbase_offset{ 0 },
m_high_watermark{ 0 },
m_last_touched{ 0 },
m_pbase{ data },
m_pnext{ data },
m_pend{ data + size },
m_dirty_start{ data },
m_dirty{ false }
// m_implies_mutability{ true }
{
}

obstreambuf::obstreambuf()
:
m_pbase_offset{ 0 },
m_high_watermark{ 0 },
m_last_touched{ 0 },
m_pbase{ nullptr },
m_pnext{ nullptr },
m_pend{ nullptr },
m_dirty_start{ nullptr },
m_dirty{ false }
// m_implies_mutability{ true }  // TODO: really?
{}

obstreambuf::obstreambuf( obstreambuf&& rhs )
: 
m_pbase_offset{ rhs.m_pbase_offset },
m_high_watermark{ rhs.m_high_watermark },
m_last_touched{ rhs.m_last_touched },
m_pbase{ rhs.m_pbase },
m_pnext{ rhs.m_pnext },
m_pend{ rhs.m_pend },
m_dirty_start{ rhs.m_dirty_start },
m_dirty{ rhs.m_dirty }
// m_implies_mutability{ rhs.m_implies_mutability }
{
	rhs.m_pbase_offset = 0;
	rhs.m_high_watermark = 0;
	rhs.m_last_touched = 0;
	rhs.m_pbase = nullptr;
	rhs.m_pnext = nullptr;
	rhs.m_pend = nullptr;
	rhs.m_dirty_start = nullptr;
	rhs.m_dirty = false;
	// rhs.m_implies_mutability = true; // TODO: really?

}

void
obstreambuf::flush( std::error_code& err )
{
    clear_error( err );
    if ( m_dirty )
    {
        // assert( mutability_implied() );
        really_flush( err );
        if ( ! err )
        {
            set_high_watermark();
            m_last_touched = ppos();
            m_dirty = false;
        }
    }
}

void
obstreambuf::flush()
{
    if ( m_dirty )
    {
        // assert( mutability_implied() );
        std::error_code err;
        really_flush( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
        set_high_watermark();
        m_last_touched = ppos();
        m_dirty = false;
    }
}

void
obstreambuf::put( byte_type byte, std::error_code& err )
{
    // force_mutable();
    if ( ! m_dirty )
    {
        touch( err );
        if ( err ) goto exit;
    }
    if ( m_pnext >= m_pend )
    {
        assert( m_pnext == m_pend );
        overflow( 1, err );
        if ( err ) goto exit;
        assert( ! m_dirty );
    }
    if ( ! m_dirty )
    {
        m_dirty_start = m_pnext;
    }
    *m_pnext++ = byte;
    m_dirty = true;

exit:
    return;
}

void
obstreambuf::put( byte_type byte )
{
    // force_mutable();
    if ( ! m_dirty )
    {
        touch();
    }
    if ( m_pnext >= m_pend )
    {
        assert( m_pnext == m_pend );
        overflow( 1 );
        assert( ! m_dirty );
    }
    if ( ! m_dirty )
    {
        m_dirty_start = m_pnext;
    }
    *m_pnext++ = byte;
    m_dirty = true;
}

void
obstreambuf::putn( const byte_type* src, size_type n, std::error_code& err )
{
    // force_mutable();
    if ( ! m_dirty )
    {
        touch( err );
        if ( err ) goto exit;
        m_dirty_start = m_pnext;
    }
    if ( n <= static_cast< size_type >( m_pend - m_pnext ) ) // optimize for common case ( no overflow )
    {
        ::memcpy( m_pnext, src, n );
        m_pnext += n;
        m_dirty = true;
    }
    else 
    {
        overflow( n, err );
        if ( err ) goto exit;
        if ( n <= static_cast< size_type >( m_pend - m_pnext ) ) // try it in one go
        {
            if ( ! m_dirty )
            {
                m_dirty_start = m_pnext;
            }
            ::memcpy( m_pnext, src, n );
            m_pnext += n;
            m_dirty = true;
        }
        else // put it in chunks
        {
            auto remaining = n;
            auto p = src;
            while ( remaining > 0 )
            {
                if ( m_pnext >= m_pend ) // should be false on first iteration
                {
                    assert( m_pnext == m_pend );
                    overflow( remaining, err );
                    if ( err ) goto exit;
                }

                assert( m_pend - m_pnext > 0 );

                size_type chunk_size = std::min( remaining, static_cast< size_type >( m_pend - m_pnext ) );
                if ( ! m_dirty )
                {
                    m_dirty_start = m_pnext;
                }
                ::memcpy( m_pnext, p, chunk_size );
                remaining -= chunk_size;
                p += chunk_size;
                m_pnext += chunk_size;
                m_dirty = true;
            }
        }
    }
    
exit:
    return;
}

void
obstreambuf::putn( const byte_type* src, size_type n )
{
    std::error_code err;
    putn( src, n, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}

void
obstreambuf::filln( const byte_type fill_byte, size_type n, std::error_code& err )
{
    // force_mutable();
    if ( ! m_dirty )
    {
        touch( err );
        if ( err ) goto exit;
        m_dirty_start = m_pnext;
    }
    if ( n <= static_cast< size_type >( m_pend - m_pnext ) ) // optimize for common case ( no overflow )
    {
        ::memset( m_pnext, fill_byte, n );
        m_pnext += n;
        m_dirty = true;
    }
    else 
    {
        overflow( n, err );
        if ( err ) goto exit;
        if ( n <= static_cast< size_type >( m_pend - m_pnext ) ) // try it in one go
        {
            if ( ! m_dirty )
            {
                m_dirty_start = m_pnext;
            }
            ::memset( m_pnext, fill_byte, n );
            m_pnext += n;
            m_dirty = true;
        }
        else // put it in chunks
        {
            auto remaining = n;
            while ( remaining > 0 )
            {
                if ( m_pnext >= m_pend ) // should be false on first iteration
                {
                    assert( m_pnext == m_pend );
                    overflow( remaining, err );
                    if ( err ) goto exit;
                }

                assert( m_pend - m_pnext > 0 );

                size_type chunk_size = std::min( remaining, static_cast< size_type >( m_pend - m_pnext ) );
                if ( ! m_dirty )
                {
                    m_dirty_start = m_pnext;
                }
                ::memset( m_pnext, fill_byte, chunk_size );
                remaining -= chunk_size;
                m_pnext += chunk_size;
                m_dirty = true;
            }
        }
    }
    
exit:
    return;
}

void
obstreambuf::filln( const byte_type fill_byte, size_type n )
{
    std::error_code err;
    filln( fill_byte, n, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}

position_type
obstreambuf::seek( seek_anchor where, offset_type offset )
{
    std::error_code err;
    auto result = really_seek( where, offset, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

position_type
obstreambuf::tell( seek_anchor where )
{
    std::error_code err;
    auto result = really_tell( where, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

void
obstreambuf::touch( std::error_code& err )
{
    assert( ! m_dirty );
    // assert( mutability_implied() );
    auto pos = ppos();
    
    if ( m_last_touched != pos )
    {
        really_touch( err );
        if ( err ) goto exit;
    }

    assert( ppos() == pos );
    assert( pos == m_last_touched );
    assert( ! m_dirty );
exit:
    return;
}

void
obstreambuf::touch()
{
    assert( ! m_dirty );
    // assert( mutability_implied() );
    auto pos = ppos();
    
    if ( m_last_touched != pos )
    {
        std::error_code err;
        really_touch( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }

    assert( ppos() == pos );
    assert( pos == m_last_touched );
    assert( ! m_dirty );
}

void
obstreambuf::overflow( size_type requested, std::error_code& err )
{
    flush( err );
    if ( err ) goto exit;

    really_overflow( requested, err );
    if ( err ) goto exit;

    assert( m_pend > m_pnext );

exit:
    return;
}

void
obstreambuf::overflow( size_type requested )
{
    flush();
    std::error_code err;

    really_overflow( requested, err );
    if ( err )
    {
        throw std::system_error{ err };
    }

    assert( m_pend > m_pnext );
}

// bool
// obstreambuf::really_force_mutable()
// {
//     return true;
// }

void
obstreambuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto hwm = get_high_watermark();
    auto pos = ppos();
    assert( hwm < m_pend - m_pbase && pos <= m_pend - m_pbase );
    assert( m_last_touched != pos );

    if ( hwm < pos )
    {
        m_pnext = m_pbase + ( hwm - m_pbase_offset );

        filln( pos - hwm, 0, err );
        if ( err ) goto exit;

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
obstreambuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
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
            // high watermark will be current because of flush() above
            auto end_pos = get_high_watermark();
            result = end_pos + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 || result > m_pend - m_pbase )
    {
        err = make_error_code( std::errc::invalid_argument );
        result = invalid_position;
        goto exit;
    }

    m_pnext = m_pbase + ( result - m_pbase_offset );

exit:
    return result;
}

position_type
obstreambuf::really_tell( seek_anchor where, std::error_code& err )
{
	clear_error( err );

	position_type result = invalid_position;

	switch ( where )
	{
		case seek_anchor::current:
		{
			result = ppos();
		}
		break;
        
		case seek_anchor::end:
		{
			set_high_watermark(); // Note: hwm may advance ahead of last_flushed and dirty_start here. This should be ok.
			result = get_high_watermark();
        }
        break;

		case seek_anchor::begin:
		{
			result = 0;
		}
		break;
    }

	return result;
}

void
obstreambuf::really_overflow( size_type, std::error_code& err )
{
    err = make_error_code( std::errc::no_buffer_space );
}

void
obstreambuf::really_flush( std::error_code& err )
{
    clear_error( err );
    assert( m_dirty && m_pnext > m_dirty_start );
}

