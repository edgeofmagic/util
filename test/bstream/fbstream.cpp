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
#include <logicmill/bstream/ifbstream.h>
#include <logicmill/bstream/ofbstream.h>

using namespace logicmill;
using namespace bstream;
using bstream::ofbstream;

#define STRING_INIT_MUTABLE_BUFFER(_name_, _contents_)                                                                 \
	util::mutable_buffer _name_{::strlen(_contents_)};                                                                 \
	{                                                                                                                  \
		std::string str_{_contents_};                                                                                  \
		_name_.putn(0, str_.data(), str_.size());                                                                      \
		_name_.size(str_.size());                                                                                      \
	}                                                                                                                  \
	/**/

TEST_CASE("logicmill/smoke/bstream/fbstream/write_read")
{
	bstream::ofbstream os("fbstream_test_file", bstream::open_mode::truncate);
	// util::mutable_buffer outbuf{ "abcdefghijklmnop" };
	STRING_INIT_MUTABLE_BUFFER(outbuf, "abcdefghijklmnop");
	os.putn(outbuf);
	os.close();

	bstream::ifbstream is("fbstream_test_file");
	auto               fsize = is.size();
	util::const_buffer inbuf = is.get_slice(fsize);
	is.close();
	CHECK(outbuf == inbuf);
}

TEST_CASE("logicmill/smoke/bstream/fbstream/write_read_ate")
{
	{
		bstream::ofbstream os("fbstream_test_file", bstream::open_mode::truncate);
		STRING_INIT_MUTABLE_BUFFER(outbuf, "abcdefghijklmnop");
		// util::mutable_buffer outbuf{ "abcdefghijklmnop" };
		os.putn(outbuf);
		os.close();
	}
	{
		bstream::ifbstream is("fbstream_test_file");
		auto               fsize = is.size();
		util::const_buffer inbuf = is.get_slice(fsize);
		is.close();
		// util::mutable_buffer expected{ "abcdefghijklmnop" };
		STRING_INIT_MUTABLE_BUFFER(expected, "abcdefghijklmnop");
		CHECK(expected == inbuf);
	}
	{
		bstream::ofbstream os("fbstream_test_file", bstream::open_mode::at_end);
		auto               pos = os.position();
		CHECK(pos == 16);
		auto fsize = os.size();
		CHECK(fsize == 16);
		// util::mutable_buffer outbuf{ "qrstuvwxyz" };
		STRING_INIT_MUTABLE_BUFFER(outbuf, "qrstuvwxyz");
		os.putn(outbuf);
		os.close();
	}
	{
		bstream::ifbstream is("fbstream_test_file");
		auto               fsize = is.size();
		CHECK(fsize == 26);
		util::const_buffer inbuf = is.get_slice(fsize);
		is.close();
		// util::mutable_buffer expected{ "abcdefghijklmnopqrstuvwxyz" };
		STRING_INIT_MUTABLE_BUFFER(expected, "abcdefghijklmnopqrstuvwxyz");
		CHECK(expected == inbuf);
	}
	{
		bstream::ifbstream is("fbstream_test_file");
		auto               fsize = is.size();
		CHECK(fsize == 26);
		is.position(16, seek_anchor::begin);
		util::const_buffer inbuf            = is.get_slice(10);
		bool               caught_exception = false;
		try
		{
			is.get();
			CHECK(false);    // should never get here
		}
		catch (std::system_error const& e)
		{
			caught_exception = true;
		}
		CHECK(caught_exception);
		is.close();
		// util::mutable_buffer expected{ "qrstuvwxyz" };
		STRING_INIT_MUTABLE_BUFFER(expected, "qrstuvwxyz");
		CHECK(expected == inbuf);
	}
	{
		bstream::ofbstream os("fbstream_test_file", bstream::open_mode::append);
		auto               pos = os.position();
		CHECK(pos == 26);
		// util::mutable_buffer outbuf{ "0123456789" };
		STRING_INIT_MUTABLE_BUFFER(outbuf, "0123456789");
		os.putn(outbuf);
		auto zpos = os.position(0);
		os.putn(outbuf);
		zpos = os.position();
		os.close();
	}
	{
		bstream::ofbstream os("fbstream_test_file");
		auto               pos = os.position();
		CHECK(pos == 0);
		auto fsize = os.size();
		CHECK(fsize == 46);
		// util::mutable_buffer outbuf{ "0123456789" };
		STRING_INIT_MUTABLE_BUFFER(outbuf, "0123456789");
		os.putn(outbuf);
		auto zpos = os.position();
		CHECK(zpos == 10);
		os.close();
	}
	{
		bstream::ifbstream is("fbstream_test_file");
		auto               fsize = is.size();
		CHECK(fsize == 46);
		util::const_buffer inbuf = is.get_slice(fsize);
		is.close();
		// util::mutable_buffer expected{ "0123456789klmnopqrstuvwxyz01234567890123456789" };
		STRING_INIT_MUTABLE_BUFFER(expected, "0123456789klmnopqrstuvwxyz01234567890123456789");
		CHECK(expected == inbuf);
	}
	{
		bstream::ofbstream os("fbstream_test_file", bstream::open_mode::truncate);
		// util::mutable_buffer outbuf{ "abcdefghijklmnop" };
		STRING_INIT_MUTABLE_BUFFER(outbuf, "abcdefghijklmnop");
		os.putn(outbuf);
		os.position(36);
		// util::mutable_buffer outbuf2{ "0123456789" };
		STRING_INIT_MUTABLE_BUFFER(outbuf2, "0123456789");
		os.putn(outbuf2);
		auto zpos = os.position();
		CHECK(zpos == 46);
		os.close();
	}
}
