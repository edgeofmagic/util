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
