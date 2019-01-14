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

#include <logicmill/buffer.h>
#include <doctest.h>
#include <iostream>

using namespace logicmill;

// using fbroker = buffer::functor_broker< buffer::default_allocator, buffer::default_reallocator, buffer::default_deallocator >;

// using lambda_broker = buffer::delegating_broker< 

#if 0

TEST_CASE( "logicmill::buffer::memory_broker [ smoke ]" )
{

	SUBCASE( "default_broker" ) 
	{
		auto bp = buffer::default_broker::get();
		CHECK( bp );
		auto blk = bp->allocate( 16 );
		CHECK( blk );
		bp->deallocate( blk );
	}

	SUBCASE( "no_realloc_broker" ) 
	{
		auto bp = buffer::no_realloc_broker::get();
		CHECK( bp );
		auto blk = bp->allocate( 16 );
		CHECK( ! bp->can_reallocate() );
		CHECK( blk );
		bool exception_caught = false;
		try
		{
			bp->reallocate( blk, 0, 256 );
			CHECK( false );
		}
		catch ( std::system_error const& e )
		{
			exception_caught = true;
			CHECK( e.code() == std::errc::operation_not_supported );
		}
		CHECK( exception_caught );
		bp->deallocate( blk );
	}

	SUBCASE( "null_broker" ) 
	{
		auto bp = buffer::null_broker::get();
		CHECK( ! bp->can_allocate() );
		CHECK( ! bp->can_reallocate() );
		CHECK( ! bp->can_deallocate() );
		{
			bool exception_caught = false;
			try
			{
				bp->allocate( 256 );
				CHECK( false );
			}
			catch ( std::system_error const& e )
			{
				exception_caught = true;
				CHECK( e.code() == std::errc::operation_not_supported );
			}
			CHECK( exception_caught );
		}
		{
			bool exception_caught = false;
			try
			{
				bp->reallocate( nullptr, 0, 0 );
				CHECK( false );
			}
			catch ( std::system_error const& e )
			{
				exception_caught = true;
				CHECK( e.code() == std::errc::operation_not_supported );
			}
			CHECK( exception_caught );
		}
		{
			bool exception_caught = false;
			try
			{
				bp->deallocate( nullptr );
				CHECK( false );
			}
			catch ( std::system_error const& e )
			{
				exception_caught = true;
				CHECK( e.code() == std::errc::operation_not_supported );
			}
			CHECK( exception_caught );
		}
	}	

	SUBCASE( "create_broker with default functors" ) 
	{
		// auto dp = std::make_shared< fbroker >( buffer::default_allocator{}, buffer::default_reallocator{}, buffer::default_deallocator{} );
		auto bp = buffer::create_broker( buffer::default_allocator{}, buffer::default_reallocator{}, buffer::default_deallocator{} );
		CHECK( bp );
		CHECK( bp->can_allocate() );
		CHECK( bp->can_reallocate() );
		CHECK( bp->can_deallocate() );
		auto blk = bp->allocate( 16 );
		CHECK( blk );
		bp->deallocate( blk );
	}

	SUBCASE( "create_broker with 3 lambdas" ) 
	{
		auto bp = buffer::create_broker(
		[] ( size_type cap )
		{
			return new byte_type[ cap ];
		},
		[] ( byte_type* data, size_type preserve, size_type new_cap )
		{
			auto new_data = new byte_type[ new_cap ];
			::memcpy( new_data, data, preserve );
			delete [] data;
			return new_data;
		},
		[] ( byte_type* data )
		{
			delete [] data;
		} );

		CHECK( bp );
		CHECK( bp->can_allocate() );
		CHECK( bp->can_reallocate() );
		CHECK( bp->can_deallocate() );
		auto blk1 = bp->allocate( 32 );
		CHECK( blk1 );
		auto blk2 = bp->reallocate( blk1, 0, 64 );
		CHECK( blk1 != blk2 );
		bp->deallocate( blk2 );
	}

	SUBCASE( "create_broker with 2 lambdas" ) 
	{
		auto bp = buffer::create_broker( 
		[] ( size_type cap )
		{
			return new byte_type[ cap ];
		},
		[] ( byte_type* data )
		{
			delete [] data;
		} );
		CHECK( bp );
		CHECK( bp->can_allocate() );
		CHECK( ! bp->can_reallocate() );
		CHECK( bp->can_deallocate() );
		auto blk1 = bp->allocate( 32 );
		CHECK( blk1 );
		bp->deallocate( blk1 );
	}

	SUBCASE( "create_broker with 1 lambda1" ) 
	{
		auto blk = new byte_type[ 32 ];
		
		auto bp = buffer::create_broker( 
		[] ( byte_type* data )
		{
			delete [] data;
		} );
		CHECK( bp );
		CHECK( ! bp->can_allocate() );
		CHECK( ! bp->can_reallocate() );
		bp->deallocate( blk );
	}

	SUBCASE( "create_broker with null_delete" ) 
	{
		auto blk = new byte_type[ 32 ];
		auto bp = buffer::create_broker( bstream::null_delete<byte_type[]>{} );
		CHECK( bp );
		CHECK( ! bp->can_allocate() );
		CHECK( ! bp->can_reallocate() );
		bp->deallocate( blk );
		delete [] blk;
	}
}
#endif

