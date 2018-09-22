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

#ifndef LOGICMILL_BSTREAM_RANDOM_SOURCE_BASE_H
#define LOGICMILL_BSTREAM_RANDOM_SOURCE_BASE_H

#include <logicmill/bstream/buffer.h>
#include <logicmill/bstream/types.h>
#include <logicmill/bstream/sequential/source.h>

namespace logicmill 
{
namespace bstream 
{
namespace random
{

class source_base
{

public:

    position_type
    position( position_type position, std::error_code& err );

    position_type
    position( position_type position );

    position_type
    position( offset_type offset, seek_anchor where, std::error_code& err );

    position_type
    position( offset_type offset, seek_anchor where );

    position_type
    position() const;

protected:

	position_type
	new_position( offset_type offset, seek_anchor where ) const;

    virtual position_type
    really_seek( position_type pos, std::error_code& err ) = 0;

	virtual position_type
	really_get_position() const = 0;

	virtual size_type
	really_get_size() const = 0;

};

} // namespace random
} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_RANDOM_SOURCE_BASE_H
