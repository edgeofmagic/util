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
#include <fstream>
#include <iostream>
#include <sstream>
#include <util/membuf.h>


TEST_CASE("util::membuf [ smoke ] { basic functionality }")
{
	std::char_traits<char> ct;

	util::omembuf mbuf{util::mutable_buffer{1024}};
	std::ostream  os{&mbuf};
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
	std::string  s0, s1, s2;
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

TEST_CASE("util::membuf [ smoke ] { sstream behavior check }")
{
	std::ostringstream ostrstrm;
	std::stringbuf     strbuf{std::ios_base::out};
	std::ostream       oss{&strbuf};
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

class mystrbuf : public std::stringbuf
{
public:
	mystrbuf(std::string const& s) : std::stringbuf{s} {}

protected:
	virtual int_type
	underflow() override
	{
		std::cout << "underflow called:" << std::endl;
		{
			void*          eb  = eback();
			void*          gp  = gptr();
			void*          eg  = egptr();
			std::ptrdiff_t pos = gptr() - eback();
			std::cout << "[" << eb << ", " << gp << ", " << eg << "] "
					  << "pos=" << pos << std::endl;
		}
		int_type result = std::stringbuf::underflow();
		std::cout << "underflow result is " << result << std::endl;
		{
			void*          eb  = eback();
			void*          gp  = gptr();
			void*          eg  = egptr();
			std::ptrdiff_t pos = gptr() - eback();
			std::cout << "[" << eb << ", " << gp << ", " << eg << "] "
					  << "pos=" << pos << std::endl;
		}
		return result;
	}

	virtual int_type
	overflow(int_type c = traits_type::eof()) override
	{
		std::cout << "overflow called with c==" << c << std::endl;
		{
			void*          pb  = pbase();
			void*          pp  = pptr();
			void*          ep  = epptr();
			std::ptrdiff_t pos = pptr() - pbase();
			std::cout << "[" << pb << ", " << pp << ", " << ep << "] "
					  << "pos=" << pos << std::endl;
		}
		int_type result = std::stringbuf::overflow(c);
		std::cout << "overflow result is " << result << std::endl;
		{
			void*          pb  = pbase();
			void*          pp  = pptr();
			void*          ep  = epptr();
			std::ptrdiff_t pos = pptr() - pbase();
			std::cout << "[" << pb << ", " << pp << ", " << ep << "] "
					  << "pos=" << pos << std::endl;
		}
		return result;
	}
};


TEST_CASE("util::membuf [ smoke ] { sstream behavior check 2 }")
{
	// std::stringbuf sbuf{"hello, there."};
	mystrbuf sbuf{"hello, there."};
	char     rbuf[13];
	auto     got = sbuf.sgetn(rbuf, 13);
	CHECK(got == 13);
	std::string gotstr{rbuf, 13};
	CHECK(gotstr == "hello, there.");
	auto ch = sbuf.sgetc();
	CHECK(ch == util::imemqbuf::traits_type::eof());
}

TEST_CASE("util::membuf [ smoke ] { duplicate sstream behavior }")
{
	util::omembuf strbuf{std::ios_base::out};
	std::ostream  oss{&strbuf};
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

TEST_CASE("util::membuf [ smoke ] { omemqbuf }")
{
	util::omemqbuf strbuf{16};
	std::ostream   oss{&strbuf};
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

TEST_CASE("util::membuf [ smoke ] { omemqbuf with mutable_buffer_alloc_factory }")
{
	util::omemqbuf strbuf{16};
	std::ostream   oss{&strbuf};
	oss << "abcdefghijklmnop";
	auto pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 16);
	CHECK(strbuf.get_buffer().size() == 1);
	CHECK(strbuf.get_buffer()[0].size() == 16);
	oss << "A";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 17);
	CHECK(strbuf.get_buffer().size() == 2);
	CHECK(strbuf.get_buffer()[1].size() == 1);
	oss << "BCDEFGHIJKLMNOP";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 32);
	CHECK(strbuf.get_buffer().size() == 2);
	CHECK(strbuf.get_buffer()[1].size() == 16);
	oss << "0123456789!@#$^&";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 48);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	oss.seekp(-8, std::ios_base::cur);
	CHECK(!oss.fail());
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 40);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	oss.seekp(16, std::ios_base::cur);
	CHECK(oss.fail());
	oss.clear();
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 40);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	oss.seekp(8, std::ios_base::beg);
	CHECK(!oss.fail());
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 8);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	oss << "IJKLMNOP";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 16);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	CHECK(strbuf.get_buffer()[0].to_string() == "abcdefghIJKLMNOP");
	oss << "hgfedcba";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 24);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	CHECK(strbuf.get_buffer()[1].to_string() == "hgfedcbaIJKLMNOP");
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf with mutable_buffer_fixed_factory }")
{
	util::omemqbuf strbuf{std::integral_constant<std::size_t, 16>{}};
	std::ostream   oss{&strbuf};
	oss << "abcdefghijklmnop";
	auto pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 16);
	CHECK(strbuf.get_buffer().size() == 1);
	CHECK(strbuf.get_buffer()[0].size() == 16);
	oss << "A";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 17);
	CHECK(strbuf.get_buffer().size() == 2);
	CHECK(strbuf.get_buffer()[1].size() == 1);
	oss << "BCDEFGHIJKLMNOP";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 32);
	CHECK(strbuf.get_buffer().size() == 2);
	CHECK(strbuf.get_buffer()[1].size() == 16);
	oss << "0123456789!@#$^&";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 48);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	oss.seekp(-8, std::ios_base::cur);
	CHECK(!oss.fail());
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 40);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	oss.seekp(16, std::ios_base::cur);
	CHECK(oss.fail());
	oss.clear();
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 40);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	oss.seekp(8, std::ios_base::beg);
	CHECK(!oss.fail());
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 8);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	oss << "IJKLMNOP";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 16);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	CHECK(strbuf.get_buffer()[0].to_string() == "abcdefghIJKLMNOP");
	oss << "hgfedcba";
	pos = oss.tellp();
	CHECK(static_cast<std::streamoff>(pos) == 24);
	CHECK(strbuf.get_buffer().size() == 3);
	CHECK(strbuf.get_buffer()[2].size() == 16);
	CHECK(strbuf.get_buffer()[1].to_string() == "hgfedcbaIJKLMNOP");
}

