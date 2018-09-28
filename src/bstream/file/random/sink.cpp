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

#include <logicmill/bstream/file/random/sink.h>
#include <unistd.h>
#include <fcntl.h>

using namespace logicmill;
using namespace bstream;

file::random::sink::sink( sink&& rhs )
:
bstream::random::sink{ std::move( rhs ) },
m_buf{ std::move( rhs.m_buf ) },
m_filename{ std::move( rhs.m_filename ) },
m_is_open{ rhs.m_is_open },
m_mode{ rhs.m_mode },
m_flags{ rhs.m_flags },
m_fd{ rhs.m_fd }
{}

file::random::sink::sink( std::string const& filename, open_mode mode, std::error_code& err, size_type buffer_size )
:
bstream::random::sink{},
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

file::random::sink::sink( std::string const& filename, open_mode mode, size_type buffer_size )
:
bstream::random::sink{},
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

file::random::sink::sink( open_mode mode, size_type buffer_size )
:
bstream::random::sink{},
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
file::random::sink::open( std::string const& filename, open_mode mode, std::error_code& err )
{
	m_filename = filename;
	m_mode = mode;
	m_flags = to_flags( mode );
	really_open( err );
}

void
file::random::sink::open( std::string const& filename, open_mode mode )
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
file::random::sink::open( std::string const& filename, open_mode mode, int flags_override, std::error_code& err )
{
	m_filename = filename;
	m_mode = mode;
	m_flags = flags_override;
	really_open( err );
}

void
file::random::sink::open( std::string const& filename, open_mode mode, int flags_override )
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

void
file::random::sink::really_flush( std::error_code& err )
{
    err.clear();
    auto pos = ppos();
    assert( m_dirty && m_next > m_dirty_start );
    assert( m_dirty_start == m_base );

	size_type n = static_cast< size_type >( m_next - m_base );
	auto write_result = ::write( m_fd, m_base, n );
	if ( write_result < 0 )
	{
		err = std::error_code{ errno, std::generic_category() };
		goto exit;
	}
	assert( static_cast< size_type >( write_result ) == n );
	m_base_offset = pos;
	m_next = m_base;
exit:
    return;
}

bool
file::random::sink::is_valid_position( position_type pos ) const
{
    return pos >= 0;
}

void
file::random::sink::really_jump( std::error_code& err )
{
	err.clear();
	assert( m_did_jump );

	if ( m_dirty )
	{
		flush( err );
	}

    auto result = ::lseek( m_fd, m_jump_to, SEEK_SET );
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        goto exit;
    }

	m_base_offset = m_jump_to;
	assert( ! m_dirty );
    assert( ppos() == m_jump_to );
    assert( m_next == m_base );

exit:
	m_did_jump = false;
	return;
}

void
file::random::sink::really_overflow( size_type, std::error_code& err )
{
    err.clear();
    assert( m_base_offset == ppos() && m_next == m_base );
}

void
file::random::sink::close( std::error_code& err )
{
    err.clear();
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
file::random::sink::close()
{
    std::error_code err;
    close( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}

void
file::random::sink::open()
{
    std::error_code err;
    really_open( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}

position_type
file::random::sink::truncate( std::error_code& err )
{
    err.clear();
    position_type result = bstream::npos;

    flush( err );
    if ( err ) goto exit;

    {
        auto pos = ppos();
        assert( pos == m_base_offset );
        auto trunc_result = ::ftruncate( m_fd, pos );
        if ( trunc_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }

        force_high_watermark( pos );
        result = pos;
    }

exit:
    return result;
}

position_type
file::random::sink::truncate()
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
file::random::sink::really_open( std::error_code& err )
{
    err.clear();
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
            m_base_offset = end_pos;
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
            m_base_offset = 0;
        }
        reset_ptrs();
        m_dirty = false;
    }

exit:
    return;
}

int
file::random::sink::to_flags( open_mode mode )
{
	switch ( mode )
	{
		case open_mode::append:
			return O_WRONLY | O_CREAT | O_APPEND;
		case open_mode::truncate:
			return O_WRONLY | O_CREAT | O_TRUNC;
		default:
			return O_WRONLY | O_CREAT;
	}
}

bool
file::random::sink::is_truncate( int flags )
{
	return ( flags & O_TRUNC ) != 0;
}

bool
file::random::sink::is_append( int flags )
{
	return ( flags & O_APPEND ) != 0;
}
