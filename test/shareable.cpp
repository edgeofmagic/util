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
#include <logicmill/util/allocator.h>
#include <doctest.h>
#include <iostream>

using namespace logicmill;
using namespace util;

namespace shareable_test
{

static bool free_mem_called{false};

void* get_mem(unsigned long bytes)
{
	std::cout << "get_mem: " << bytes << std::endl;
	return ::malloc(bytes);
}

void free_mem(void* p, unsigned long bytes)
{
	std::cout << "free_mem: " << bytes << std::endl;
	free_mem_called = true;
	::free(p);
}

UTIL_ALLOCATOR_POLICY(malloc_policy, get_mem, free_mem);

UTIL_ALLOCATOR_ALIAS(string_alloc, malloc_policy, std::string);


class sstr : handle<std::string>
{
protected:
	using base = handle<std::string>;

public:

	template<class... Args>
	sstr(Args... args) : base{std::forward<Args>(args)...}, m_view{*base::get_ptr()} {}

	sstr(sstr const& rhs) : base{ static_cast<base const&>(rhs)}, m_view{rhs.m_view} {}

	sstr(sstr&& rhs) : base{std::move(static_cast<base&&>(rhs))}, m_view{rhs.m_view} {}

	template<class... Args>
	static sstr create(Args&&... args)
	{
		return sstr{std::forward<Args>(args)...};
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

class foo 
{
public:

	foo(int i, std::string const& s) : m_ival{i}, m_sval{s} {}

	foo(foo const& f) : m_ival{f.m_ival}, m_sval{f.m_sval} {}

	foo(foo&& f) : m_ival{f.m_ival}, m_sval{std::move(f.m_sval)} {}

	int num() const
	{
		return m_ival;
	}

	std::string const& str() const
	{
		return m_sval;
	}

private:
	int m_ival;
	std::string m_sval;
 };

class bar : public foo
{
public:

	bar(int i, std::string const& s, double d) : foo{i, s}, m_fpnum{d} {}

	double dnum() const
	{
		return m_fpnum;
	}

private:
	double m_fpnum;
};


class foov
{
public:

	virtual ~foov() {}

	foov(int i, std::string const& s) : m_ival{i}, m_sval{s} {}

	foov(foov const& f) : m_ival{f.m_ival}, m_sval{f.m_sval} {}

	foov(foov&& f) : m_ival{f.m_ival}, m_sval{std::move(f.m_sval)} {}

	int num() const
	{
		return m_ival;
	}

	std::string const& str() const
	{
		return m_sval;
	}

private:
	int m_ival;
	std::string m_sval;
 };

class barv : public foov
{
public:

	barv(int i, std::string const& s, double d) : foov{i, s}, m_fpnum{d} {}

	double dnum() const
	{
		return m_fpnum;
	}

private:
	double m_fpnum;
};

struct int_malloc
{
	int* operator()() const
	{
		return static_cast<int*>(::malloc(sizeof(int)));
	}
};

static int int_free_call_count{0};

struct int_free
{
	void operator()(int* p) const
	{
		++int_free_call_count;
		::free(p);
	}
};

}    // namespace shareable_test

TEST_CASE( "logicmill::util::shareable [ smoke ]" )
{
	auto p = phandle<std::string>::create("hello");
	CHECK(*p == "hello");
	auto p_copy = p;
	CHECK(p.use_count() == 2);
	CHECK(p.get() == p_copy.get());
	CHECK(p == p_copy);
}

TEST_CASE( "logicmill::util::shareable [ smoke ] { non-pointer handle }" )
{
	auto ss = shareable_test::sstr::create("hello");
	CHECK(ss.view() == "hello");
	auto ss_copy = ss;
	CHECK(ss.use_count() == 2);
	CHECK(ss.view() == ss_copy.view());
}

TEST_CASE( "logicmill::util::shareable [ smoke ] { phandle with deleter }" )
{
	using sintp = phandle<std::string, shareable_test::string_alloc>;
	sintp p = sintp::create("zoot");
	CHECK(*p == "zoot");
	CHECK(bool(p));
	p.reset();
	CHECK(!p);
	CHECK(shareable_test::free_mem_called);
}

TEST_CASE( "logicmill::util::shareable [ smoke ] { phandle with class }" )
{
	using foop = phandle<shareable_test::foo>;
	foop p = foop::create(27, "zoot");
	CHECK(p->str() == "zoot");
	CHECK(p->num() == 27);
	CHECK(bool(p));
	foop p_moved(std::move(p));
	CHECK(p_moved.use_count() == 1);
	CHECK(!p);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr copy ctor }")
{
	util::shared_ptr<std::string> sp = util::shared_ptr<std::string>::create("zoot");
	util::shared_ptr<std::string> sp_copy{sp};
	CHECK(sp.use_count() == 2);
	CHECK(sp_copy == sp);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
	CHECK(sp.use_count() == 1);
}

#if 1
TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr move ctor }")
{
	util::shared_ptr<std::string> sp = util::shared_ptr<std::string>::create("zoot");
	util::shared_ptr<std::string> sp_copy{std::move(sp)};
	CHECK(!sp);
	CHECK(sp_copy.use_count() == 1);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
}
#endif

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr copy assign }")
{
	util::shared_ptr<std::string> sp = util::shared_ptr<std::string>::create("zoot");
	util::shared_ptr<std::string> sp_copy;
	sp_copy = sp;
	CHECK(sp.use_count() == 2);
	CHECK(sp_copy == sp);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
	CHECK(sp.use_count() == 1);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr move assign }")
{
	util::shared_ptr<std::string> sp = util::shared_ptr<std::string>::create("zoot");
	util::shared_ptr<std::string> sp_copy;
	sp_copy = std::move(sp);
	CHECK(!sp);
	CHECK(sp_copy.use_count() == 1);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr copy construct poly }")
{
	util::shared_ptr<shareable_test::bar> bp = util::shared_ptr<shareable_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shareable_test::foo> fp{bp};
	CHECK(fp.use_count() == 2);
	CHECK(fp == bp);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
	CHECK(bp.use_count() == 1);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr move construct poly }")
{
	util::shared_ptr<shareable_test::bar> bp = util::shared_ptr<shareable_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shareable_test::foo> fp{std::move(bp)};
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr copy assign poly }")
{
	util::shared_ptr<shareable_test::bar> bp = util::shared_ptr<shareable_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shareable_test::foo> fp;
	fp = bp;
	CHECK(fp.use_count() == 2);
	CHECK(fp == bp);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
	CHECK(bp.use_count() == 1);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr move assign poly }")
{
	util::shared_ptr<shareable_test::bar> bp = util::shared_ptr<shareable_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shareable_test::foo> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr static_ptr_cast }")
{
	util::shared_ptr<shareable_test::bar> bp = util::shared_ptr<shareable_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shareable_test::foo> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	util::shared_ptr<shareable_test::bar> barp = util::shared_ptr<shareable_test::bar>::static_ptr_cast(fp);
	CHECK(fp.use_count() == 2);
	CHECK(barp->dnum() == 3.5);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr dynamic_ptr_cast }")
{
	util::shared_ptr<shareable_test::barv> bp = util::shared_ptr<shareable_test::barv>::create(7, "groot", 3.5);
	util::shared_ptr<shareable_test::foov> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	util::shared_ptr<shareable_test::barv> barvp = util::shared_ptr<shareable_test::barv>::dynamic_ptr_cast(fp);
	CHECK(fp.use_count() == 2);
	CHECK(barvp->dnum() == 3.5);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr with deleter }")
{
	int* ip = shareable_test::int_malloc{}();
	*ip = 27;
	util::shared_ptr<int, shareable_test::int_free> p = util::shared_ptr<int, shareable_test::int_free>{ip, shareable_test::int_free{}};
	util::shared_ptr<int, shareable_test::int_free> p_copy{p};
	CHECK(p.use_count() == 2);
	CHECK(p_copy == p);
	CHECK(*p_copy == 27);
	p_copy.reset();
	CHECK(shareable_test::int_free_call_count == 0);
	CHECK(!p_copy);
	CHECK(p.use_count() == 1);
	p.reset();
	CHECK(shareable_test::int_free_call_count == 1);
}