TEST_CASE("util::membuf [ smoke ] { imemqbuf basic }")
{
	std::deque<util::const_buffer> bufs;
	bufs.emplace_back(util::const_buffer{util::mutable_buffer{"hello "}});           // 6
	bufs.emplace_back(util::const_buffer{util::mutable_buffer{"there you "}});       // 10
	bufs.emplace_back(util::const_buffer{util::mutable_buffer{"silly, silly "}});    // 13
	bufs.emplace_back(util::const_buffer{util::mutable_buffer{"little "}});          // 7
	bufs.emplace_back(util::const_buffer{util::mutable_buffer{"person, you."}});     // 12 = 48
	util::imemqbuf imbuf{std::move(bufs)};
	CHECK(imbuf.size() == 48);
	char rbuf[48];
	auto n = imbuf.sgetn(rbuf, 48);
	CHECK(n == 48);
	std::string got{rbuf, 48};
	CHECK(got == "hello there you silly, silly little person, you.");
	auto ch = imbuf.sgetc();
	CHECK(ch == util::imemqbuf::traits_type::eof());


	{
		auto pos = imbuf.pubseekpos(0);
		CHECK(pos == 0);
		auto ch = imbuf.sbumpc();
		CHECK(ch == 'h');
		pos = imbuf.pubseekoff(0, std::ios_base::cur);
		CHECK(pos == 1);
	}

	{
		auto pos = imbuf.pubseekpos(47);
		CHECK(pos == 47);
		auto ch = imbuf.sbumpc();
		CHECK(ch == '.');
		pos = imbuf.pubseekoff(0, std::ios_base::cur);
		CHECK(pos == 48);
	}

	{
		auto pos = imbuf.pubseekpos(5);
		CHECK(pos == 5);
		auto ch = imbuf.sbumpc();
		CHECK(ch == ' ');
		pos = imbuf.pubseekoff(0, std::ios_base::cur);
		CHECK(pos == 6);
	}

	{
		auto pos = imbuf.pubseekpos(36);
		CHECK(pos == 36);
		auto ch = imbuf.sbumpc();
		CHECK(ch == 'p');
		pos = imbuf.pubseekoff(0, std::ios_base::cur);
		CHECK(pos == 37);
	}

	{
		auto pos = imbuf.pubseekpos(6);
		CHECK(pos == 6);
		auto ch = imbuf.sbumpc();
		CHECK(ch == 't');
		pos = imbuf.pubseekoff(0, std::ios_base::cur);
		CHECK(pos == 7);
	}

	{
		auto pos = imbuf.pubseekpos(35);
		CHECK(pos == 35);
		auto ch = imbuf.sbumpc();
		CHECK(ch == ' ');
		pos = imbuf.pubseekoff(0, std::ios_base::cur);
		CHECK(pos == 36);
	}

	{
		auto pch = imbuf.sungetc();
		CHECK(pch == ' ');
		auto pos = imbuf.pubseekoff(0, std::ios_base::cur);
		CHECK(pos == 35);
		auto ch = imbuf.sbumpc();
		CHECK(ch == ' ');
		pos = imbuf.pubseekoff(0, std::ios_base::cur);
		CHECK(pos == 36);
	}
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf null ctor }")
{
	util::omembuf omb{};

	auto pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == util::omembuf::traits_type::eof());
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == util::omembuf::traits_type::eof());

	CHECK(omb.get_buffer().capacity() == 0);

	omb.sputc('A');

	pos = omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == 1);

	pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == 1);

	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	CHECK(omb.get_buffer().capacity() > 0);
	CHECK(omb.get_buffer().size() == 1);
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf mutable_buffer ctor with non-zero size }")
{
	std::string          content{"abcdefghijklmnopqrstuvwxyz"};
	util::mutable_buffer buf{content};
	buf.size(content.size());
	util::omembuf omb{std::move(buf)};
	auto          pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == content.size());
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);
	std::string buf_content{reinterpret_cast<char*>(const_cast<util::byte_type*>(omb.get_buffer().data())),
							omb.get_buffer().size()};
	CHECK(buf_content == content);
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf mutable_buffer ctor with zero size }")
{
	std::string content{"abcdefghijklmnopqrstuvwxyz"};

	util::mutable_buffer buf{32};
	util::omembuf        omb{std::move(buf)};
	auto                 pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == 0);
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	auto n = omb.sputn(content.data(), content.size());
	CHECK(n == content.size());

	pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == content.size());
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	std::string buf_content{reinterpret_cast<char*>(const_cast<util::byte_type*>(omb.get_buffer().data())),
							omb.get_buffer().size()};
	CHECK(buf_content == content);
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf move ctor }")
{
	std::string content{"abcdefghijklmnopqrstuvwxyz"};

	util::mutable_buffer buf{32};
	util::omembuf        omb{std::move(buf)};
	auto                 pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == 0);
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	auto n = omb.sputn(content.data(), content.size());
	CHECK(n == content.size());

	pos = omb.pubseekpos(13);
	CHECK(pos == 13);

	pos = omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == 13);

	util::omembuf moved_omb{std::move(omb)};

	pos = moved_omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == 13);

	pos = moved_omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == content.size());

	pos = moved_omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	std::string buf_content{reinterpret_cast<char*>(const_cast<util::byte_type*>(moved_omb.get_buffer().data())),
							moved_omb.get_buffer().size()};
	CHECK(buf_content == content);

	pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == util::omembuf::traits_type::eof());
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == util::omembuf::traits_type::eof());

	CHECK(omb.get_buffer().capacity() == 0);
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf move assignment }")
{
	std::string content{"abcdefghijklmnopqrstuvwxyz"};

	util::mutable_buffer buf{32};
	util::omembuf        omb{std::move(buf)};
	auto                 pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == 0);
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	auto n = omb.sputn(content.data(), content.size());
	CHECK(n == content.size());

	pos = omb.pubseekpos(13);
	CHECK(pos == 13);

	pos = omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == 13);

	util::omembuf moved_omb{};

	moved_omb = std::move(omb);

	pos = moved_omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == 13);

	pos = moved_omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == content.size());

	pos = moved_omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	std::string buf_content{reinterpret_cast<char*>(const_cast<util::byte_type*>(moved_omb.get_buffer().data())),
							moved_omb.get_buffer().size()};
	CHECK(buf_content == content);

	pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == util::omembuf::traits_type::eof());
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == util::omembuf::traits_type::eof());

	CHECK(omb.get_buffer().capacity() == 0);
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf buffer assignment }")
{
	std::string content{"abcdefghijklmnopqrstuvwxyz"};

	util::mutable_buffer buf{content};
	buf.size(content.size());

	util::omembuf omb;
	omb = std::move(buf);

	auto pos = omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == 0);

	pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == content.size());

	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	auto avail = omb.in_avail();
	std::cout << "in_avail is " << avail << std::endl;

	auto gotc = omb.sgetc();
	std::cout << "sgetc is " << gotc << std::endl;

	std::string buf_content{reinterpret_cast<char*>(const_cast<util::byte_type*>(omb.get_buffer().data())),
							omb.get_buffer().size()};
	CHECK(buf_content == content);

	CHECK(buf.data() == nullptr);
	CHECK(buf.size() == 0);
	CHECK(buf.capacity() == 0);
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf empty buffer assignment }")
{
	util::mutable_buffer buf;
	util::omembuf        omb;
	omb = std::move(buf);

	auto pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == util::omembuf::traits_type::eof());
	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == util::omembuf::traits_type::eof());
	pos = omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == util::omembuf::traits_type::eof());

	CHECK(omb.get_buffer().data() == nullptr);
	CHECK(omb.get_buffer().size() == 0);
	CHECK(omb.get_buffer().capacity() == 0);
}

