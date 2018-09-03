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

#ifndef LOGICMILL_BSTREAM_STDLIB_UNORDERED_SET_H
#define LOGICMILL_BSTREAM_STDLIB_UNORDERED_SET_H

#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/obstream.h>
#include <unordered_set>

namespace logicmill
{
namespace bstream
{

template< class T, class Hash, class Equal, class Alloc >
struct value_deserializer< std::unordered_set< T, Hash, Equal, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::unordered_set< T, Hash, Equal, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unordered_set< T, Hash, Equal, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::unordered_set< T, Hash, Equal, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};	

template< class T, class Hash, class Equal, class Alloc >
struct value_deserializer< std::unordered_multiset< T, Hash, Equal, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::unordered_multiset< T, Hash, Equal, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unordered_multiset< T, Hash, Equal, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::unordered_multiset< T, Hash, Equal, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};	

template< class T, class Hash, class Equal, class Alloc >
struct serializer< std::unordered_set< T, Hash, Equal, Alloc > >
{
	static obstream& put( obstream& os, const std::unordered_set< T, Hash, Equal, Alloc >& s )
	{
		os.write_array_header( s.size() );
		for ( auto it = s.begin(); it != s.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

template< class T, class Hash, class Equal, class Alloc >
struct serializer< std::unordered_multiset< T, Hash, Equal, Alloc > >
{
	static obstream& put( obstream& os, const std::unordered_multiset< T, Hash, Equal, Alloc >& s )
	{
		os.write_array_header( s.size() );
		for ( auto it = s.begin(); it != s.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_STDLIB_UNORDERED_SET_H