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

#include <logicmill/bstream/sequential/sink.h>

using namespace logicmill;
using namespace bstream;
using namespace sequential;

sink::sink( byte_type* data, size_type size )
:
m_base_offset{ 0 },
m_base{ data },
m_next{ data },
m_end{ data + size }
{
}

sink::sink()
:
m_base_offset{ 0 },
m_base{ nullptr },
m_next{ nullptr },
m_end{ nullptr }
{}

sink::sink( sink&& rhs )
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

void
sink::flush( std::error_code& err )
{
	really_flush( err );
}

void
sink::flush()
{
	std::error_code err;
	really_flush( err );
	if ( err )
	{
		throw std::system_error{ err };
	}
}

void
sink::put( byte_type byte, std::error_code& err )
{
	err.clear();
    if ( m_next >= m_end )
    {
        assert( m_next == m_end );
        overflow( 1, err );
        if ( err ) goto exit;
    }
    *m_next++ = byte;

exit:
    return;
}

void
sink::put( byte_type byte )
{
    if ( m_next >= m_end )
    {
        assert( m_next == m_end );
        overflow( 1 );
    }
    *m_next++ = byte;
}

void
sink::putn( const byte_type* src, size_type n, std::error_code& err )
{
	err.clear();
	if ( n < 1 ) goto exit;
	
    if ( n <= static_cast< size_type >( m_end - m_next ) ) // optimize for common case ( no overflow )
    {
        ::memcpy( m_next, src, n );
        m_next += n;
    }
	else 
	{
		auto p = src;
		auto limit = src + n;
		while ( p < limit )
		{
			if ( m_next >= m_end ) 
			{
				assert( m_next == m_end ); // just checking
				overflow( limit - p, err );
				if ( err ) goto exit;
			}
			size_type chunk_size = std::min( static_cast< size_type >( m_end - m_next ), static_cast< size_type >( limit - p ) );
			::memcpy( m_next, p, chunk_size );
			p += chunk_size;
			m_next += chunk_size;
		}
	}
	
exit:
    return;
}

void
sink::putn( const byte_type* src, size_type n )
{
	if ( n > 0 )
	{		
		if ( n <= static_cast< size_type >( m_end - m_next ) ) // optimize for common case ( no overflow )
		{
			::memcpy( m_next, src, n );
			m_next += n;
		}
		else 
		{
			auto p = src;
			auto limit = src + n;
			while ( p < limit )
			{
				if ( m_next >= m_end ) 
				{
					assert( m_next == m_end ); // just checking
					overflow( limit - p );
				}
				size_type chunk_size = std::min( static_cast< size_type >( m_end - m_next ), static_cast< size_type >( limit - p ) );
				::memcpy( m_next, p, chunk_size );
				p += chunk_size;
				m_next += chunk_size;
			}
		}
	}
}


void
sink::filln( const byte_type fill_byte, size_type n, std::error_code& err )
{
	err.clear();
	if ( n < 1 ) goto exit;

    if ( n <= static_cast< size_type >( m_end - m_next ) ) // optimize for common case ( no overflow )
    {
        ::memset( m_next, fill_byte, n );
        m_next += n;
    }
	else 
	{
		size_type remaining = n;
		while ( remaining > 0 )
		{
			if ( m_next >= m_end ) 
			{
				assert( m_next == m_end ); // just checking
				overflow( remaining, err );
				if ( err ) goto exit;
			}
			size_type chunk_size = std::min( static_cast< size_type >( m_end - m_next ), remaining );
			::memset( m_next, fill_byte, chunk_size );
			remaining -= chunk_size;
			m_next += chunk_size;
		}
	}
	
exit:
    return;
}

void
sink::really_fill( byte_type fill_byte, size_type n )
{
	assert( n > 0 );
	if ( n <= static_cast< size_type >( m_end - m_next ) ) // optimize for common case ( no overflow )
	{
		::memset( m_next, fill_byte, n );
		m_next += n;
	}
	else 
	{
		size_type remaining = n;
		while ( remaining > 0 )
		{
			if ( m_next >= m_end ) 
			{
				assert( m_next == m_end ); // just checking
				overflow( remaining );
			}
			size_type chunk_size = std::min( static_cast< size_type >( m_end - m_next ), remaining );
			::memset( m_next, fill_byte, chunk_size );
			remaining -= chunk_size;
			m_next += chunk_size;
		}
	}
}

void
sink::filln( const byte_type fill_byte, size_type n )
{
	if ( n > 0 )
	{
		really_fill( fill_byte, n );		
	}
}

void
sink::overflow( size_type requested, std::error_code& err )
{
    flush( err );
    if ( err ) goto exit;

    really_overflow( requested, err );
    if ( err ) goto exit;

    assert( m_end > m_next );

exit:
    return;
}

void
sink::overflow( size_type requested )
{
    flush();
    std::error_code err;

    really_overflow( requested, err );
    if ( err )
    {
        throw std::system_error{ err };
    }

    assert( m_end > m_next );
}

void
sink::really_overflow( size_type, std::error_code& err )
{
    err = make_error_code( std::errc::no_buffer_space );
}

void
sink::really_flush( std::error_code& err )
{
    err.clear();
}