TEST_CASE("util::membuf [ smoke ] { omemqbuf setbuf }")
{
	char        space[32];
	std::string content{"abcdefghijklmnopqrstuvwxyz"};

	util::omembuf omb;
	omb.pubsetbuf(space, sizeof(space));

	auto pos = omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == 0);

	pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == 0);

	CHECK(omb.get_buffer().capacity() == 32);

	auto n = omb.sputn(content.data(), content.size());

	CHECK(n == content.size());

	pos = omb.pubseekoff(0, std::ios_base::cur);
	CHECK(pos == content.size());

	pos = omb.pubseekoff(0, std::ios_base::beg);
	CHECK(pos == 0);

	pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == content.size());

	std::string buf_content{reinterpret_cast<char*>(const_cast<util::byte_type*>(omb.get_buffer().data())),
							omb.get_buffer().size()};
	CHECK(buf_content == content);

	std::string expected{"abcdefghijklmnopqrstuvwxyzabcdef"};

	n = omb.sputn(content.data(), content.size());

	CHECK(n == sizeof(space) - content.size());

	std::string full_buf_content{reinterpret_cast<char*>(const_cast<util::byte_type*>(omb.get_buffer().data())),
								 omb.get_buffer().size()};
	CHECK(full_buf_content == expected);

	std::string from_space{space, sizeof(space)};
	CHECK(from_space == expected);

	pos = omb.pubseekoff(0, std::ios_base::end);
	CHECK(pos == sizeof(space));
}
