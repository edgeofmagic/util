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


#ifndef LOGICMILL_BSTREAM_FILE_RANDOM_SOURCE_H
#define LOGICMILL_BSTREAM_FILE_RANDOM_SOURCE_H

#include <logicmill/bstream/file/sequential/source.h>
#include <logicmill/bstream/random/source_base.h>

#ifndef LOGICMILL_BSTREAM_DEFAULT_FILE_SOURCE_BUFFER_SIZE
#define LOGICMILL_BSTREAM_DEFAULT_FILE_SOURCE_BUFFER_SIZE  16384UL
#endif

namespace logicmill 
{
namespace bstream 
{
namespace file
{
namespace random
{

namespace detail
{
	class source_test_probe;
}

class source : public file::sequential::source, public bstream::random::source_base
{
public:

	friend class file::random::detail::source_test_probe;

    source( size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_FILE_SOURCE_BUFFER_SIZE );

    source( std::string const& filename, std::error_code& err, int flag_overrides = 0, size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_FILE_SOURCE_BUFFER_SIZE );

    source( std::string const& filename, int flag_overrides = 0, size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_FILE_SOURCE_BUFFER_SIZE );

protected:

    virtual position_type
    really_seek( position_type pos, std::error_code& err ) override;

	virtual position_type
	really_get_position() const override;

	virtual size_type
	really_get_size() const override;

};

} // namespace random
} // namespace file
} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_FILE_RANDOM_SOURCE_H