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


#ifndef LOGICMILL_BSTREAM_MEMORY_SIMPLE_RANDOM_SOURCE_H
#define LOGICMILL_BSTREAM_MEMORY_SIMPLE_RANDOM_SOURCE_H

#include <logicmill/bstream/memory/simple/sequential/source.h>
#include <logicmill/bstream/random/source_base.h>

#ifndef LOGICMILL_BSTREAM_DEFAULT_FILE_BUFFER_SIZE
#define LOGICMILL_BSTREAM_DEFAULT_FILE_BUFFER_SIZE  16384UL
#endif

namespace logicmill 
{
namespace bstream 
{
namespace memory
{
namespace simple
{
namespace random
{

namespace detail
{
	template< class Buffer >
	class source_test_probe;
}

template< class Buffer >
class source : public memory::simple::sequential::source< Buffer >, public bstream::random::source_base
{
public:
	using sbase = memory::simple::sequential::source< Buffer >;

	friend class memory::simple::random::detail::source_test_probe< Buffer >;

	source( shared_buffer const& buf )
	:
	memory::simple::sequential::source< Buffer >( buf )
	{}

	source( shared_buffer&& buf )
	:
	memory::simple::sequential::source< Buffer >( std::move( buf ) )
	{}

    source( const_buffer const& buf )
	:
	memory::simple::sequential::source< Buffer >( buf )
	{}

    source( const_buffer&& buf )
	:
	memory::simple::sequential::source< Buffer >( std::move( buf ) )
	{}

	source( mutable_buffer&& buf )
	:
	memory::simple::sequential::source< Buffer >( std::move( buf ) )
	{}

protected:

    virtual position_type
    really_seek( position_type pos, std::error_code& err ) override
	{
		err.clear();
		sbase::m_next = sbase::m_base + pos;
		return pos;
	}

	virtual position_type
	really_get_position() const override
	{
		return sbase::gpos();
	}

	virtual size_type
	really_get_size() const override
	{
		return sbase::m_buf.size();
	}
};

} // namespace random
} // namespace simple
} // namespace memory
} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_MEMORY_SIMPLE_RANDOM_SOURCE_H