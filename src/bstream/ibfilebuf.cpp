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

#include <logicmill/bstream/ibfilebuf.h>
#include <unistd.h>
#include <fcntl.h>

using namespace logicmill;
using namespace bstream;

ibfilebuf::ibfilebuf( size_type buffer_size )
:
m_buf{ buffer_size },
m_filename{},
m_is_open{ false },
m_flags{ O_RDONLY },
m_fd{ -1 }
{}

ibfilebuf::ibfilebuf( std::string const& filename, std::error_code& err, int flag_overrides, size_type buffer_size )
:
m_buf{ buffer_size },
m_filename{ filename },
m_is_open{ false },
m_flags{ O_RDONLY | flag_overrides },
m_fd{ -1 }
{
	really_open( err );
}

ibfilebuf::ibfilebuf( std::string const& filename, int flag_overrides, size_type buffer_size )
:
m_buf{ buffer_size },
m_filename{ filename },
m_is_open{ false },
m_flags{ O_RDONLY | flag_overrides },
m_fd{ -1 }
{
	std::error_code err;
	really_open( err );
	if ( err )
	{
		throw std::system_error{ err };
	}
}

position_type
ibfilebuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ::lseek( m_fd, offset, SEEK_CUR );
        }
        break;

        case seek_anchor::end:
        {
            result = ::lseek( m_fd, offset, SEEK_END );
        }
        break;

        case seek_anchor::begin:
        {
            result = ::lseek( m_fd, offset, SEEK_SET );
        }
        break;
    }
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        result = invalid_position;
        goto exit;
    }
    m_gbase_offset = result;
    reset_ptrs();
exit:
    return result;
}

position_type
ibfilebuf::really_tell( seek_anchor where, std::error_code& err )
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
            auto save = m_gbase_offset;
            result = ::lseek( m_fd, 0, SEEK_END );
            if ( result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                result = invalid_position;
                goto exit;
            }
            auto restore_result = ::lseek( m_fd, save, SEEK_SET );
            if ( restore_result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                result = invalid_position;
                goto exit;
            }
            assert( restore_result == save );
        }
        break;

        case seek_anchor::begin:
        {
            result = 0;
        }
        break;
    }
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        result = invalid_position;
        goto exit;
    }
exit:
    return result;
}

size_type
ibfilebuf::really_underflow( std::error_code& err )
{
    assert( m_gnext == m_gend );
    m_gbase_offset = gpos();
    m_gnext = m_gbase;
    size_type available = load_buffer( err );
    if ( err ) 
    {
        available = 0;
    }
    return available;
}

void
ibfilebuf::close( std::error_code& err )
{
    clear_error( err );
    if ( m_is_open )
    {
        auto close_result = ::close( m_fd );
        if ( close_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }

        m_is_open = false;
    }

exit:
    return;
}

void
ibfilebuf::close()
{
    if ( m_is_open )
    {
        auto close_result = ::close( m_fd );
        if ( close_result < 0 )
        {
            throw std::system_error{ std::error_code{ errno, std::generic_category() } };
        }

        m_is_open = false;
    }
}

size_type
ibfilebuf::load_buffer( std::error_code& err )
{
    clear_error( err );
    assert( m_gnext == m_gbase );

    auto read_result = ::read( m_fd, const_cast< byte_type * >( m_gbase ), m_buf.capacity() );
    if ( read_result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        read_result = 0;
        goto exit;
    }
	m_buf.size( read_result );
    m_gend = m_gnext + read_result;

exit:
    return read_result;
}

void 
ibfilebuf::really_open( std::error_code& err )
{
    clear_error( err );
    if ( m_is_open )
    {
        close( err );
        if ( err ) goto exit;
        m_is_open = false;
    }

    m_fd = ::open( m_filename.c_str(), m_flags );
    if ( m_fd < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        goto exit;
    }

    m_is_open = true;
    reset_ptrs();

exit:
    return;
}

void
ibfilebuf::open( std::string const& filename, std::error_code& err, int flag_overrides )
{
    m_filename = filename;
    m_flags = O_RDONLY | flag_overrides;
    really_open( err );
}

void
ibfilebuf::open( std::string const& filename, int flag_overrides )
{
    m_filename = filename;
    m_flags = O_RDONLY | flag_overrides;
    std::error_code err;
    really_open( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}
