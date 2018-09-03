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

#include <logicmill/bstream/obfilebuf.h>
#include <unistd.h>

using namespace logicmill;
using namespace bstream;

obfilebuf::obfilebuf( obfilebuf&& rhs )
:
obstreambuf{ std::move( rhs ) },
m_buf{ std::move( rhs.m_buf ) },
m_filename{ std::move( rhs.m_filename ) },
m_is_open{ rhs.m_is_open },
m_mode{ rhs.m_mode },
m_flags{ rhs.m_flags },
m_fd{ rhs.m_fd }
{}

obfilebuf::obfilebuf( std::string const& filename, open_mode mode, std::error_code& err, size_type buffer_size )
:
obstreambuf{},
m_buf{ buffer_size },
m_filename{ filename },
m_is_open{ false },
m_mode{ mode },
m_flags{ to_flags( mode ) },
m_fd{ -1 }
{
	reset_ptrs();
	really_open( err );
}

obfilebuf::obfilebuf( std::string const& filename, open_mode mode, size_type buffer_size )
:
obstreambuf{},
m_buf{ buffer_size },
m_filename{ filename },
m_is_open{ false },
m_mode{ mode },
m_flags{ to_flags( mode ) },
m_fd{ -1 }
{
	reset_ptrs();
	std::error_code err;
	really_open( err );
	if ( err )
	{
		throw std::system_error{ err };
	}
}

obfilebuf::obfilebuf( open_mode mode, size_type buffer_size )
:
obstreambuf{},
m_buf{ buffer_size },
m_filename{},
m_is_open{ false },
m_mode{ mode },
m_flags{ to_flags( m_mode ) },
m_fd{ -1 }
{
	reset_ptrs();
}

void
obfilebuf::open( std::string const& filename, open_mode mode, std::error_code& err )
{
	m_filename = filename;
	m_mode = mode;
	m_flags = to_flags( mode );
	really_open( err );
}

void
obfilebuf::open( std::string const& filename, open_mode mode )
{
	m_filename = filename;
	m_mode = mode;
	m_flags = to_flags( mode );
	std::error_code err;
	really_open( err );
	if ( err )
	{
		throw std::system_error{ err };
	}
}

void
obfilebuf::open( std::string const& filename, open_mode mode, int flags_override, std::error_code& err )
{
	m_filename = filename;
	m_mode = mode;
	m_flags = flags_override;
	really_open( err );
}

void
obfilebuf::open( std::string const& filename, open_mode mode, int flags_override )
{
	m_filename = filename;
	m_mode = mode;
	m_flags = flags_override;
	std::error_code err;
	really_open( err );
	if ( err )
	{
		throw std::system_error{ err };
	}
}

// bool
// obfilebuf::really_force_mutable()
// {
//     return true;
// }

void
obfilebuf::really_flush( std::error_code& err )
{
    clear_error( err );
    auto pos = ppos();
    assert( m_dirty && m_pnext > m_dirty_start );
    assert( m_dirty_start == m_pbase );
    if ( m_last_touched != m_pbase_offset )
    {
        auto seek_result = ::lseek( m_fd, m_pbase_offset, SEEK_SET );
        if ( seek_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }
        assert( seek_result == m_pbase_offset );
    }

    {
        size_type n = static_cast< size_type >( m_pnext - m_pbase );
        auto write_result = ::write( m_fd, m_pbase, n );
        if ( write_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }
        assert( static_cast< size_type >( write_result ) == n );
        m_pbase_offset = pos;
        m_pnext = m_pbase;
    }
exit:
    return;
}

void
obfilebuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto pos = ppos();
    assert( m_pbase_offset == pos && m_pnext == m_pbase );
    assert( m_last_touched != pos );

    auto result = ::lseek( m_fd, pos, SEEK_SET );
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        goto exit;
    }
    m_last_touched = pos;
    
exit:
    return;
}

position_type
obfilebuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
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

    if ( result < 0 )
    {
        err = make_error_code( std::errc::invalid_argument );
        result = invalid_position;
        goto exit;
    }

    m_pbase_offset = result;
    m_pnext = m_pbase;

exit:
    return result;
}

void
obfilebuf::really_overflow( size_type, std::error_code& err )
{
    clear_error( err );
    assert( m_pbase_offset == ppos() && m_pnext == m_pbase );
}

void
obfilebuf::close( std::error_code& err )
{
    clear_error( err );
    flush( err );
    if ( err ) goto exit;
    
    {
        auto result = ::close( m_fd );
        if ( result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
        }
        m_is_open = false;
    }
exit:
    return;
}

void
obfilebuf::close()
{
    std::error_code err;
    close( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}

void
obfilebuf::open()
{
    std::error_code err;
    really_open( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}

position_type
obfilebuf::truncate( std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    flush( err );
    if ( err ) goto exit;

    {
        auto pos = ppos();
        assert( pos == m_pbase_offset );
        auto trunc_result = ::ftruncate( m_fd, pos );
        if ( trunc_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }

        force_high_watermark( pos );
        m_last_touched = pos;
        result = pos;
    }

exit:
    return result;
}

position_type
obfilebuf::truncate()
{
    std::error_code err;
    auto result = truncate( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

void 
obfilebuf::really_open( std::error_code& err )
{
    clear_error( err );
    if ( m_is_open )
    {
        close( err );
        if ( err ) goto exit;
    }

    if ( ( m_flags & O_CREAT ) != 0 )
    {
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // set permssions to rw-r--r--
        m_fd = ::open( m_filename.c_str(), m_flags, mode );
    }
    else
    {
        m_fd = ::open( m_filename.c_str(), m_flags );
    }

    if ( m_fd < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        goto exit;
    }

    m_is_open = true;

    {
        auto end_pos = ::lseek( m_fd, 0, SEEK_END );
        if ( end_pos < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }
        force_high_watermark( end_pos );

        if ( m_mode == open_mode::at_end || is_append( m_flags ) )
        {
            m_pbase_offset = end_pos;
            m_last_touched = end_pos; // An acceptable lie.
        }
        else
        {
            auto pos = ::lseek( m_fd, 0, SEEK_SET );
            if ( pos < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                goto exit;
            }
            assert( pos == 0 );
            m_pbase_offset = 0;
            m_last_touched = 0UL;
        }
        reset_ptrs();
        m_dirty = false;
    }

exit:
    return;
}

