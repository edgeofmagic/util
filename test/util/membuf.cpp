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
#include <logicmill/util/membuf.h>
#include <sstream>

using namespace logicmill;

TEST_CASE("logicmill::util::membuf [ smoke ] { basic functionality }")
{
	util::omembuf mbuf{util::mutable_buffer{1024}};
	std::ostream os{&mbuf};
	os << "some buffer content";
	std::cout << mbuf.get_buffer().to_string() << std::endl;
	CHECK(mbuf.get_buffer().to_string() == "some buffer content");
	util::imembuf ibuf{util::const_buffer{mbuf.release_buffer()}};
	CHECK(ibuf.size() == 19);
	auto endpos = ibuf.pubseekoff(0, std::ios_base::end);
	ibuf.rewind();
	auto endoff = static_cast<std::streamoff>(endpos);
	CHECK(endoff == 19);
	std::istream is{&ibuf};
	std::string s0, s1, s2;
	is >> s0;
	CHECK(ibuf.position() == 4);
	CHECK(s0 == "some");
	is >> s1;
	CHECK(ibuf.position() == 11);
	CHECK(s1 == "buffer");
	is >> s2;
	CHECK(ibuf.position() == 19);
	CHECK(s2 == "content");
	
}

TEST_CASE("logicmill::util::membuf [ smoke ] { sstream behavior check }")
{
    std::stringbuf strbuf{std::ios_base::out};
    std::ostream oss{&strbuf};
	oss << "abcdefg";
	auto pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 7);
	oss.seekp(pos - static_cast<std::streamoff>(2));
	auto tell_after_seek = oss.tellp();
	std::cout << static_cast<std::streamoff>(tell_after_seek) << std::endl;



	// oss.seekp(pos + std::streamoff{8});
	auto result = strbuf.pubseekoff(8, std::ios_base::cur, std::ios_base::out);
	std::cout << static_cast<std::streamoff>(result) << std::endl;

	tell_after_seek = oss.tellp();
	std::cout << static_cast<std::streamoff>(tell_after_seek) << std::endl;

	// auto here = strbuf.pubseekoff(0, std::ios_base::cur);
	// std::cout << static_cast<std::streamoff>(here) << std::endl;

	auto there = strbuf.pubseekoff(0, std::ios_base::end, std::ios_base::out);
	std::cout << static_cast<std::streamoff>(there) << std::endl;

}

TEST_CASE("logicmill::util::membuf [ smoke ] { duplicate sstream behavior }")
{
    util::omembuf strbuf{std::ios_base::out};
    std::ostream oss{&strbuf};
	oss << "abcdefg";
	auto pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 7);
	oss.seekp(pos - static_cast<std::streamoff>(2));
	auto tell_after_seek = oss.tellp();
	std::cout << static_cast<std::streamoff>(tell_after_seek) << std::endl;



	// oss.seekp(pos + std::streamoff{8});
	auto result = strbuf.pubseekoff(8, std::ios_base::cur, std::ios_base::out);
	std::cout << static_cast<std::streamoff>(result) << std::endl;

	tell_after_seek = oss.tellp();
	std::cout << static_cast<std::streamoff>(tell_after_seek) << std::endl;

	// auto here = strbuf.pubseekoff(0, std::ios_base::cur);
	// std::cout << static_cast<std::streamoff>(here) << std::endl;

	auto there = strbuf.pubseekoff(0, std::ios_base::end, std::ios_base::out);
	std::cout << static_cast<std::streamoff>(there) << std::endl;

}
