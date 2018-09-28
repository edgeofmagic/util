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

#include <logicmill/bstream/file/random/source.h>
#include <unistd.h>
#include <fcntl.h>

using namespace logicmill;
using namespace bstream;

file::random::source::source( size_type buffer_size )
:
file::sequential::source{ buffer_size }
{}

file::random::source::source( std::string const& filename, std::error_code& err, int flag_overrides, size_type buffer_size )
:
file::sequential::source{ filename, err, flag_overrides, buffer_size }
{
}

file::random::source::source( std::string const& filename, int flag_overrides, size_type buffer_size )
:
file::sequential::source{ filename, flag_overrides, buffer_size }
{
}

position_type
file::random::source::really_seek( position_type pos, std::error_code& err )
{
    err.clear();
    position_type result = bstream::npos;

    result = ::lseek( m_fd, pos, SEEK_SET );

    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        result = bstream::npos;
        goto exit;
    }

    m_base_offset = result;
    reset_ptrs();
exit:
    return result;
}

position_type
file::random::source::really_get_position() const
{
	return gpos();
}

size_type
file::random::source::really_get_size() const
{
	return m_size;
}
