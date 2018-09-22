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

#include <logicmill/bstream/file/sequential/sink.h>
#include <logicmill/bstream/file/sequential/source.h>
#include <doctest.h>
#include <logicmill/bstream/error.h>
#include "common.h"
#include <experimental/filesystem>

using namespace logicmill;
using namespace bstream;

namespace fs = std::experimental::filesystem;

TEST_CASE( "logicmill::bstream::file::sequential::sink_to_source [ smoke ] { basic functionality }" )
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

	SUBCASE( "sink write" )
	{
		std::error_code err;
		file::sequential::sink snk{ "test_output/file_seq_0", open_mode::truncate, err, 16 };
		CHECK( ! err );

		CHECK( snk.size() == 0 );

		file::sequential::detail::sink_test_probe probe{ snk };

		CHECK( probe.is_open() );
		CHECK( probe.mode() == open_mode::truncate );
		CHECK( probe.pos() == 0 );
		CHECK( probe.buffer().capacity() == 16 );
		CHECK( probe.buffer().size() == 0 );

		snk.putn( data, 16, err );
		CHECK( ! err );

		CHECK( snk.size() == 16 );

		CHECK( probe.pos() == 16 );
		CHECK( probe.buffer().capacity() == 16 );
		CHECK( probe.buffer().size() == 0 );
		CHECK( probe.next() == probe.end() );

		snk.flush( err );
		CHECK( ! err );

		snk.close( err );
		CHECK( ! err );

		snk.open( "test_output/file_seq_0", open_mode::at_end, err );
		CHECK( ! err );

		CHECK( snk.size() == 16 );

		CHECK( probe.mode() == open_mode::at_end );
		CHECK( probe.pos() == 16 );
		CHECK( probe.base_offset() == 16 );
		CHECK( probe.next() == probe.base() );
		CHECK( probe.buffer().capacity() == 16 );
		CHECK( probe.buffer().size() == 0 );

		snk.putn( &( data[ 16 ] ), 16, err );
		CHECK( ! err );

		CHECK( snk.size() == 32 );

		CHECK( probe.pos() == 32 );
		CHECK( probe.buffer().capacity() == 16 );
		CHECK( probe.base_offset() == 16 );
		CHECK( probe.buffer().size() == 0 );
		CHECK( probe.next() == probe.end() );

		snk.put( data[ 32 ], err );
		CHECK( ! err );

		CHECK( snk.size() == 33 );

		CHECK( probe.pos() == 33 );
		CHECK( probe.buffer().capacity() == 16 );
		CHECK( probe.base_offset() == 32 );
		CHECK( probe.buffer().size() == 0 );
		CHECK( probe.next() == ( probe.base() + 1 ) );

		snk.close( err );
		CHECK( ! err );
		CHECK( ! probe.is_open() );
	}

	SUBCASE( "source read")
	{
		std::error_code err;
		file::sequential::source src{ "test_output/file_seq_0", err, 0, 16 };
		CHECK( ! err );

		file::sequential::detail::source_test_probe probe{ src };
     
		CHECK( probe.pos() == 0 );
		CHECK( src.size() == 33 );
		CHECK( probe.base_offset() == 0 );
		CHECK( probe.base() == probe.next() );
		CHECK( probe.base() == probe.end() );

		byte_type b = src.get( err );
		CHECK( ! err );
		CHECK( b == 0 );

		CHECK( probe.pos() == 1 );
		CHECK( src.size() == 33 );
		CHECK( probe.base_offset() == 0 );
		CHECK( ( probe.base() + 1 ) == probe.next() );
		CHECK( ( probe.base() + 16 ) == probe.end() );

		const_buffer cbuf = src.getn( as_const_buffer{}, 15, err );
		CHECK( ! err );
		CHECK( probe.pos() == 16 );
		CHECK( src.size() == 33 );
		CHECK( probe.base_offset() == 0 );
		CHECK( probe.end() == probe.next() );
		CHECK( ( probe.base() + 16 ) == probe.end() );

		CHECK( cbuf.size() == 15 );
		CHECK( MATCH_BUFFER( cbuf, &( data[ 1 ] ) ) );

		b = src.get( err  );
		CHECK( ! err );
		CHECK( b == 0x10 );

		CHECK( probe.pos() == 17 );
		CHECK( src.size() == 33 );
		CHECK( probe.base_offset() == 16 );
		CHECK( ( probe.base() + 1 ) == probe.next() );
		CHECK( ( probe.base() + 16 ) == probe.end() );

		shared_buffer sbuf = src.getn( as_shared_buffer{}, 17, err );
		CHECK( ! err );
		CHECK( sbuf.size() == 16 );
		CHECK( MATCH_BUFFER( sbuf, &( data[ 17 ] ) ) );

		CHECK( probe.pos() == 33 );
		CHECK( src.size() == 33 );
		CHECK( probe.base_offset() == 33 );
		CHECK( probe.base() == probe.next() );
		CHECK( probe.base() == probe.end() );

		b = src.get( err );
		CHECK( err );
		CHECK( err == bstream::errc::read_past_end_of_stream );

		CHECK( probe.pos() == 33 );
		CHECK( src.size() == 33 );
		CHECK( probe.base_offset() == 33 );
		CHECK( probe.base() == probe.next() );
		CHECK( probe.base() == probe.end() );

		src.close( err );
		CHECK( ! err );

	}
}

