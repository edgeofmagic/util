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
#include <iostream>
#include <logicmill/util/buffer.h>

using namespace logicmill;

TEST_CASE("logicmill::util::mutable_buffer [ smoke ] { basic functionality }")
{
	util::mutable_buffer mbuf{16};
	CHECK(mbuf.size() == 0);
	CHECK(mbuf.capacity() == 16);

	CHECK(mbuf.is_expandable());

	std::string contents{"abcdefghijklmnop"};

	mbuf.putn(0, contents.data(), contents.size());
	mbuf.size(contents.size());
	std::string_view v = mbuf.as_string();
	CHECK(v == contents);
	mbuf.expand(12);
	CHECK(mbuf.capacity() == 16);
	mbuf.expand(32);
	CHECK(mbuf.capacity() == 32);
	std::string more_contents{"qrstuvwxyz"};
	mbuf.putn(16, more_contents.data(), more_contents.size());
	mbuf.size(contents.size() + more_contents.size());
	std::string total = mbuf.to_string();
	CHECK(total == contents + more_contents);

	auto original_data = mbuf.data();

	util::mutable_buffer mbuf_2{std::move(mbuf)};

	CHECK(mbuf_2.data() == original_data);

	std::string_view v2 = mbuf_2.as_string();

	CHECK(v2 == total);

	// CHECK( mbuf.data() == nullptr );
	// CHECK( mbuf.capacity() == 0 );
	// CHECK( mbuf.size() == 0 );

	mbuf = util::mutable_buffer{std::allocator<byte_type>{}};

	CHECK(mbuf.is_expandable());

	CHECK(mbuf.data() == nullptr);
	CHECK(mbuf.capacity() == 0);
	CHECK(mbuf.size() == 0);

	mbuf.expand(total.size());

	CHECK(mbuf.data() != nullptr);
	CHECK(mbuf.capacity() == total.size());
}

TEST_CASE("logicmill::util::mutable_buffer logicmill::util::const_buffer [ smoke ] { move mutable to const }")
{
	std::string          contents{"abcdefghijklmnopqrstuvwxyz"};
	util::mutable_buffer mbuf{contents.size()};
	mbuf.putn(0, contents.data(), contents.size());
	CHECK(mbuf.data() != nullptr);
	CHECK(mbuf.capacity() == contents.size());
	CHECK(mbuf.size() == 0);
	mbuf.size(contents.size());
	CHECK(mbuf.size() == contents.size());

	mbuf.size(7);
	CHECK(mbuf.as_string() == "abcdefg");

	util::const_buffer cbuf{mbuf};

	CHECK(cbuf.as_string() == "abcdefg");

	void* orig_data = mbuf.data();

	util::const_buffer cbuf2{std::move(mbuf)};

	CHECK(orig_data == (void*)cbuf2.data());

	// CHECK( mbuf.capacity() == 0);
	// CHECK( mbuf.data() == nullptr );
}

#if 0