TEST_CASE( "logicmill::mutable_buffer [ smoke ] { basic functionality }" )
{
	mutable_buffer mbuf{ 16 };
	CHECK( mbuf.size() == 0 );
	CHECK( mbuf.capacity() == 16 );

	CHECK( mbuf.is_expandable() );

	std::string contents{ "abcdefghijklmnop" };

	mbuf.putn( 0, contents.data(), contents.size() );
	mbuf.size( contents.size() );
	std::string_view v = mbuf.as_string();
	CHECK( v == contents );
	mbuf.expand( 12 );
	CHECK( mbuf.capacity() == 16 );
	mbuf.expand( 32 );
	CHECK( mbuf.capacity() == 32 );
	std::string more_contents{ "qrstuvwxyz" };
	mbuf.putn( 16, more_contents.data(), more_contents.size() );
	mbuf.size( contents.size() + more_contents.size() );
	std::string total = mbuf.to_string();
	CHECK( total == contents + more_contents );

	auto original_data = mbuf.data();

	mutable_buffer mbuf_2{ std::move( mbuf ) };

	CHECK( mbuf_2.data() == original_data );

	std::string_view v2 = mbuf_2.as_string();

	CHECK( v2 == total );

	// CHECK( mbuf.data() == nullptr );
	// CHECK( mbuf.capacity() == 0 );
	// CHECK( mbuf.size() == 0 );

	mbuf = mutable_buffer{ std::allocator<byte_type>{} };

	CHECK( mbuf.is_expandable() ); 
 
 	CHECK( mbuf.data() == nullptr );
	CHECK( mbuf.capacity() == 0 );
	CHECK( mbuf.size() == 0 );

	mbuf.expand( total.size() );

	CHECK( mbuf.data() != nullptr );
	CHECK( mbuf.capacity() == total.size() );

}

TEST_CASE( "logicmill::mutable_buffer logicmill::const_buffer [ smoke ] { move mutable to const }" )
{
	std::string contents{ "abcdefghijklmnopqrstuvwxyz" };
	mutable_buffer mbuf{ contents.size() };
	mbuf.putn( 0, contents.data(), contents.size() );
	CHECK( mbuf.data() != nullptr );
	CHECK( mbuf.capacity() == contents.size() );
	CHECK( mbuf.size() == 0 );
	mbuf.size( contents.size() );
	CHECK( mbuf.size() == contents.size() );

	mbuf.size( 7 );
	CHECK( mbuf.as_string() == "abcdefg" );

	const_buffer cbuf{ mbuf };

	CHECK( cbuf.as_string() == "abcdefg" );

	void* orig_data = mbuf.data();

	const_buffer cbuf2{ std::move( mbuf ) };

	CHECK( orig_data == (void*) cbuf2 .data() );

	// CHECK( mbuf.capacity() == 0);
	// CHECK( mbuf.data() == nullptr );
}

#if 0