TEST_CASE( "logicmill::bstream::file::sequential::sink_to_source [ smoke ] { sink overwrite }" )
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

	SUBCASE( "sink write" )
	{
		std::error_code err;
		file::sequential::sink snk{ "test_output/file_seq_1", open_mode::truncate, err, 16 };
		CHECK( ! err );

		CHECK( snk.size() == 0 );

		file::sequential::detail::sink_test_probe probe{ snk };

		CHECK( probe.is_open() );
		CHECK( probe.mode() == open_mode::truncate );
		CHECK( probe.pos() == 0 );
		CHECK( probe.buffer().capacity() == 16 );
		CHECK( probe.buffer().size() == 0 );

		snk.putn( data, sizeof( data ), err );
		CHECK( ! err );

		CHECK( snk.size() == sizeof( data ) );

		CHECK( probe.pos() == sizeof( data ) );
		CHECK( probe.buffer().capacity() == 16 );
		CHECK( probe.buffer().size() == 0 );
		CHECK( probe.base_offset() == sizeof( data ) - probe.buffer().capacity() );
		CHECK( probe.next() == probe.end() );
		CHECK( probe.next() == probe.base() + probe.buffer().capacity() );

		snk.close( err );
		CHECK( ! err );
	}

	SUBCASE( "source read")
	{
		std::error_code err;
		file::sequential::source src{ "test_output/file_seq_1", err, 0, 16 };
		CHECK( ! err );

		file::sequential::detail::source_test_probe probe{ src };
     
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

	SUBCASE( "sink overwrite" )
	{
		std::error_code err;
		constexpr size_type buf_cap = 16;
		file::sequential::sink snk{ "test_output/file_seq_1", open_mode::at_begin, err, buf_cap };
		CHECK( ! err );

		file::sequential::detail::sink_test_probe probe{ snk };

		CHECK( probe.is_open() );
		CHECK( probe.mode() == open_mode::at_begin );
		CHECK( probe.pos() == 0 );
		CHECK( probe.buffer().capacity() == buf_cap );
		CHECK( probe.buffer().size() == 0 );
		CHECK( snk.size() == sizeof( data ) );

		auto overwrite_size = 24;

		snk.filln( 0xff, overwrite_size, err );
		CHECK( ! err );

		CHECK( probe.pos() == overwrite_size );
		CHECK( probe.base_offset() == ( overwrite_size / buf_cap ) * buf_cap );
		auto residue = overwrite_size % buf_cap;
		CHECK( probe.next() == probe.base() + residue );
		CHECK( probe.end() == probe.base() + buf_cap );

		snk.flush( err );
		CHECK( ! err );
		CHECK( probe.pos() == overwrite_size );
		CHECK( probe.base_offset() == overwrite_size );
		CHECK( probe.next() == probe.base() );

		snk.close( err );
		CHECK( ! err );
	}

	SUBCASE( "sink read overwrite" )
	{
		byte_type expected[] = { 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
			0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
			0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		};

		std::error_code err;
		file::sequential::source src{ "test_output/file_seq_1", err, 0, 16 };
		CHECK( ! err );

		file::sequential::detail::source_test_probe probe{ src };
     
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

}

TEST_CASE( "logicmill::bstream::file::sequential::sink_to_source [ smoke ] { one-byte buffer }" )
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

	SUBCASE( "sink with one-byte buffer")
	{
		std::error_code err;
		file::sequential::sink snk{ "test_output/file_seq_2", open_mode::truncate, err, 1 };
		CHECK( ! err );

		CHECK( snk.size() == 0 );

		file::sequential::detail::sink_test_probe probe{ snk };

		CHECK( probe.is_open() );
		CHECK( probe.mode() == open_mode::truncate );
		CHECK( probe.pos() == 0 );
		CHECK( probe.buffer().capacity() == 1 );
		CHECK( probe.buffer().size() == 0 );

		snk.putn( data, sizeof( data ), err );
		CHECK( ! err );

		CHECK( snk.size() == sizeof( data ) );

		CHECK( probe.pos() == sizeof( data ) );
		CHECK( probe.buffer().capacity() == 1);
		CHECK( probe.buffer().size() == 0 );
		CHECK( probe.base_offset() == sizeof( data ) - probe.buffer().capacity() );
		CHECK( probe.next() == probe.end() );
		CHECK( probe.next() == probe.base() + probe.buffer().capacity() );

		snk.close( err );
		CHECK( ! err );	
	}

	SUBCASE( "source read")
	{
		std::error_code err;
		file::sequential::source src{ "test_output/file_seq_2", err, 0, 1 };
		CHECK( ! err );

		file::sequential::detail::source_test_probe probe{ src };
     
		CHECK( probe.pos() == 0 );
		CHECK( src.size() == sizeof( data ) );
		CHECK( probe.buffer().capacity() == 1 );
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