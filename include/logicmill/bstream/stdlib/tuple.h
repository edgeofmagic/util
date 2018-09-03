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

#ifndef LOGICMILL_BSTREAM_STDLIB_TUPLE_H
#define LOGICMILL_BSTREAM_STDLIB_TUPLE_H

#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/obstream.h>
#include <tuple>

namespace logicmill
{
namespace bstream
{

template< class... Args >
struct value_deserializer< std::tuple< Args... >,
        std::enable_if_t< utils::conjunction< is_ibstream_readable< Args >::value... >::value > >
{
    using tuple_type = std::tuple< Args... >;
    
    tuple_type 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static tuple_type
    get( ibstream& is )
    {
        is.check_array_header( std::tuple_size< tuple_type >::value );
        tuple_type tup;
        get_members< 0, Args... >( is, tup );
        return tup;
    }
    
    template< unsigned int N, class First, class... Rest >
    static 
    typename std::enable_if< ( sizeof...( Rest ) > 0 ) >::type
    get_members( ibstream& is, tuple_type& tup )
    {
        is >> std::get< N >( tup );
        get_members< N+1, Rest... >( is, tup );
    }
    
    template < unsigned int N, class T >
    static void
    get_members( ibstream& is, tuple_type& tup )
    {
        is >> std::get< N >( tup );
    }
};

template< class... Args >
struct serializer< std::tuple< Args... > >
{
	using tuple_type = std::tuple< Args... >;
	static obstream& put( obstream& os, tuple_type const& tup )
	{
		os.write_array_header( std::tuple_size< tuple_type >::value );
		put_members< 0, Args... >( os, tup );
		return os;
	}
	
	template< unsigned int N, class First, class... Rest >
	static typename std::enable_if< ( sizeof...( Rest ) > 0 ) >::type
	put_members( obstream& os, tuple_type const& tup )
	{
		os << std::get< N >( tup );
		put_members< N+1, Rest... >( os, tup );
	}
	
	template< unsigned int N, class T >
	static void
	put_members( obstream& os, tuple_type const& tup )
	{
		os << std::get< N >( tup );
	}		
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_STDLIB_TUPLE_H