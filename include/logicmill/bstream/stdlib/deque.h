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

#ifndef LOGICMILL_BSTREAM_STDLIB_DEQUE_H
#define LOGICMILL_BSTREAM_STDLIB_DEQUE_H

#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/obstream.h>
#include <deque>

namespace logicmill
{
namespace bstream
{
template< class T, class Alloc >
struct value_deserializer< std::deque< T, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::deque< T, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::deque< T, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::deque< T, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace_back( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};

template< class T, class Alloc >
struct serializer< std::deque< T, Alloc > >
{
	static obstream& put( obstream& os, const std::deque< T, Alloc >& deq )
	{
		os.write_array_header( deq.size() );
		for ( auto it = deq.begin(); it != deq.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_STDLIB_DEQUE_H