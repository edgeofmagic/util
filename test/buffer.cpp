#include <logicmill/buffer.h>
#include "doctest.h"
#include <iostream>

using namespace logicmill;

TEST_CASE( "logicmill/buffer/smoke/basic" )
{
	std::string s1{ "This is some buffer content." };
	std::string s2{ " This is more buffer content." };
	mutable_buffer buf{ s1 };
	CHECK( buf.capacity() == s1.size() );
	buf.size( buf.capacity() );

	const_buffer cbuf{ buf };

	CHECK( buf == cbuf );
	CHECK( buf.data() != cbuf.data() );

	void* baddr0 = buf.data();

	buf.capacity( s1.size() + s2.size() );

	CHECK( buf.size() == s1.size() );

	buf.putn( buf.size(), s2.data(), s2.size() );

	buf.size( buf.capacity() );

	void* baddr1 = buf.data();

	std::cout << "baddr1 is " << baddr0 << ", baddr1 is " << baddr1 << std::endl;

	std::cout << buf.to_string() << std::endl;

	CHECK( buf.to_string() == s1 + s2 );
}

TEST_CASE( "logicmill/buffer/smoke/slice" )
{
	std::string s1{ "This is some buffer content." };
	mutable_buffer buf{ s1 };

	buf.size( buf.capacity() );
	void* baddr0 = buf.data();

	const_buffer cbuf{ std::move( buf ) };
	const void* baddr1 = cbuf.data();

	CHECK( buf.ref_count() == 0 );
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
	mutable_buffer buf{ s1 };

	buf.size( buf.capacity() );
	void* baddr0 = buf.data();

	const_buffer cbuf{ std::move( buf ) };
	const void* baddr1 = cbuf.data();

	CHECK( buf.ref_count() == 0 );
	CHECK( cbuf.ref_count() == 1 );
	CHECK( baddr0 == baddr1 );

	auto slice = cbuf.slice( 13, 6 );

	CHECK( slice.size() == 6 );
	CHECK( slice.to_string() == "buffer" );
	CHECK( slice.ref_count() == 2 );
	
	const_buffer cbuf1{ reinterpret_cast< buffer& >( cbuf ) };
	const void* baddr2 = cbuf1.data();
	CHECK( baddr2 != baddr1 );
	CHECK( cbuf1.ref_count() == 1 );
	CHECK( slice.ref_count() == 2 );
}
