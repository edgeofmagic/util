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
ibstreambuf::new_position( offset_type offset, seek_anchor where ) const
{
    position_type result = bstream::npos;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = gpos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            result = m_end_position + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

	return result;
}

position_type
ibstreambuf::position( offset_type offset, seek_anchor where, std::error_code& err )
{
    err.clear();
	position_type result = new_position( offset, where );

    if ( result < 0 || result > ( m_end_position ) )
    {
        err = make_error_code( std::errc::invalid_seek );
        result = bstream::npos;
        goto exit;
    }

	result = really_seek( result, err );
	if ( err ) goto exit;

exit:
	return result;
}

position_type
ibstreambuf::position( position_type pos )
{
    std::error_code err;
    auto result = position( static_cast< offset_type >( pos ), seek_anchor::begin, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

position_type
ibstreambuf::position( offset_type offset, seek_anchor where )
{
    std::error_code err;
    auto result = position( offset, where, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

position_type
ibstreambuf::position( position_type pos, std::error_code& err )
{
	return position( static_cast< offset_type >( pos ), seek_anchor::begin, err );
}

position_type
ibstreambuf::really_seek( position_type pos, std::error_code& err )
{
    err.clear();
    m_gnext = m_gbase + pos;
    return pos;
}

position_type
ibstreambuf::position() const
{
	return gpos();
}

size_type
ibstreambuf::really_underflow( std::error_code& err )
{
    err.clear();
	m_gnext = m_gend;
    return 0UL;
}

byte_type 
ibstreambuf::get( std::error_code& err )
{
	err.clear();
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
	err.clear();
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

shared_buffer
ibstreambuf::getn( as_shared_buffer, size_type n, std::error_code& err )
{
	err.clear();
	mutable_buffer buf{ n };
	auto got = getn( buf.data(), n, err );
	buf.size( got );
	return shared_buffer{ std::move( buf ) };
}

shared_buffer
ibstreambuf::getn( as_shared_buffer, size_type n )
{
    mutable_buffer buf{ n };
    auto got = getn( buf.data(), n );
    buf.size( got );
	return shared_buffer{ std::move( buf ) };
}

const_buffer
ibstreambuf::getn( as_const_buffer, size_type n, std::error_code& err )
{
	err.clear();
	mutable_buffer buf{ n };
	auto got = getn( buf.data(), n, err );
	buf.size( got );
	return const_buffer{ std::move( buf ) };
}

const_buffer
ibstreambuf::getn( as_const_buffer, size_type n )
{
    mutable_buffer buf{ n };
    auto got = getn( buf.data(), n );
    buf.size( got );
	return const_buffer{ std::move( buf ) };
}

size_type 
ibstreambuf::getn( byte_type* dst, size_type n, std::error_code& err )
{
	err.clear();
	size_type result = 0;
	// optimize for the available case
	if ( n < m_gend - m_gnext )
	{
		::memcpy( dst, m_gnext, n );
		m_gnext += n;
		result = n;
	}
	else
	{
		byte_type* p = dst;
		byte_type* endp = dst + n;
		while ( p < endp )
		{
			if ( m_gnext >= m_gend )
			{
				assert( m_gnext == m_gend );
				underflow( err );
				if ( err ) goto exit;
			}
			size_type chunk_size = std::min( static_cast< size_type >( m_gend - m_gnext ), static_cast< size_type >( endp - p ) );
			if ( chunk_size < 1 ) break;
			::memcpy( p, m_gnext, chunk_size );
			p += chunk_size;
			m_gnext += chunk_size;
		}
		result = static_cast< size_type >( p - dst );
	}
exit:
	return result;
}

size_type 
ibstreambuf::getn( byte_type* dst, size_type n )
{
	size_type result = 0;
	// optimize for the available case
	if ( n < m_gend - m_gnext )
	{
		::memcpy( dst, m_gnext, n );
		m_gnext += n;
		result = n;
	}
	else
	{
		byte_type* p = dst;
		byte_type* endp = dst + n;
		while ( p < endp )
		{
			if ( m_gnext >= m_gend )
			{
				assert( m_gnext == m_gend );
				underflow();
			}
			size_type chunk_size = std::min( static_cast< size_type >( m_gend - m_gnext ), static_cast< size_type >( endp - p ) );
			if ( chunk_size < 1 ) break;
			::memcpy( p, m_gnext, chunk_size );
			p += chunk_size;
			m_gnext += chunk_size;
		}
		result = static_cast< size_type >( p - dst );
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

