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
#include <logicmill/util/allocator.h>
#include <logicmill/util/shared_ptr.h>

using namespace logicmill;
using namespace util;

namespace shared_ptr_test
{

static bool free_mem_called{false};

void*
get_mem(unsigned long bytes)
{
	std::cout << "get_mem: " << bytes << std::endl;
	return ::malloc(bytes);
}

void
free_mem(void* p, unsigned long bytes)
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

	int
	num() const
	{
		return m_ival;
	}

	std::string const&
	str() const
	{
		return m_sval;
	}

private:
	int         m_ival;
	std::string m_sval;
};

class bar : public foo
{
public:
	bar(int i, std::string const& s, double d) : foo{i, s}, m_fpnum{d} {}

	double
	dnum() const
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

	int
	num() const
	{
		return m_ival;
	}

	std::string const&
	str() const
	{
		return m_sval;
	}

private:
	int         m_ival;
	std::string m_sval;
};

class barv : public foov
{
public:
	barv(int i, std::string const& s, double d) : foov{i, s}, m_fpnum{d} {}

	double
	dnum() const
	{
		return m_fpnum;
	}

private:
	double m_fpnum;
};

struct int_malloc
{
	int*
	operator()() const
	{
		return static_cast<int*>(::malloc(sizeof(int)));
	}
};

static int int_free_call_count{0};

struct int_free
{
	void
	operator()(int* p) const
	{
		++int_free_call_count;
		::free(p);
	}
};

struct containing_class;

struct contained_member : util::enable_shared_from_this<contained_member>
{
	contained_member(containing_class& o, bool& flag) : owner{o}, destruct_flag{flag} {}

	~contained_member()
	{
		destruct_flag = true;
	}

	containing_class&
	container() const
	{
		return owner;
	}

	util::shared_ptr<contained_member>
	self();

	containing_class& owner;
	bool& destruct_flag;
};

struct containing_class : util::enable_shared_from_this<containing_class>
{
	containing_class(bool& flag, bool& member_flag) : destruct_flag{flag}, member{*this, member_flag} {}

	~containing_class() 
	{
		destruct_flag = true;
	}

	util::shared_ptr<containing_class>
	self()
	{
		return shared_from_this();
	}

	bool& destruct_flag;
	contained_member member;
};

util::shared_ptr<contained_member>
shared_ptr_test::contained_member::self()
{
	return util::shared_ptr<contained_member>(owner.self(), this);
}


}    // namespace shared_ptr_test


TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr copy ctor }")
{
	util::shared_ptr<std::string> sp = util::make_shared<std::string>("zoot");
	util::shared_ptr<std::string> sp_copy{sp};
	CHECK(sp.use_count() == 2);
	CHECK(sp_copy == sp);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
	CHECK(sp.use_count() == 1);
}


TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr make_shared copy ctor }")
{
	util::shared_ptr<std::string> sp = util::make_shared<std::string>("zoot");
	util::shared_ptr<std::string> sp_copy{sp};
	CHECK(sp.use_count() == 2);
	CHECK(sp_copy == sp);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
	CHECK(sp.use_count() == 1);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr allocate_shared copy ctor }")
{
	util::shared_ptr<std::string> sp = util::allocate_shared<std::string>(std::allocator<std::string>{}, "zoot");
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
	util::shared_ptr<std::string> sp = util::make_shared<std::string>("zoot");
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
	util::shared_ptr<std::string> sp = util::make_shared<std::string>("zoot");
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
	util::shared_ptr<std::string> sp = util::make_shared<std::string>("zoot");
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
	util::shared_ptr<shared_ptr_test::bar> bp = util::make_shared<shared_ptr_test::bar>(7, "groot", 3.5);
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
	util::shared_ptr<shared_ptr_test::bar> bp = util::make_shared<shared_ptr_test::bar>(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foo> fp{std::move(bp)};
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	fp.reset();
	CHECK(!fp);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr copy assign poly }")
{
	util::shared_ptr<shared_ptr_test::bar> bp = util::make_shared<shared_ptr_test::bar>(7, "groot", 3.5);
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
	util::shared_ptr<shared_ptr_test::bar> bp = util::make_shared<shared_ptr_test::bar>(7, "groot", 3.5);
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
	util::shared_ptr<shared_ptr_test::bar> bp = util::make_shared<shared_ptr_test::bar>(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foo> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	util::shared_ptr<shared_ptr_test::bar> barp = util::static_pointer_cast<shared_ptr_test::bar>(fp);
	CHECK(fp.use_count() == 2);
	CHECK(barp->dnum() == 3.5);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr dynamic_ptr_cast }")
{
	util::shared_ptr<shared_ptr_test::barv> bp = util::make_shared<shared_ptr_test::barv>(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foov> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	util::shared_ptr<shared_ptr_test::barv> barvp = util::dynamic_pointer_cast<shared_ptr_test::barv>(fp);
	CHECK(fp.use_count() == 2);
	CHECK(barvp->dnum() == 3.5);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr dynamic_pointer_cast }")
{
	util::shared_ptr<shared_ptr_test::barv> bp = util::make_shared<shared_ptr_test::barv>(7, "groot", 3.5);
	util::shared_ptr<shared_ptr_test::foov> fp;
	fp = std::move(bp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	util::shared_ptr<shared_ptr_test::barv> barvp = util::dynamic_pointer_cast<shared_ptr_test::barv>(fp);
	CHECK(fp.use_count() == 2);
	CHECK(barvp->dnum() == 3.5);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr dynamic_pointer_cast failure}")
{
	util::shared_ptr<shared_ptr_test::foov> fp = util::make_shared<shared_ptr_test::foov>(7, "groot");
	util::shared_ptr<shared_ptr_test::barv> bp = util::dynamic_pointer_cast<shared_ptr_test::barv>(fp);
	CHECK(!bp);
	CHECK(fp.use_count() == 1);
	CHECK(fp->num() == 7);
	fp = nullptr;
	CHECK(!fp);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr with deleter }")
{
	int* ip                 = shared_ptr_test::int_malloc{}();
	*ip                     = 27;
	util::shared_ptr<int> p = util::shared_ptr<int>{ip, shared_ptr_test::int_free{}};
	util::shared_ptr<int> p_copy{p};
	CHECK(p.use_count() == 2);
	CHECK(p_copy == p);
	CHECK(*p_copy == 27);
	// p_copy.reset();
	p_copy = nullptr;
	CHECK(shared_ptr_test::int_free_call_count == 0);
	CHECK(!p_copy);
	CHECK(p.use_count() == 1);
	p.reset();
	CHECK(shared_ptr_test::int_free_call_count == 1);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr with allocate }")
{
	using zallocator                 = util::allocator<std::string, shared_ptr_test::malloc_policy<std::string>>;
	util::shared_ptr<std::string> sp = util::allocate_shared<std::string>(zallocator{}, "zoot");
	util::shared_ptr<std::string> sp_copy{sp};
	CHECK(sp.use_count() == 2);
	CHECK(sp_copy == sp);
	CHECK(*sp_copy == "zoot");
	sp_copy.reset();
	CHECK(!sp_copy);
	CHECK(sp.use_count() == 1);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr with allocate_shared }")
{
	using zallocator                 = util::allocator<std::string, shared_ptr_test::malloc_policy<std::string>>;
	util::shared_ptr<std::string> sp = util::allocate_shared<std::string>(zallocator{}, "zoot");
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
	using zallocator                     = util::allocator<int, shared_ptr_test::malloc_policy<int>>;
	int* ip                              = shared_ptr_test::int_malloc{}();
	*ip                                  = 27;
	util::shared_ptr<int> p              = util::shared_ptr<int>{ip, shared_ptr_test::int_free{}, zallocator{}};
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
	std::cout
			<< "size of ptr_ctrl_blk is "
			<< sizeof(util::detail::
							  ptr_ctrl_blk<std::string, std::default_delete<std::string>, std::allocator<std::string>>)
			<< std::endl;
	std::cout << "size of value_control_block is "
			  << sizeof(util::detail::value_ctrl_blk<std::string, std::allocator<std::string>>) << std::endl;
	std::cout << "size of control_blk is " << sizeof(util::detail::ctrl_blk) << std::endl;
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { util::shared_ptr construct from unique_ptr }")
{
	std::unique_ptr<std::string>  ups{new std::string{"zoot"}};
	util::shared_ptr<std::string> sps{std::move(ups)};
	CHECK(*sps == "zoot");
	CHECK(ups.get() == nullptr);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { weak_ptr construct from shared_ptr }")
{
	auto                        sp0 = util::make_shared<std::string>("zoot");
	CHECK(sp0.weak_count() == 1);
	util::weak_ptr<std::string> wp{sp0};
	CHECK(wp.weak_count() == 2);
	CHECK(wp.use_count() == 1);
	CHECK(sp0.weak_count() == 2);
	CHECK(sp0.use_count() == 1);
	auto sp1 = wp.lock();
	CHECK(sp1.use_count() == 2);
	util::shared_ptr<std::string> sp2{wp};
	CHECK(sp2.use_count() == 3);
	sp2.reset();
	sp1.reset();
	CHECK(sp0.use_count() == 1);
	CHECK(wp.weak_count() == 2);
	CHECK(sp1.use_count() == 0);
	CHECK(sp2.use_count() == 0);
	sp0.reset();
	CHECK(wp.use_count() == 0);
	CHECK(wp.weak_count() == 1);
	sp1 = wp.lock();
	CHECK(!sp1);
	CHECK(wp.use_count() == 0);
	CHECK(wp.weak_count() == 1);
	bool caught{false};
	try
	{
		util::shared_ptr<std::string> sp3{wp};
		CHECK(false);
	}
	catch (std::bad_weak_ptr const& e)
	{
		caught = true;
	}
	CHECK(caught);
}

TEST_CASE("logicmill::util::shared_ptr [ smoke ] { shared alias }")
{
	using namespace shared_ptr_test;
	bool outer_destruct{false};
	bool inner_destruct{false};

	auto original = util::make_shared<containing_class>(outer_destruct, inner_destruct);
	CHECK(original.use_count() == 1);
	CHECK(original.weak_count() == 2);

	util::shared_ptr<contained_member> member_alias = util::shared_ptr<contained_member>(original, &(original->member));
	CHECK(original.use_count() == 2);
	CHECK(member_alias.use_count() == 2);
	CHECK(original.weak_count() == 2);

	auto copy_from_member = member_alias->container().self();
	CHECK(original.use_count() == 3);
	CHECK(member_alias.use_count() == 3);
	CHECK(copy_from_member.use_count() == 3);
	CHECK(original.weak_count() == 2);

	auto copy_of_original = original;
	auto copy_of_member = member_alias;
	CHECK(original.use_count() == 5);
	CHECK(member_alias.use_count() == 5);
	CHECK(copy_from_member.use_count() == 5);
	CHECK(copy_of_original.use_count() == 5);
	CHECK(copy_of_member.use_count() == 5);
	CHECK(original.weak_count() == 2);

	auto copy_of_original_from_self = original->self();
	auto copy_of_member_from_self = member_alias->self();
	CHECK(original.use_count() == 7);
	CHECK(member_alias.use_count() == 7);
	CHECK(copy_from_member.use_count() == 7);
	CHECK(copy_of_original.use_count() == 7);
	CHECK(copy_of_member.use_count() == 7);
	CHECK(copy_of_original_from_self.use_count() == 7);
	CHECK(copy_of_member_from_self.use_count() == 7);
	CHECK(original.weak_count() == 2);

	CHECK(!outer_destruct);
	CHECK(!inner_destruct);

	original.reset();
	CHECK(member_alias.use_count() == 6);
	CHECK(copy_from_member.use_count() == 6);
	CHECK(copy_of_original.use_count() == 6);
	CHECK(copy_of_member.use_count() == 6);
	CHECK(copy_of_original_from_self.use_count() == 6);
	CHECK(copy_of_member_from_self.use_count() == 6);
	CHECK(original.weak_count() == 0);
	CHECK(member_alias.weak_count() == 2);

	member_alias.reset();
	CHECK(copy_from_member.use_count() == 5);
	CHECK(copy_of_original.use_count() == 5);
	CHECK(copy_of_member.use_count() == 5);
	CHECK(copy_of_original_from_self.use_count() == 5);
	CHECK(copy_of_member_from_self.use_count() == 5);
	CHECK(original.weak_count() == 0);
	CHECK(member_alias.weak_count() == 0);
	CHECK(copy_from_member.weak_count() == 2);


	copy_from_member.reset();
	CHECK(copy_of_original.use_count() == 4);
	CHECK(copy_of_member.use_count() == 4);
	CHECK(copy_of_original_from_self.use_count() == 4);
	CHECK(copy_of_member_from_self.use_count() == 4);
	CHECK(original.weak_count() == 0);

	copy_of_original.reset();
	CHECK(copy_of_member.use_count() == 3);
	CHECK(copy_of_original_from_self.use_count() == 3);
	CHECK(copy_of_member_from_self.use_count() == 3);

	copy_of_member.reset();
	CHECK(copy_of_original_from_self.use_count() == 2);
	CHECK(copy_of_member_from_self.use_count() == 2);

	auto weak_original = util::weak_ptr<containing_class>(copy_of_original_from_self);
	CHECK(copy_of_original_from_self.use_count() == 2);
	CHECK(copy_of_member_from_self.use_count() == 2);
	CHECK(copy_of_original_from_self.weak_count() == 3);
	CHECK(copy_of_member_from_self.weak_count() == 3);
	CHECK(weak_original.use_count() == 2);
	CHECK(weak_original.weak_count() == 3);

	auto weak_member = util::weak_ptr<contained_member>(copy_of_member_from_self);

	CHECK(copy_of_original_from_self.use_count() == 2);
	CHECK(copy_of_member_from_self.use_count() == 2);
	CHECK(copy_of_original_from_self.weak_count() == 4);
	CHECK(copy_of_member_from_self.weak_count() == 4);
	CHECK(weak_original.use_count() == 2);
	CHECK(weak_member.use_count() == 2);
	CHECK(weak_original.weak_count() == 4);
	CHECK(weak_member.weak_count() == 4);

	copy_of_original_from_self.reset();
	CHECK(copy_of_member_from_self.use_count() == 1);
	CHECK(copy_of_member_from_self.weak_count() == 4);
	CHECK(weak_original.use_count() == 1);
	CHECK(weak_member.use_count() == 1);
	CHECK(weak_original.weak_count() == 4);
	CHECK(weak_member.weak_count() == 4);

	CHECK(!outer_destruct);
	CHECK(!inner_destruct);

	copy_of_member_from_self.reset();

	CHECK(outer_destruct);
	CHECK(inner_destruct);

}