TEST_CASE("logicmill::util::shareable [ smoke ] { util::shared_ptr with allocate }")
{
	using zallocator = util::allocator<std::string, shareable_test::malloc_policy<std::string>>;
	using shared_ptr_type = util::shared_ptr<std::string>;
	using control_blk_type = shared_ptr_type::control_blk;
	using shareable_value_type = shared_ptr_type::shareable_value<zallocator>;
	util::shared_ptr<std::string> sp = util::shared_ptr<std::string>::allocate(zallocator{}, "zoot");
	util::shared_ptr<std::string> sp_copy{sp};
	CHECK(sp.use_count() == 2);
	CHECK(sp_copy == sp);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
	CHECK(sp.use_count() == 1);

	// std::cout << "sizeof string is " << sizeof(std::string) << std::endl;
	// std::cout << "sizeof shared ptr is " << sizeof(shared_ptr_type) << std::endl;
	// std::cout << "sizeof control blk base is " << sizeof(util::detail::control_blk_base) << std::endl;
	// std::cout << "sizeof control blk is " << sizeof(control_blk_type) << std::endl;
	// std::cout << "sizeof shareable value is " << sizeof(shareable_value_type) << std::endl;
	// std::cout << "sizeof zallocator is " << sizeof(zallocator) << std::endl;
	// std::cout << "sizeof deleter is " << sizeof(std::default_delete<std::string>) << std::endl;

}