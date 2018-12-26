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

// #include <logicmill/bstream/compound_memory/sink.h>
// #include <logicmill/bstream/compound_memory/source.h>
#include "test_probes/compound_memory.h"
#include "common.h"
#include <doctest.h>
#include <logicmill/bstream/buffer.h>
#include <logicmill/bstream/error.h>

using namespace logicmill;
using namespace bstream;

TEST_CASE("logicmill::bstream::compound_memory::sink [ smoke ] { expanding buffer }")
{
	byte_type data[] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,

			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,

			0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,

			0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	};

	compound_memory::sink snk{32};
	// memory::detail::sink_test_probe probe{ snk };

	std::error_code err;
	snk.filln(0xff, 16, err);
	CHECK(!err);
	CHECK(snk.size() == 16);
	CHECK(snk.position() == 16);
	CHECK(snk.get_buffers().size() == 1);
	CHECK(snk.get_buffers()[0].size() == 16);

	CHECK(MATCH_BUFFER(snk.get_buffers()[0], &data[0]));

	snk.filln(0x80, 32, err);
	CHECK(!err);
	CHECK(snk.size() == 48);
	CHECK(snk.position() == 48);
	CHECK(snk.get_buffers().size() == 2);
	CHECK(snk.get_buffers()[0].size() == 32);
	CHECK(snk.get_buffers()[1].size() == 16);

	CHECK(MATCH_BUFFER(snk.get_buffers()[0], &data[0]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[1], &data[32]));

	snk.filln(0x33, 16, err);
	CHECK(!err);
	CHECK(snk.size() == 64);
	CHECK(snk.position() == 64);
	CHECK(snk.get_buffers().size() == 2);
	CHECK(snk.get_buffers()[0].size() == 32);
	CHECK(snk.get_buffers()[1].size() == 32);

	CHECK(MATCH_BUFFER(snk.get_buffers()[0], &data[0]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[1], &data[32]));

	snk.put(0x33);
	CHECK(!err);
	CHECK(snk.size() == 65);
	CHECK(snk.position() == 65);
	CHECK(snk.get_buffers().size() == 3);
	CHECK(snk.get_buffers()[0].size() == 32);
	CHECK(snk.get_buffers()[1].size() == 32);
	CHECK(snk.get_buffers()[2].size() == 1);
	CHECK(MATCH_BUFFER(snk.get_buffers()[0], &data[0]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[1], &data[32]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[2], &data[64]));

	snk.filln(0x33, 15, err);
	CHECK(!err);
	CHECK(snk.size() == 80);
	CHECK(snk.position() == 80);
	CHECK(snk.get_buffers().size() == 3);
	CHECK(snk.get_buffers()[0].size() == 32);
	CHECK(snk.get_buffers()[1].size() == 32);
	CHECK(snk.get_buffers()[2].size() == 16);
	CHECK(MATCH_BUFFER(snk.get_buffers()[0], &data[0]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[1], &data[32]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[2], &data[64]));

	// CHECK( MATCH_BUFFER( probe.buffer(), data ) );
	// std::cout << "capacity is " << probe.buffer().capacity() << std::endl;
}

TEST_CASE("logicmill::bstream::compound_memory::sink [ smoke ] { expanding buffer with seek }")
{
	byte_type data[] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,

			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,

			0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,

			0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	};

	byte_type expected_0[] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xDE, 0xAD, 0xBE, 0xEF,

			0xCA, 0xFE, 0xBA, 0xBE, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,

			0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,

			0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	};

	compound_memory::sink snk{32};
	// memory::detail::sink_test_probe probe{ snk };

	std::error_code err;

	snk.putn(&data[0], 80, err);
	CHECK(!err);
	CHECK(snk.size() == 80);
	CHECK(snk.position() == 80);
	CHECK(snk.get_buffers().size() == 3);
	CHECK(snk.get_buffers()[0].size() == 32);
	CHECK(snk.get_buffers()[1].size() == 32);
	CHECK(snk.get_buffers()[2].size() == 16);
	CHECK(MATCH_BUFFER(snk.get_buffers()[0], &data[0]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[1], &data[32]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[2], &data[64]));

	snk.position(28);
	std::uint64_t n{0xDEADBEEFCAFEBABEULL};
	snk.put_num(n, true, err);
	CHECK(!err);
	CHECK(snk.size() == 80);
	CHECK(snk.position() == 36);
	CHECK(snk.get_buffers().size() == 3);
	CHECK(snk.get_buffers()[0].size() == 32);
	CHECK(snk.get_buffers()[1].size() == 32);
	CHECK(snk.get_buffers()[2].size() == 16);
	CHECK(MATCH_BUFFER(snk.get_buffers()[0], &expected_0[0]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[1], &expected_0[32]));
	CHECK(MATCH_BUFFER(snk.get_buffers()[2], &expected_0[64]));


	// CHECK( MATCH_BUFFER( probe.buffer(), data ) );
	// std::cout << "capacity is " << probe.buffer().capacity() << std::endl;
}

TEST_CASE("logicmill::bstream::compound_memory::source [ smoke ] { basic }")
{
	byte_type data_0[] = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	byte_type data_1[] = {
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
			0x01,
	};

	byte_type data_2[] = {
			0x02,
			0x02,
			0x02,
			0x02,
			0x02,
			0x02,
			0x02,
			0x02,
	};

	byte_type data_3[] = {
			0x03,
			0x03,
			0x03,
			0x03,
			// 0x04,
			// 0x04,
			// 0x05,
			// 0x06
	};

	byte_type data_4[] = {
			0x04,
			0x04,
			// 0x05,
			// 0x06
	};

	byte_type data_5[] = {
			0x05, // 0x06,
	};

	byte_type data_6[] = {
			0x06,
	};

	byte_type expected[] = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06,
	};

	std::deque<bstream::const_buffer> bufs;
	bufs.emplace_back(const_buffer{&data_0[0], sizeof(data_0)});
	bufs.emplace_back(const_buffer{&data_1[0], sizeof(data_1)});
	bufs.emplace_back(const_buffer{&data_2[0], sizeof(data_2)});
	bufs.emplace_back(const_buffer{&data_3[0], sizeof(data_3)});
	bufs.emplace_back(const_buffer{&data_4[0], sizeof(data_4)});
	bufs.emplace_back(const_buffer{&data_5[0], sizeof(data_5)});
	bufs.emplace_back(const_buffer{&data_6[0], sizeof(data_6)});

	compound_memory::source<bstream::const_buffer> src{bufs};

	std::error_code err;
	CHECK(src.size() == 64);

	const_buffer got = src.get_slice(64, err);
	CHECK(!err);

	CHECK(MATCH_BUFFER(got, &expected[0]));
}