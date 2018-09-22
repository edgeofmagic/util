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

#include <logicmill/bstream/ibsharedbuf.h>

using namespace logicmill;
using namespace bstream;

shared_buffer
ibsharedbuf::slice( size_type n )
{
    size_type available = static_cast< size_type >( m_gend - m_gnext );
    size_type slice_size = std::min( available, n );
    if ( slice_size < 1 )
    {
        return shared_buffer{};
    }
    else
    {
        auto pos = gpos();
        gbump( n );
        return shared_buffer{ m_buf, pos, n };
    }
}

shared_buffer
ibsharedbuf::getn( as_shared_buffer, size_type n, std::error_code& err )
{
    err.clear();
    return slice( n );
}

shared_buffer
ibsharedbuf::getn( as_shared_buffer, size_type n )
{
    return slice( n );
}

