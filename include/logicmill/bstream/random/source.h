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

#ifndef LOGICMILL_BSTREAM_RANDOM_SOURCE_H
#define LOGICMILL_BSTREAM_RANDOM_SOURCE_H

#include <logicmill/bstream/buffer.h>
#include <logicmill/bstream/types.h>
#include <logicmill/bstream/sequential/source.h>
#include <logicmill/bstream/random/source_base.h>

namespace logicmill 
{
namespace bstream 
{
namespace random
{

namespace detail
{
    class source_test_probe;
}

class source : public sequential::source, public random::source_base
{

public:

	using base = sequential::source;

	friend class detail::source_test_probe;

    source( const byte_type * buf, size_type size )
    :
	base{ buf, size }
    {}

    source( source const& ) = delete;
    source& operator=( source&& ) = delete;
    source& operator=( source const& ) = delete;

protected:

    source()
    : 
	source{ nullptr, 0 }
    {}

	source( source&& rhs )
	:
	base{ std::move( rhs ) }
	{}

public:

    virtual position_type
    really_seek( position_type pos, std::error_code& err ) override;

	virtual position_type
	really_get_position() const override;

	virtual size_type
	really_get_size() const override;

};

} // namespace random
} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_RANDOM_SOURCE_H
