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

#include <logicmill/util/shared_ptr.h>
#include <logicmill/util/allocator.h>
#include <doctest.h>
#include <iostream>

using namespace logicmill;
using namespace util;

namespace shared_ptr_test
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

}    // namespace shared_ptr_test


TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr copy ctor }")
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
TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr move ctor }")
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

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr copy assign }")
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

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr move assign }")
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

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr copy construct poly }")
{
	util::shared_ptr<shared_ptr_test::bar> bp = util::shared_ptr<shared_ptr_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foo> fp{bp};
	CHECK(fp.use_count() == 2);
	CHECK(fp == bp);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
	CHECK(bp.use_count() == 1);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr move construct poly }")
{
	util::shared_ptr<shared_ptr_test::bar> bp = util::shared_ptr<shared_ptr_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foo> fp{std::move(bp)};
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr copy assign poly }")
{
	util::shared_ptr<shared_ptr_test::bar> bp = util::shared_ptr<shared_ptr_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foo> fp;
	fp = bp;
	CHECK(fp.use_count() == 2);
	CHECK(fp == bp);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
	CHECK(bp.use_count() == 1);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr move assign poly }")
{
	util::shared_ptr<shared_ptr_test::bar> bp = util::shared_ptr<shared_ptr_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foo> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr static_ptr_cast }")
{
	util::shared_ptr<shared_ptr_test::bar> bp = util::shared_ptr<shared_ptr_test::bar>::create(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foo> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	util::shared_ptr<shared_ptr_test::bar> barp = util::shared_ptr<shared_ptr_test::bar>::static_ptr_cast(fp);
	CHECK(fp.use_count() == 2);
	CHECK(barp->dnum() == 3.5);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr dynamic_ptr_cast }")
{
	util::shared_ptr<shared_ptr_test::barv> bp = util::shared_ptr<shared_ptr_test::barv>::create(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foov> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	util::shared_ptr<shared_ptr_test::barv> barvp = util::shared_ptr<shared_ptr_test::barv>::dynamic_ptr_cast(fp);
	CHECK(fp.use_count() == 2);
	CHECK(barvp->dnum() == 3.5);
}
TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr with deleter }")
{
	int* ip = shared_ptr_test::int_malloc{}();
	*ip = 27;
	util::shared_ptr<int> p = util::shared_ptr<int>{ip, shared_ptr_test::int_free{}};
	util::shared_ptr<int> p_copy{p};
	CHECK(p.use_count() == 2);
	CHECK(p_copy == p);
	CHECK(*p_copy == 27);
	p_copy.reset();
	CHECK(shared_ptr_test::int_free_call_count == 0);
	CHECK(!p_copy);
	CHECK(p.use_count() == 1);
	p.reset();
	CHECK(shared_ptr_test::int_free_call_count == 1);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr with allocate }")
{
	using zallocator = util::allocator<std::string, shared_ptr_test::malloc_policy<std::string>>;
	util::shared_ptr<std::string> sp = util::shared_ptr<std::string>::allocate(zallocator{}, "zoot");
	util::shared_ptr<std::string> sp_copy{sp};
	CHECK(sp.use_count() == 2);
	CHECK(sp_copy == sp);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
	CHECK(sp.use_count() == 1);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr with deleter and allocator }")
{
	shared_ptr_test::int_free_call_count = 0;
	using zallocator = util::allocator<std::string, shared_ptr_test::malloc_policy<std::string>>;
	int* ip = shared_ptr_test::int_malloc{}();
	*ip = 27;
	util::shared_ptr<int> p = util::shared_ptr<int>{ip, shared_ptr_test::int_free{}, zallocator{}};
	util::shared_ptr<int> p_copy{p};
	CHECK(p.use_count() == 2);
	CHECK(p_copy == p);
	CHECK(*p_copy == 27);
	p_copy.reset();
	CHECK(shared_ptr_test::int_free_call_count == 0);
	CHECK(!p_copy);
	CHECK(p.use_count() == 1);
	p.reset();
	CHECK(shared_ptr_test::int_free_call_count == 1);

	std::cout << "size of shared_ptr is " << sizeof(util::shared_ptr<int>) << std::endl;
	std::cout << "size of std::string is " << sizeof(std::string) << std::endl;
	std::cout << "size of control_blk is " << sizeof(util::shared_ptr<int>::control_blk) << std::endl;
	std::cout << "size of value_control_block is " << sizeof(util::shared_ptr<int>::value_control_block) << std::endl;
	std::cout << "size of control_blk_base is " << sizeof(util::detail::control_blk_base) << std::endl;
	std::cout << "size of control_blk_base::delete_erasure is " << sizeof(util::detail::control_blk_base::delete_erasure) << std::endl;
	std::cout << "size of control_blk_base::destruct_erasure is " << sizeof(util::detail::control_blk_base::destruct_erasure) << std::endl;
}