TEST_CASE( "logicmill::mutable_buffer [ smoke ] { no_realloc_broker }" )
{
	mutable_buffer mbuf{ 128, buffer::no_realloc_broker::get() };
	CHECK( mbuf.data() != nullptr );
	CHECK( mbuf.capacity() == 128 );
	CHECK( mbuf.size() == 0 );

	CHECK( ! mbuf.is_expandable() );

	auto original_data = mbuf.data();

	std::error_code err;
	mbuf.expand( 256, err );
	CHECK( err );
	CHECK( err == std::errc::operation_not_supported );

	CHECK( mbuf.data() == original_data );
	CHECK( mbuf.capacity() == 128 );
	CHECK( mbuf.size() == 0 );

	bool exception_caught = false;
	try
	{
		mbuf.expand( 256 );
		CHECK( false );
	}
	catch ( std::system_error const& e )
	{
		exception_caught = true;
		CHECK( e.code() == std::errc::operation_not_supported );
	}
	CHECK( exception_caught );
 
 	CHECK( mbuf.data() == original_data );
	CHECK( mbuf.capacity() == 128 );
	CHECK( mbuf.size() == 0 );
}

#endif

TEST_CASE( "logicmill::mutable_buffer [ smoke ] { null_delete }" )
{
	byte_type blk[32];
	mutable_buffer mbuf{ &blk, 32, null_delete<byte_type[]>{} }; // sanitizer will go nuts if stack gets deallocated
}

TEST_CASE( "logicmill::const_buffer [ smoke ] { dealloc only broker }" )
{
	std::string contents{ "some buffer contents" };
	// buffer::memory_broker::ptr broker = buffer::default_broker::get();
	auto block = new byte_type[1024];
	memcpy( block, contents.data(), contents.size() );
	const_buffer cbuf{ block, contents.size(), std::default_delete<byte_type[]>{} };
}


TEST_CASE( "logicmill/buffer/smoke/basic" )
{
	std::string s1{ "This is some buffer content." };
	std::string s2{ " This is more buffer content." };
	mutable_buffer buf{ s1.size() };
	buf.putn( 0, s1.data(), s1.size() );
	CHECK( buf.capacity() == s1.size() );
	buf.size( buf.capacity() );

	const_buffer cbuf{ buf };

	CHECK( buf == cbuf );
	CHECK( buf.data() != cbuf.data() );

	void* baddr0 = buf.data();

	buf.expand( s1.size() + s2.size() );

	CHECK( buf.size() == s1.size() );

	buf.putn( buf.size(), s2.data(), s2.size() );

	buf.size( buf.capacity() );

	void* baddr1 = buf.data();

	// std::cout << "baddr0 is " << baddr0 << ", baddr1 is " << baddr1 << std::endl;

	std::cout << buf.to_string() << std::endl;

	CHECK( buf.to_string() == s1 + s2 );
}

TEST_CASE( "logicmill/buffer/smoke/slice" )
{
	std::string s1{ "This is some buffer content." };
	mutable_buffer buf{ s1.size() };
	buf.putn( 0, s1.data(), s1.size() );

	buf.size( buf.capacity() );
	void* baddr0 = buf.data();

	shared_buffer cbuf{ std::move( buf ) };
	const void* baddr1 = cbuf.data();

	// CHECK( buf.ref_count() == 0 );
	CHECK( cbuf.ref_count() == 1 );
	CHECK( baddr0 == baddr1 );

	auto slice = cbuf.slice( 13, 6 );

	CHECK( slice.size() == 6 );
	CHECK( slice.to_string() == "buffer" );
	CHECK( slice.ref_count() == 2 );
}

TEST_CASE( "logicmill/buffer/smoke/string_alias" )
{
	std::string s1{ "This is some buffer content." };
	mutable_buffer buf{ s1.size() };
	buf.putn( 0, s1.data(), s1.size() );

	buf.size( buf.capacity() );
	void* baddr0 = buf.data();

	shared_buffer cbuf{ std::move( buf ) };
	const void* baddr1 = cbuf.data();

	// CHECK( buf.ref_count() == 0 );
	CHECK( cbuf.ref_count() == 1 );
	CHECK( baddr0 == baddr1 );

	auto slice = cbuf.slice( 13, 6 );

	CHECK( slice.size() == 6 );
	CHECK( slice.to_string() == "buffer" );
	CHECK( slice.ref_count() == 2 );
	
	shared_buffer cbuf1{ cbuf };
	const void* baddr2 = cbuf1.data();
	CHECK( baddr2 == baddr1 );
	CHECK( cbuf1.ref_count() == 3 );
	CHECK( slice.ref_count() == 3 );
}
