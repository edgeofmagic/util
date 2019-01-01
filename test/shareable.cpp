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

#include <logicmill/util/shareable.h>
#include <doctest.h>
#include <iostream>

using namespace logicmill;
using namespace util;

class sstring : public shareable<sstring>
{
public:
	sstring(std::string const& s) : m_str{s} {}
	std::string const&
	to_string() const
	{
		return m_str;
	}

private:
	std::string m_str;
};

class sstr : handle<sstring>
{
protected:
	using base = handle<sstring>;

	sstr(sstring* sp) : base{sp}, m_view{base::get_ptr()->to_string()} {}

public:

	sstr(sstr const& rhs) : base{rhs}, m_view{rhs.m_view} {}

	sstr(sstr&& rhs) : base{std::move(rhs)}, m_view{rhs.m_view} {}

	template<class... Args>
	static sstr create(Args&&... args)
	{
		return sstr(new sstring(std::forward<Args>(args)...));
	}

	std::string_view const&
	view() const
	{
		return m_view;
	}

	sstr& operator=(sstr const& rhs)
	{
		base::assign(rhs);
		return *this;
	}

	sstr& operator=(sstr&& rhs)
	{
		base::assign(std::move(rhs));
		return *this;
	}

	std::size_t use_count() const
	{
		return base::get_refcount();
	}

private:
	std::string_view m_view;
};

class sint : public shareable<int>
{
public:
	using base = shareable<int>;

	sint(int val = 0) : m_value{val} {}

	int get() const
	{
		return m_value;
	}

	operator int() const
	{
		return m_value;
	}

	void set(int val)
	{
		m_value = val;
	}

private:
	int m_value;
 };

 using sint_deleter = std::function<void(sint*)>;

TEST_CASE( "logicmill::util::shareable [ smoke ]" )
{
	auto sh = make_shareable<sstring>("hello");
	CHECK(sh->to_string() == "hello");
	auto sh_copy = sh;
	CHECK(sh.use_count() == 2);
	CHECK(sh.get() == sh_copy.get());
	CHECK(sh == sh_copy);
}

TEST_CASE( "logicmill::util::shareable [ smoke ] { non-pointer handle }" )
{
	auto ss = sstr::create("hello");
	CHECK(ss.view() == "hello");
	auto ss_copy = ss;
	CHECK(ss.use_count() == 2);
	CHECK(ss.view() == ss_copy.view());
}

TEST_CASE( "logicmill::util::shareable [ smoke ] { non-pointer handle }" )
{
	auto ss = sstr::create("hello");
	CHECK(ss.view() == "hello");
	auto ss_copy = ss;
	CHECK(ss.use_count() == 2);
	CHECK(ss.view() == ss_copy.view());
}

TEST_CASE( "logicmill::util::shareable [ smoke ] { phandle with deleter }" )
{
	bool deleter_called{false};
	using sintp = phandle<sint, sint_deleter>;
	sintp p = sintp{new sint{7}, [&](sint* p) { delete p; deleter_called = true; }};
	CHECK(p->get() == 7);
	CHECK(p);
	p.reset();
	CHECK(!p);
	CHECK(deleter_called);
}

TEST_CASE( "logicmill::util::shareable [ smoke ] { phandle with null deleter }" )
{
	using sintp = phandle<sint, do_not_delete<sint>>;
	sint auto_sint{27};
	sintp p = sintp{&auto_sint};
	CHECK(p->get() == 27);
	p.reset();
	CHECK(!p);
}

