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

// #include <logicmill/bstream/file/random/sink.h>
#include <logicmill/bstream/file/random/source.h>
#include <logicmill/bstream/file/random/sink.h>
#include <doctest.h>
#include <logicmill/bstream/error.h>
#include "common.h"
#include <experimental/filesystem>

using namespace logicmill;
using namespace bstream;

namespace fs = std::experimental::filesystem;

// TEST_CASE( "logicmill::bstream::file::random::sink [ smoke ] { sink seek write }" )
// {
// 	if ( ! fs::is_directory( "test_output" ) || ! fs::exists( "test_output" ) )
// 	{
//     	fs::create_directory( "test_output" ); 
// 	}

// 	std::error_code err;
// 	file::random::sink snk{ "test_output/file_rand_0", open_mode::truncate, err, 16 };
// 	CHECK( ! err );


// }

TEST_CASE( "logicmill::bstream::file::random::source [ smoke ] { source seek read }" )
{
	byte_type data[] = { 
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	};

	if ( ! fs::is_directory( "test_output" ) || ! fs::exists( "test_output" ) )
	{
    	fs::create_directory( "test_output" ); 
	}

	SUBCASE( "create test file" )
	{
		std::error_code err;
		file::random::sink snk{ "test_output/file_rand_0", open_mode::truncate, err, 16 };
		CHECK( ! err );

		snk.putn( data, sizeof( data ), err );
		CHECK( ! err );

		snk.close( err );
		CHECK( ! err );
	}

	SUBCASE( "source read")
	{
		std::error_code err;
		file::random::source src{ "test_output/file_rand_0", err, 0, 16 };
		CHECK( ! err );

		file::random::detail::source_test_probe probe{ src };
     
		CHECK( probe.pos() == 0 );
		CHECK( src.size() == sizeof( data ) );
		CHECK( probe.base_offset() == 0 );
		CHECK( probe.base() == probe.next() );
		CHECK( probe.base() == probe.end() );

		byte_type read_block[ sizeof( data ) ] = { 0 };

		auto count = src.getn(  read_block, sizeof( data ), err );
		CHECK( ! err );
		CHECK( count == sizeof( data ) );
		CHECK( probe.pos() == sizeof( data ) );
		CHECK( probe.base_offset() == sizeof( data ) - probe.buffer().capacity() );
		CHECK( probe.end() == probe.next() );
		CHECK( ( probe.base() + probe.buffer().capacity() ) == probe.end() );

		CHECK( MATCH_MEMORY( data, read_block ) );

		src.close( err );
		CHECK( ! err );

	}

}


TEST_CASE( "logicmill::bstream::file::random::sink [ smoke ] { sink seek write }" )
{
	byte_type data[] = { 
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	};

	if ( ! fs::is_directory( "test_output" ) || ! fs::exists( "test_output" ) )
	{
    	fs::create_directory( "test_output" ); 
	}

	SUBCASE( "create hole" )
	{
		std::error_code err;
		file::random::sink snk{ "test_output/file_rand_1", open_mode::truncate, err, 16 };
		CHECK( ! err );

		snk.putn( data, 16, err );
		CHECK( ! err );

		snk.position( 32, seek_anchor::current, err );
		CHECK( ! err );

		CHECK( snk.position() == 48 );

		snk.putn( &( data[ 48 ] ), 16, err );
		CHECK( ! err );

		snk.close( err );
		CHECK( ! err );

		byte_type expected[] = { 
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
			0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		};

		file::random::source src{ "test_output/file_rand_1", err, 0, 16 };
		CHECK( ! err );

		file::random::detail::source_test_probe probe{ src };
     
		CHECK( probe.pos() == 0 );
		CHECK( src.size() == sizeof( data ) );
		CHECK( probe.base_offset() == 0 );
		CHECK( probe.base() == probe.next() );
		CHECK( probe.base() == probe.end() );

		byte_type read_block[ sizeof( data ) ] = { 0 };

		auto count = src.getn(  read_block, sizeof( data ), err );
		CHECK( ! err );
		CHECK( count == sizeof( data ) );
		CHECK( probe.pos() == sizeof( data ) );
		CHECK( probe.base_offset() == sizeof( data ) - probe.buffer().capacity() );
		CHECK( probe.end() == probe.next() );
		CHECK( ( probe.base() + probe.buffer().capacity() ) == probe.end() );

		CHECK( MATCH_MEMORY( expected, read_block ) );

		src.close( err );
		CHECK( ! err );

	}

	SUBCASE( "fill hole in reverse" )
	{
		std::error_code err;
		file::random::sink snk{ "test_output/file_rand_1", open_mode::truncate, err, 16 };
		CHECK( ! err );

		CHECK( snk.size() == 0 );

		snk.putn( data, 16, err );
		CHECK( ! err );

		CHECK( snk.size() == 16 );

		snk.position( 32, seek_anchor::current, err );
		CHECK( ! err );

		CHECK( snk.size() == 16 );

		CHECK( snk.position() == 48 );

		snk.putn( &( data[ 48 ] ), 16, err );
		CHECK( ! err );

		CHECK( snk.size() == 64 );

		position_type p = 47;
		snk.position( p, err );
		CHECK( ! err );

		for ( auto i = 47; i >= 16; -- i )
		{
			snk.put( data[ i ], err );
			CHECK( ! err );
			snk.position( -2, seek_anchor::current, err );
			CHECK( ! err );
			
			CHECK( snk.size() == 64 );

		}

		snk.close( err );
		CHECK( ! err );

		byte_type expected[] = { 
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
			0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		};

		file::random::source src{ "test_output/file_rand_1", err, 0, 16 };
		CHECK( ! err );

		file::random::detail::source_test_probe probe{ src };
     
		CHECK( probe.pos() == 0 );
		CHECK( src.size() == sizeof( data ) );
		CHECK( probe.base_offset() == 0 );
		CHECK( probe.base() == probe.next() );
		CHECK( probe.base() == probe.end() );

		byte_type read_block[ sizeof( data ) ] = { 0 };

		auto count = src.getn(  read_block, sizeof( data ), err );
		CHECK( ! err );
		CHECK( count == sizeof( data ) );
		CHECK( probe.pos() == sizeof( data ) );
		CHECK( probe.base_offset() == sizeof( data ) - probe.buffer().capacity() );
		CHECK( probe.end() == probe.next() );
		CHECK( ( probe.base() + probe.buffer().capacity() ) == probe.end() );

		CHECK( MATCH_MEMORY( data, read_block ) );

		src.close( err );
		CHECK( ! err );

	}
}

