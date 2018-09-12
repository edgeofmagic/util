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
m_jump_to{ 0 },
m_pbase{ data },
m_pnext{ data },
m_pend{ data + size },
m_dirty_start{ data },
m_dirty{ false },
m_did_jump{ false }
{
}

obstreambuf::obstreambuf()
:
m_pbase_offset{ 0 },
m_high_watermark{ 0 },
m_jump_to{ 0 },
m_pbase{ nullptr },
m_pnext{ nullptr },
m_pend{ nullptr },
m_dirty_start{ nullptr },
m_dirty{ false },
m_did_jump{ false }
{}

obstreambuf::obstreambuf( obstreambuf&& rhs )
: 
m_pbase_offset{ rhs.m_pbase_offset },
m_high_watermark{ rhs.m_high_watermark },
m_jump_to{ rhs.m_jump_to },
m_pbase{ rhs.m_pbase },
m_pnext{ rhs.m_pnext },
m_pend{ rhs.m_pend },
m_dirty_start{ rhs.m_dirty_start },
m_dirty{ rhs.m_dirty },
m_did_jump{ rhs.m_did_jump }
{
	rhs.m_pbase_offset = 0;
	rhs.m_high_watermark = 0;
	rhs.m_jump_to = 0;
	rhs.m_pbase = nullptr;
	rhs.m_pnext = nullptr;
	rhs.m_pend = nullptr;
	rhs.m_dirty_start = nullptr;
	rhs.m_dirty = false;
	rhs.m_did_jump = false;

}

void
obstreambuf::flush( std::error_code& err )
{
    clear_error( err );
    if ( m_dirty )
    {
        really_flush( err );
		if ( err ) goto exit;

		set_high_watermark();
		m_dirty = false;
    }

exit:
	return;
}

