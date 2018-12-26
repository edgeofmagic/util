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

// #include <logicmill/bstream/memory/source.h>
// #include <logicmill/bstream/memory/sink.h>
#include "test_probes/memory.h"
#include "common.h"
#include <doctest.h>
#include <logicmill/bstream/buffer.h>
#include <logicmill/bstream/error.h>

using namespace logicmill;
using namespace bstream;


TEST_CASE("logicmill::bstream::memory::sink [ smoke ] { basic functionality }")
{
	byte_type buf[] = {
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};

	byte_type s[] = {
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	};

	byte_type expected_0[] = {
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};

	byte_type expected_1[] = {
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};

	byte_type expected_2[] = {
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};

	byte_type expected_3[] = {
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	mutable_buffer mbuf{buf, sizeof(buf), buffer::null_deallocator{}};

	memory::sink snk{std::move(mbuf)};

	memory::detail::sink_test_probe probe{snk};

	std::error_code err;

	snk.putn(&s[0], 8, err);

	CHECK(!err);
	CHECK(MATCH_MEMORY(buf, expected_0));

	CHECK(probe.pos() == 8);

	CHECK(probe.dirty());
	CHECK(probe.dirty_start_position() == 0);

	snk.position(8, seek_anchor::current, err);
	CHECK(!err);
	CHECK(!probe.dirty());

	CHECK(snk.size() == 8);
	CHECK(probe.pos() == 8);
	CHECK(probe.jump_to() == 16);
	CHECK(probe.did_jump());

	CHECK(MATCH_MEMORY(buf, expected_0));

	snk.putn(&(s[16]), 8, err);
	CHECK(probe.dirty());
	CHECK(probe.dirty_start_position() == 8);
	CHECK(probe.pos() == 24);
	CHECK(snk.size() == 24);
	CHECK(probe.hwm() == 8);

	CHECK(MATCH_MEMORY(buf, expected_1));

	snk.position(8, err);
	CHECK(!err);
	CHECK(!probe.dirty());
	CHECK(probe.hwm() == 24);
	CHECK(snk.size() == 24);
	CHECK(probe.pos() == 24);
	CHECK(snk.position() == 8);
	CHECK(probe.did_jump());
	CHECK(probe.jump_to() == 8);

	for (auto i = 8; i < 16; ++i)
	{
		snk.put(s[i]);
	}

	CHECK(probe.dirty());
	CHECK(probe.dirty_start_position() == 8);
	CHECK(probe.hwm() == 24);
	CHECK(snk.size() == 24);
	CHECK(probe.pos() == 16);
	CHECK(snk.position() == 16);

	CHECK(MATCH_MEMORY(buf, expected_2));

	snk.flush(err);
	CHECK(!err);

	CHECK(!probe.dirty());
	CHECK(probe.hwm() == 24);
	CHECK(snk.size() == 24);
	CHECK(probe.pos() == 16);
	CHECK(snk.position() == 16);

	snk.position(8, seek_anchor::end, err);
	CHECK(!err);
	CHECK(!probe.dirty());
	CHECK(probe.hwm() == 24);
	CHECK(snk.size() == 24);
	CHECK(probe.pos() == 16);
	CHECK(snk.position() == 32);
	CHECK(probe.did_jump());
	CHECK(probe.jump_to() == 32);

	snk.put(0xff, err);

	CHECK(err);
	CHECK(err == std::errc::no_buffer_space);

	CHECK(!probe.dirty());    // overflow clears dirty
	CHECK(probe.hwm() == 32);
	CHECK(snk.size() == 32);
	CHECK(probe.pos() == 32);
	CHECK(snk.position() == 32);
	CHECK(!probe.did_jump());

	CHECK(MATCH_MEMORY(buf, expected_3));
}

TEST_CASE("logicmill::bstream::memory::sink [ smoke ] { seek expanding buffer }")
{
	byte_type data[] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

			0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	};

	memory::sink                    snk{32};
	memory::detail::sink_test_probe probe{snk};

	std::error_code err;
	snk.filln(0xff, 16, err);
	CHECK(!err);
	CHECK(snk.size() == 16);
	CHECK(snk.position() == 16);

	snk.position(48, err);
	CHECK(!err);
	CHECK(snk.size() == 16);
	CHECK(snk.position() == 48);
	CHECK(probe.did_jump());
	CHECK(probe.jump_to() == 48);

	snk.filln(0x33, 16);
	CHECK(!err);
	CHECK(snk.size() == 64);
	CHECK(snk.position() == 64);

	CHECK(MATCH_BUFFER(probe.buffer(), data));
	std::cout << "capacity is " << probe.buffer().capacity() << std::endl;
}