TEST_CASE( "logicmill::util::mutable_buffer [ smoke ] { no_realloc_broker }" )
{
	util::mutable_buffer mbuf{ 128, buffer::no_realloc_broker::get() };
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

TEST_CASE("logicmill::util::mutable_buffer [ smoke ] { null_delete }")
{
	byte_type            blk[32];
	util::mutable_buffer mbuf{
			&blk, 32, util::null_delete<byte_type[]>{}};    // sanitizer will go nuts if stack gets deallocated
}

TEST_CASE("logicmill::util::const_buffer [ smoke ] { dealloc only broker }")
{
	std::string contents{"some buffer contents"};
	// buffer::memory_broker::ptr broker = buffer::default_broker::get();
	auto block = new byte_type[1024];
	memcpy(block, contents.data(), contents.size());
	util::const_buffer cbuf{block, contents.size(), std::default_delete<byte_type[]>{}};
}


TEST_CASE("logicmill/buffer/smoke/basic")
{
	std::string          s1{"This is some buffer content."};
	std::string          s2{" This is more buffer content."};
	util::mutable_buffer buf{s1.size()};
	buf.putn(0, s1.data(), s1.size());
	CHECK(buf.capacity() == s1.size());
	buf.size(buf.capacity());

	util::const_buffer cbuf{buf};

	CHECK(buf == cbuf);
	CHECK(buf.data() != cbuf.data());

	void* baddr0 = buf.data();

	buf.expand(s1.size() + s2.size());

	CHECK(buf.size() == s1.size());

	buf.putn(buf.size(), s2.data(), s2.size());

	buf.size(buf.capacity());

	void* baddr1 = buf.data();

	// std::cout << "baddr0 is " << baddr0 << ", baddr1 is " << baddr1 << std::endl;

	std::cout << buf.to_string() << std::endl;

	CHECK(buf.to_string() == s1 + s2);
}

TEST_CASE("logicmill/buffer/smoke/slice")
{
	std::string          s1{"This is some buffer content."};
	util::mutable_buffer buf{s1.size()};
	buf.putn(0, s1.data(), s1.size());

	buf.size(buf.capacity());
	void* baddr0 = buf.data();

	util::shared_buffer cbuf{std::move(buf)};
	const void*         baddr1 = cbuf.data();

	// CHECK( buf.ref_count() == 0 );
	CHECK(cbuf.ref_count() == 1);
	CHECK(baddr0 == baddr1);

	auto slice = cbuf.slice(13, 6);

	CHECK(slice.size() == 6);
	CHECK(slice.to_string() == "buffer");
	CHECK(slice.ref_count() == 2);
}

TEST_CASE("logicmill/buffer/smoke/string_alias")
{
	std::string          s1{"This is some buffer content."};
	util::mutable_buffer buf{s1.size()};
	buf.putn(0, s1.data(), s1.size());

	buf.size(buf.capacity());
	void* baddr0 = buf.data();

	util::shared_buffer cbuf{std::move(buf)};
	const void*         baddr1 = cbuf.data();

	// CHECK( buf.ref_count() == 0 );
	CHECK(cbuf.ref_count() == 1);
	CHECK(baddr0 == baddr1);

	auto slice = cbuf.slice(13, 6);

	CHECK(slice.size() == 6);
	CHECK(slice.to_string() == "buffer");
	CHECK(slice.ref_count() == 2);

	util::shared_buffer cbuf1{cbuf};
	const void*         baddr2 = cbuf1.data();
	CHECK(baddr2 == baddr1);
	CHECK(cbuf1.ref_count() == 3);
	CHECK(slice.ref_count() == 3);
}

TEST_CASE("logicmill::util::buffer [ smoke ] { consolidating ctor mutable_buffer }")
{
	{
		std::deque<util::mutable_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::mutable_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}
	{
		std::deque<util::const_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::mutable_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}

	{
		std::deque<util::shared_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::mutable_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}

	{
		std::deque<util::shared_buffer> bufs;
		util::mutable_buffer consolidated(bufs);
		CHECK(consolidated.size() == 0);
	}
}

TEST_CASE("logicmill::util::buffer [ smoke ] { consolidating ctor const_buffer }")
{
	{
		std::deque<util::mutable_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::const_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}
	{
		std::deque<util::const_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::const_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}

	{
		std::deque<util::shared_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::const_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}

	{
		std::deque<util::mutable_buffer> bufs;
		util::const_buffer consolidated(bufs);
		CHECK(consolidated.size() == 0);
	}
}

TEST_CASE("logicmill::util::buffer [ smoke ] { consolidating ctor shared_buffer }")
{
	{
		std::deque<util::mutable_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::shared_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}
	{
		std::deque<util::const_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::shared_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}

	{
		std::deque<util::shared_buffer> bufs;
		bufs.emplace_back(util::mutable_buffer{"This "});
		bufs.emplace_back(util::mutable_buffer{"is "});
		bufs.emplace_back(util::mutable_buffer{"a "});
		bufs.emplace_back(util::mutable_buffer{"set "});
		bufs.emplace_back(util::mutable_buffer{"of "});
		bufs.emplace_back(util::mutable_buffer{"mutable "});
		bufs.emplace_back(util::mutable_buffer{"buffers."});
		util::shared_buffer consolidated(bufs);
		CHECK(consolidated.to_string() == "This is a set of mutable buffers.");
	}

	{
		std::deque<util::shared_buffer> bufs;
		util::shared_buffer consolidated(bufs);
		CHECK(consolidated.size() == 0);
	}
}

TEST_CASE("logicmill::util::buffer [ smoke ] { fixed region }")
{
	util::mutable_buffer mbuf{util::buffer::fixed_region_factory<1024>{}};
	CHECK(mbuf.capacity() == 1024 );
	std::error_code err;
	mbuf.expand(1025, err);
	CHECK(err);
	CHECK(err == std::errc::operation_not_supported);
	CHECK(!mbuf.is_expandable());
}

TEST_CASE("logicmill::util::buffer [ smoke ] { binned fixed region factory }")
{
	util::buffer::binned_fixed_region_factory factory;
	auto reg = factory.create(1);
	CHECK(reg->capacity() == 16);
	reg = factory.create(16);
	CHECK(reg->capacity() == 16);
	reg = factory.create(17);
	CHECK(reg->capacity() == 24);
	reg = factory.create(24);
	CHECK(reg->capacity() == 24);
	reg = factory.create(25);
	CHECK(reg->capacity() == 32);
	reg = factory.create(32);
	CHECK(reg->capacity() == 32);
	reg = factory.create(33);
	CHECK(reg->capacity() == 48);

	reg = factory.create(1048576);
	CHECK(reg->capacity() == 1048576);
	reg = factory.create(1048577);
	CHECK(reg->capacity() == 1572864);

	reg = factory.create(16777216);
	CHECK(reg->capacity() == 16777216);
	try
	{
		reg = factory.create(16777217);
		CHECK(false);
	}
	catch(const std::invalid_argument& e)
	{
		CHECK(std::string{e.what()} == "size exceeds maximum");
	}
}
