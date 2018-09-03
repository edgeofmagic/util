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

#include <logicmill/bstream/numstream.h>
#include <logicmill/bstream/error.h>

using namespace logicmill;
using namespace bstream;

const_buffer
inumstream::getn( size_type nbytes, bool throw_on_incomplete )
{
    const_buffer buf = m_strmbuf->getn( nbytes );
    if ( throw_on_incomplete && buf.size() < nbytes )
    {
        throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
    }
    return buf;
}

const_buffer
inumstream::getn( size_type nbytes, std::error_code& err, bool err_on_incomplete )
{
    clear_error( err );
    const_buffer buf = m_strmbuf->getn( nbytes, err );
    if ( err ) goto exit;

    if ( err_on_incomplete && buf.size() < nbytes )
    {
        err = make_error_code( bstream::errc::read_past_end_of_stream );
        goto exit;
    }
exit:
    return buf;
}

size_type
inumstream::getn( byte_type* dst, size_type nbytes, bool throw_on_incomplete )
{
    auto result = m_strmbuf->getn( dst, nbytes );
    if ( throw_on_incomplete && result < nbytes )
    {
        throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
    }
    return result;
}

size_type
inumstream::getn( byte_type* dst, size_type nbytes, std::error_code& err, bool err_on_incomplete )
{
    clear_error( err );
    auto result = m_strmbuf->getn( dst, nbytes, err );
    if ( err ) goto exit;

    if ( err_on_incomplete && result < nbytes )
    {
        err = make_error_code( bstream::errc::read_past_end_of_stream );
        goto exit;
    }
exit:
    return result;
}