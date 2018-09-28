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

#include <logicmill/bstream/sequential/sink.h>
#include <logicmill/bstream/sequential/source.h>
#include <logicmill/bstream/error.h>
#include <logicmill/bstream/buffer.h>
#include <doctest.h>
#include "common.h"

using namespace logicmill;
using namespace bstream;

TEST_CASE( "logicmill::bstream::sequential::sink [ smoke ] { basic functionality }" )
{
	byte_type buf[] = { 
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};

	byte_type s[] = { 
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
		0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	};

	byte_type expected_0[] = {
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};
	
	byte_type expected_1[] = {
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};
	
	byte_type expected_2[] = {
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};
	
	byte_type expected_3[] = {
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	
	sequential::sink snk{ buf, sizeof( buf ) };

	bstream::sequential::detail::sink_test_probe probe{ snk };

	std::error_code err;

	snk.putn( &s[0], 8, err );

	CHECK( ! err );
	CHECK( MATCH_MEMORY( buf, expected_0 ) );
	CHECK( probe.pos() == 8 );
	CHECK( snk.size() == 8 );

	snk.filln( 0, 8, err );
	CHECK( ! err );
	CHECK( MATCH_MEMORY( buf, expected_1 ) );
	CHECK( probe.pos() == 16 );
	CHECK( snk.size() == 16 );

	for ( auto i = 16; i < 24; ++i )
	{
		snk.put( s[ i ], err );
		CHECK( ! err );
	}
	CHECK( MATCH_MEMORY( buf, expected_2 ) );
	CHECK( probe.pos() == 24 );
	CHECK( snk.size() == 24 );

	snk.filln( 0, 8, err );
	CHECK( ! err );
	CHECK( MATCH_MEMORY( buf, expected_3 ) );
	CHECK( probe.pos() == 32 );
	CHECK( snk.size() == 32 );

	snk.put( 0xff, err );
	CHECK( err );
	CHECK( err == std::errc::no_buffer_space );
	CHECK( MATCH_MEMORY( buf, expected_3 ) );
	CHECK( probe.pos() == 32 );
	CHECK( snk.size() == 32 );
}

TEST_CASE( "logicmill::bstream::sequential::source [ smoke ] { basic functionality }" )
{
	byte_type buf[] = { 
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};

	sequential::source src{ buf, sizeof( buf ) };

	bstream::sequential::detail::source_test_probe probe{ src };

	std::error_code err;

	CHECK( probe.pos() == 0 );
	CHECK( src.size() == sizeof( buf ) );

	for ( auto i = 0; i < 8; ++i )
	{
		byte_type b = src.get( err );
		CHECK( ! err );
		CHECK( b == buf[ i ] );
		CHECK( probe.pos() == i + 1 );
	}

	const_buffer cbuf = src.get_slice( 8, err );
	CHECK( ! err );
	CHECK( probe.pos() == 16 );
	CHECK( MATCH_BUFFER( cbuf, &buf[ 8 ] ) );

	shared_buffer sbuf = src.get_slice( 8, err );
	CHECK( ! err );
	CHECK( probe.pos() == 24 );

	CHECK( MATCH_BUFFER( sbuf, &buf[ 16 ] ) );

	byte_type bytes[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	auto got = src.getn( bytes, sizeof( bytes ), err );
	CHECK( ! err );
	CHECK( got == 8 );
	CHECK( probe.pos() == 32);

	CHECK( MATCH_MEMORY_N( bytes, &buf[ 24 ], 8 ) );

	byte_type not_there = src.get( err );

	CHECK( err );
	CHECK( err == bstream::errc::read_past_end_of_stream );
	CHECK( probe.pos() == 32);

}