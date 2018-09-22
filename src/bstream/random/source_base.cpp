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

#include <logicmill/bstream/random/source.h>
#include <logicmill/bstream/error.h>

using namespace logicmill;
using namespace bstream;

position_type
random::source_base::new_position( offset_type offset, seek_anchor where ) const
{
    position_type result = bstream::npos;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = really_get_position() + offset;
        }
        break;

        case seek_anchor::end:
        {
            result = really_get_size() + offset;
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
random::source_base::position( offset_type offset, seek_anchor where, std::error_code& err )
{
    err.clear();
	position_type result = new_position( offset, where );

    if ( result < 0 || result > ( really_get_size() ) )
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
random::source_base::position( position_type pos )
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
random::source_base::position( offset_type offset, seek_anchor where )
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
random::source_base::position( position_type pos, std::error_code& err )
{
	return position( static_cast< offset_type >( pos ), seek_anchor::begin, err );
}

position_type
random::source_base::position() const
{
	return really_get_position();
}

