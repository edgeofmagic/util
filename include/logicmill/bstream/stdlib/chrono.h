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

#ifndef LOGICMILL_BSTREAM_STDLIB_CHRONO_H
#define LOGICMILL_BSTREAM_STDLIB_CHRONO_H

#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/obstream.h>
#include <chrono>

namespace logicmill
{
namespace bstream
{

template< class Rep, class Ratio >
struct value_deserializer< std::chrono::duration< Rep,Ratio > >
{
    using duration_type = std::chrono::duration< Rep,Ratio >;

    duration_type 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static duration_type
    get( ibstream&is )
    {
        auto count = is.read_as< duration_type::rep >();
        return duration_type{ count };
    }
};

template< class Clock, class Duration >
struct value_deserializer< std::chrono::time_point< Clock,Duration > >
{
    using time_point_type = std::chrono::time_point< Clock,Duration >;

    time_point_type 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static time_point_type
    get( ibstream& is )
    {
        auto ticks = is.read_as< typename time_point_type::rep >();
        return time_point_type( typename time_point_type::duration( ticks ) );
    }
};

template< class Rep, class Ratio >
struct serializer< std::chrono::duration< Rep,Ratio > >
{
	using duration_type = std::chrono::duration< Rep,Ratio >;
	static obstream& put( obstream& os, duration_type val )
	{
		os << val.count();
		return os;
	}
};

template< class Clock, class Duration >
struct serializer< std::chrono::time_point< Clock,Duration > >
{
	using time_point_type = std::chrono::time_point< Clock,Duration >;
	static obstream& put( obstream& os, time_point_type val )
	{
		os << val.time_since_epoch().count();
		return os;
	}
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_STDLIB_CHRONO_H