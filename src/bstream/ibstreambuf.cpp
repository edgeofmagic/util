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

#include <logicmill/bstream/ibstreambuf.h>
#include <logicmill/bstream/error.h>

using namespace logicmill;
using namespace bstream;

position_type
ibstreambuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = gpos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            result = ( m_gend - m_gbase ) + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 || result > ( m_gend - m_gbase ) )
    {
        err = make_error_code( std::errc::invalid_seek );
        result = invalid_position;
        goto exit;
    }

    m_gnext = m_gbase + result;

exit:
    return result;

}

position_type
ibstreambuf::really_tell( seek_anchor where, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = gpos();
        }
        break;
        
        case seek_anchor::end:
        {
            result = static_cast< position_type >( m_gend - m_gbase );
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

size_type
ibstreambuf::really_underflow( std::error_code& err )
{
    clear_error( err );
    return 0UL;
}

byte_type 
ibstreambuf::get( std::error_code& err )
{
    byte_type result = 0;
    if ( m_gnext >= m_gend )
    {
        assert( m_gnext == m_gend );
        auto available = underflow( err );
        if ( err ) goto exit;
        if ( available < 1 )
        {
            err = make_error_code( bstream::errc::read_past_end_of_stream );
            goto exit;
        }
    }
    assert( m_gnext < m_gend );
    result = *m_gnext++;
exit:
    return result;
}

byte_type
ibstreambuf::get()
{
    if ( m_gnext >= m_gend )
    {
        assert( m_gnext == m_gend );
        if ( underflow() < 1 )
        {
            throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
        }
    }
    assert( m_gnext < m_gend );
    return *m_gnext++;
}

byte_type
ibstreambuf::peek( std::error_code& err )
{
    byte_type result = 0;
    if ( m_gnext >= m_gend )
    {
        assert( m_gnext == m_gend );
        auto available = underflow( err );
        if ( err ) goto exit;
        if ( available < 1 )
        {
            err = make_error_code( bstream::errc::read_past_end_of_stream );
            goto exit;
        }
    }
    assert( m_gnext < m_gend );
    result = * m_gnext;
exit:
    return result;
}

byte_type
ibstreambuf::peek()
{
    if ( m_gnext >= m_gend )
    {
        assert( m_gnext == m_gend );
        if ( underflow() < 1 )
        {
            throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
        }
    }
    assert( m_gnext < m_gend );
    return * m_gnext;
}

const_buffer
ibstreambuf::getn( size_type n, std::error_code& err )
{
	mutable_buffer buf{ n };
	auto got = getn( buf.data(), n, err );
	buf.size( got );
	return const_buffer{ buf };
}

const_buffer
ibstreambuf::getn( size_type n )
{
    mutable_buffer buf{ n };
    auto got = getn( buf.data(), n );
    buf.size( got );
	return const_buffer{ buf };
}

size_type 
ibstreambuf::getn( byte_type* dst, size_type n, std::error_code& err )
{
    size_type available = m_gend - m_gnext;
    size_type remaining = n;
    byte_type* bp = dst;
    while ( remaining > 0 )
    {
        if ( available < 1 )
        {
            underflow( err );
            if ( err ) goto exit;
            available = m_gend - m_gnext;
            if ( available < 1 )
            {
                break;
            }
        }
        size_type chunk_size = std::min( remaining, available );
        ::memcpy( bp, m_gnext, chunk_size );
        bp += chunk_size;
        m_gnext += chunk_size;
        remaining -= chunk_size;
        available = m_gend - m_gnext;
    }
exit:
    return n - remaining; 
}

size_type 
ibstreambuf::getn( byte_type* dst, size_type n )
{
    size_type available = m_gend - m_gnext;
    size_type remaining = n;
    byte_type* bp = dst;
    while ( remaining > 0 )
    {
        if ( available < 1 )
        {
            underflow();
            available = m_gend - m_gnext;
            if ( available < 1 )
            {
                break;
            }
        }
        size_type chunk_size = std::min( remaining, available );
        ::memcpy( bp, m_gnext, chunk_size );
        bp += chunk_size;
        m_gnext += chunk_size;
        remaining -= chunk_size;
        available = m_gend - m_gnext;
    }
    return n - remaining; 
}

position_type
ibstreambuf::seek( position_type position )
{
    std::error_code err;
    auto result = really_seek( seek_anchor::begin, static_cast< offset_type >( position ), err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

position_type
ibstreambuf::seek( seek_anchor where, offset_type offset )
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
ibstreambuf::tell( seek_anchor where )
{
    std::error_code err;
    auto result = really_tell( where, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

size_type
ibstreambuf::underflow()
{
    std::error_code err;
    auto available = really_underflow( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return available;
}