void
obstreambuf::flush()
{
    if ( m_dirty )
    {
        std::error_code err;
        really_flush( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
        set_high_watermark();
        m_dirty = false;
    }
}

void
obstreambuf::put( byte_type byte, std::error_code& err )
{
	clear_error( err );
    if ( m_did_jump )
    {
        really_jump( err );
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
    if ( m_did_jump )
    {
		std::error_code err;
        really_jump( err );
		if ( err ) throw std::system_error{ err };
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
	    m_dirty = true;
    }
    *m_pnext++ = byte;
}

void
obstreambuf::putn( const byte_type* src, size_type n, std::error_code& err )
{
	clear_error( err );
	if ( n < 1 ) goto exit;

	if ( m_did_jump )
	{
        really_jump( err );
	}
	
    if ( n <= static_cast< size_type >( m_pend - m_pnext ) ) // optimize for common case ( no overflow )
    {
		if ( ! m_dirty )
		{
			m_dirty_start = m_pnext;
			m_dirty = true;
		}
        ::memcpy( m_pnext, src, n );
        m_pnext += n;
    }
	else 
	{
		auto p = src;
		auto limit = src + n;
		while ( p < limit )
		{
			if ( m_pnext >= m_pend ) 
			{
				assert( m_pnext == m_pend ); // just checking
				overflow( limit - p, err );
				if ( err ) goto exit;
				assert( ! m_dirty );
			}
			if ( ! m_dirty )
			{
				m_dirty_start = m_pnext;
				m_dirty = true;
			}
			size_type chunk_size = std::min( static_cast< size_type >( m_pend - m_pnext ), static_cast< size_type >( limit - p ) );
			::memcpy( m_pnext, p, chunk_size );
			p += chunk_size;
			m_pnext += chunk_size;
		}
	}
	
exit:
    return;
}

void
obstreambuf::putn( const byte_type* src, size_type n )
{
	if ( n > 0 )
	{
		if ( m_did_jump )
		{
			std::error_code err;
			really_jump( err );
			if ( err ) throw std::system_error{ err };
		}
		
		if ( n <= static_cast< size_type >( m_pend - m_pnext ) ) // optimize for common case ( no overflow )
		{
			if ( ! m_dirty )
			{
				m_dirty_start = m_pnext;
				m_dirty = true;
			}
			::memcpy( m_pnext, src, n );
			m_pnext += n;
		}
		else 
		{
			auto p = src;
			auto limit = src + n;
			while ( p < limit )
			{
				if ( m_pnext >= m_pend ) 
				{
					assert( m_pnext == m_pend ); // just checking
					overflow( limit - p );
					assert( ! m_dirty );
				}
				if ( ! m_dirty )
				{
					m_dirty_start = m_pnext;
					m_dirty = true;
				}
				size_type chunk_size = std::min( static_cast< size_type >( m_pend - m_pnext ), static_cast< size_type >( limit - p ) );
				::memcpy( m_pnext, p, chunk_size );
				p += chunk_size;
				m_pnext += chunk_size;
			}
		}
	}
}


void
obstreambuf::filln( const byte_type fill_byte, size_type n, std::error_code& err )
{
	clear_error( err );
	if ( n < 1 ) goto exit;

	if ( m_did_jump )
	{
        really_jump( err );
	}
	
    if ( n <= static_cast< size_type >( m_pend - m_pnext ) ) // optimize for common case ( no overflow )
    {
		if ( ! m_dirty )
		{
			m_dirty_start = m_pnext;
			m_dirty = true;
		}
        ::memset( m_pnext, fill_byte, n );
        m_pnext += n;
    }
	else 
	{
		size_type remaining = n;
		while ( remaining > 0 )
		{
			if ( m_pnext >= m_pend ) 
			{
				assert( m_pnext == m_pend ); // just checking
				overflow( remaining, err );
				if ( err ) goto exit;
				assert( ! m_dirty );
			}
			if ( ! m_dirty )
			{
				m_dirty_start = m_pnext;
				m_dirty = true;
			}
			size_type chunk_size = std::min( static_cast< size_type >( m_pend - m_pnext ), remaining );
			::memset( m_pnext, fill_byte, chunk_size );
			remaining -= chunk_size;
			m_pnext += chunk_size;
		}
	}
	
exit:
    return;
}

void
obstreambuf::really_fill( byte_type fill_byte, size_type n )
{
	assert( n > 0 );
	if ( n <= static_cast< size_type >( m_pend - m_pnext ) ) // optimize for common case ( no overflow )
	{
		if ( ! m_dirty )
		{
			m_dirty_start = m_pnext;
			m_dirty = true;
		}
		::memset( m_pnext, fill_byte, n );
		m_pnext += n;
	}
	else 
	{
		size_type remaining = n;
		while ( remaining > 0 )
		{
			if ( m_pnext >= m_pend ) 
			{
				assert( m_pnext == m_pend ); // just checking
				overflow( remaining );
				assert( ! m_dirty );
			}
			if ( ! m_dirty )
			{
				m_dirty_start = m_pnext;
				m_dirty = true;
			}
			size_type chunk_size = std::min( static_cast< size_type >( m_pend - m_pnext ), remaining );
			::memset( m_pnext, fill_byte, chunk_size );
			remaining -= chunk_size;
			m_pnext += chunk_size;
		}
	}
}

void
obstreambuf::filln( const byte_type fill_byte, size_type n )
{
	if ( n > 0 )
	{
		if ( m_did_jump )
		{
			std::error_code err;
			really_jump( err );
			if ( err ) throw std::system_error{ err };
		}
		really_fill( fill_byte, n );		
	}
}


size_type
obstreambuf::size() const
{
	// don't actually set_high_watermark here... at some point flush() will do that
	// note: this means that ppos() and size may exceed high watermark, but only if m_dirty is false
	return ( m_dirty && ( ppos() > get_high_watermark() ) ) ? ppos() : get_high_watermark();
}

position_type
obstreambuf::new_position( offset_type offset, seek_anchor where  ) const
{
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ppos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            result = size() + offset;
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

bool
obstreambuf::is_valid_position( position_type pos ) const
{
    return ( pos >= 0 ) && ( pos <= ( m_pend - m_pbase ) );
}


position_type
obstreambuf::position( offset_type offset, seek_anchor where )
{
	auto new_pos = new_position( offset, where );

    if ( ! is_valid_position( new_pos ) )
    {
        throw std::system_error{  make_error_code( std::errc::invalid_argument ) };
    }

	if ( new_pos != ppos() )
	{
		m_did_jump = true;
		m_jump_to = new_pos;
	}

    return new_pos;
}

position_type
obstreambuf::position( offset_type offset, seek_anchor where, std::error_code& err )
{
    clear_error( err );

	auto new_pos = new_position( offset, where );

    if ( ! is_valid_position( new_pos ) )
    {
        err = make_error_code( std::errc::invalid_argument );
		goto exit;
    }

	if ( new_pos != ppos() )
	{
		if ( err ) goto exit;
		m_did_jump = true;
		m_jump_to = new_pos;
	}

exit:
    return new_pos;
}

position_type
obstreambuf::position() const
{
	return ppos();
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

void
obstreambuf::really_jump( std::error_code& err )
{
	clear_error( err );
	assert( m_did_jump );
	assert( is_valid_position( m_jump_to ) );

    auto hwm = set_high_watermark();

    if ( hwm < m_jump_to )
    {
		auto gap = m_jump_to - hwm;

		really_fill( 0, gap );
		set_high_watermark();

        assert( ppos() == m_jump_to );
        assert( get_high_watermark() == m_jump_to );
    }
    else
    {
		m_pnext = m_pbase + m_jump_to;
        assert( ppos() == m_jump_to );
    }
	m_did_jump = false;
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