TEST_CASE("logicmill::bstream::memory::source [ smoke ] { basic functionality }")
{
	byte_type data[] = {
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};

	const_buffer                 buf{data, sizeof(data)};
	memory::source<const_buffer> src{buf};

	bstream::memory::detail::source_test_probe<const_buffer> probe{src};

	std::error_code err;

	CHECK(probe.pos() == 0);
	CHECK(src.size() == sizeof(data));

	for (auto i = 0; i < 8; ++i)
	{
		byte_type b = src.get(err);
		CHECK(!err);
		CHECK(b == data[i]);
		CHECK(probe.pos() == i + 1);
	}

	const_buffer cbuf = src.get_slice(8, err);
	CHECK(!err);
	CHECK(probe.pos() == 16);
	CHECK(MATCH_BUFFER(cbuf, &data[8]));

	shared_buffer sbuf = src.get_shared_slice(8, err);
	CHECK(!err);
	CHECK(probe.pos() == 24);

	CHECK(MATCH_BUFFER(sbuf, &data[16]));

	byte_type bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	auto got = src.getn(bytes, sizeof(bytes), err);
	CHECK(!err);
	CHECK(got == 8);
	CHECK(probe.pos() == 32);

	CHECK(MATCH_MEMORY_N(bytes, &data[24], 8));

	byte_type not_there = src.get(err);

	CHECK(err);
	CHECK(err == bstream::errc::read_past_end_of_stream);
	CHECK(probe.pos() == 32);

	src.position(8, err);
	CHECK(!err);
	CHECK(probe.pos() == 8);

	cbuf = src.get_slice(8, err);
	CHECK(!err);
	CHECK(probe.pos() == 16);
	CHECK(src.position() == 16);
	CHECK(MATCH_BUFFER(cbuf, &data[8]));

	src.position(8, seek_anchor::current, err);
	CHECK(!err);
	CHECK(probe.pos() == 24);
	CHECK(src.position() == 24);

	sbuf = src.get_shared_slice(8, err);
	CHECK(!err);
	CHECK(probe.pos() == 32);
	CHECK(src.position() == 32);
	CHECK(MATCH_BUFFER(sbuf, &data[24]));

	src.position(1, seek_anchor::current, err);
	CHECK(err);
	CHECK(err == std::errc::invalid_seek);
	CHECK(probe.pos() == 32);
	CHECK(src.position() == 32);

	src.position(-8, seek_anchor::current, err);
	CHECK(!err);
	CHECK(probe.pos() == 24);
	CHECK(src.position() == 24);

	src.position(-1, seek_anchor::begin, err);
	CHECK(err);
	CHECK(err == std::errc::invalid_seek);
	CHECK(probe.pos() == 24);
	CHECK(src.position() == 24);

	src.position(1, seek_anchor::end, err);
	CHECK(err);
	CHECK(err == std::errc::invalid_seek);
	CHECK(probe.pos() == 24);
	CHECK(src.position() == 24);

	src.position(0, seek_anchor::end, err);
	CHECK(!err);
	CHECK(probe.pos() == 32);
	CHECK(src.position() == 32);

	not_there = src.get(err);
	CHECK(err);
	CHECK(err == bstream::errc::read_past_end_of_stream);
	CHECK(probe.pos() == 32);

	src.position(-1, seek_anchor::end, err);
	CHECK(!err);
	CHECK(probe.pos() == 31);
	CHECK(src.position() == 31);

	byte_type is_there = src.get(err);
	CHECK(!err);
	CHECK(is_there == 0x2f);
	CHECK(probe.pos() == 32);
	CHECK(src.position() == 32);
}