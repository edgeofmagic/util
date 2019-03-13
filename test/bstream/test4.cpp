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

#include <doctest.h>
#include <logicmill/bstream.h>
#include <logicmill/bstream/ombstream.h>
#include <logicmill/bstream/imbstream.h>
#include <logicmill/bstream/stdlib.h>
#include <utility>
#include <thread>
#include <chrono>


using namespace logicmill;

namespace test_4
{
struct struct_A
{
    struct_A() {}
    struct_A( int v_0, float v_1, std::string const& v_2, std::vector< unsigned int > const v_3 )
    : m_0{ v_0 }, m_1{ v_1 }, m_2{ v_2 }, m_3{ v_3 } {} 
    int m_0;
    float m_1;
    std::string m_2;
    std::vector< unsigned int > m_3;

    friend bool operator==( struct_A const& a, struct_A const& b )
    {
        return 
            a.m_0 == b.m_0 &&
            a.m_1 == b.m_1 &&
            a.m_2 == b.m_2 &&
            a.m_3 == b.m_3;
    }

};


} // namespace test_4


template<>
struct bstream::serializer< test_4::struct_A >
{
	static inline bstream::obstream& put( bstream::obstream& os, test_4::struct_A const& obj )
	{
		os.write_array_header( 4 );
		os << obj.m_0 << obj.m_1 << obj.m_2 << obj.m_3;
		return os;
	}
};

template<>
struct bstream::ref_deserializer< test_4::struct_A >
{
	static inline bstream::ibstream& get( bstream::ibstream& is, test_4::struct_A& obj )
	{
		is.check_array_header( 4 );
		is >> obj.m_0 >> obj.m_1 >> obj.m_2 >> obj.m_3;
		return is;
	}
};

using namespace test_4;

TEST_CASE( "logicmill/smoke/bstream/composite_types/0" )
{
//	std::this_thread::sleep_for ( std::chrono::seconds( 10 ) );

	// bstream::context<> stream_context;
	bstream::context_base const& stream_context{bstream::get_default_context()};
	
	bstream::ombstream os{ 1024, stream_context };

    struct_A a0{ -7, 3.5, "zoot", { 1, 1, 2, 3, 5, 8, 13 } };

	os << a0;
	
	bstream::imbstream is{ os.get_buffer(), stream_context };

    struct_A a1;

	is >> a1;

	CHECK( a0 == a1 );

    using p = std::pair< std::string, test_4::struct_A >;

    p p0 = std::make_pair< std::string, test_4::struct_A >( "allures", struct_A{ a0 } );

    os.clear();

    os << p0;

    is.use( os.get_buffer() );

    p p1;

    is >> p1;

    CHECK( p0 == p1 );
}
