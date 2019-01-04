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

#include <logicmill/util/allocator.h>
#include <doctest.h>
#include <iostream>
#include <unordered_map>

using namespace logicmill;
using namespace util;

namespace alloc_test
{
	void* get_mem(unsigned long bytes)
	{
		return ::malloc(bytes);
	}

	void free_mem(void* p, unsigned long /* bytes */)
	{
		::free(p);
	}

	UTIL_ALLOCATOR_POLICY(test_policy, get_mem, free_mem);
}

TEST_CASE( "logicmill::util::allocators [ smoke ] {}" )
{
	using falloc = allocator<std::uint8_t, alloc_test::test_policy<std::uint8_t>>;
	falloc f;
	auto p = f.allocate(1024);
	f.deallocate(p, 1024);
}

TEST_CASE("logicmill::util::allocators [ smoke ] { macro policy }")
{
	using map_type = std::unordered_map<
			std::string,
			int,
			std::hash<std::string>,
			std::equal_to<std::string>,
			logicmill::util::allocator<
					std::pair<const std::string, int>,
					alloc_test::test_policy<std::pair<const std::string, int>>>>;

	UTIL_ALLOCATOR_ALIAS(alloc_type, alloc_test::test_policy, std::pair<const std::string, int> );

	// using alloc_type = logicmill::util::allocator<
	// 		std::pair<const std::string, int>,
	// 		alloc_test::test_policy<std::pair<const std::string, int>>>;

	CHECK(std::is_same< std::pair<const std::string, int>, map_type::value_type >::value);
	CHECK(std::is_same< std::pair<const std::string, int>, alloc_type::value_type >::value);

	using other_alloc_type = logicmill::util::allocator<std::string, alloc_test::test_policy<std::string>>;
	using different_alloc_type = std::allocator<std::string>;

	alloc_type a1;
	other_alloc_type a2;
	different_alloc_type a3;

	CHECK(a1 == a2);

	CHECK(a1 != a3);
	CHECK(a2 != a3);

	map_type smap;
	smap.emplace("hello", 1);
	smap.emplace("kitty", 2);

	auto it = smap.find("hello");
	CHECK(it->second == 1);

	it = smap.find("kitty");
	CHECK(it->second == 2);
}

TEST_CASE("logicmill::util::allocators [ smoke ] { heap policy }")
{
	using map_type = std::unordered_map<
			std::string,
			int,
			std::hash<std::string>,
			std::equal_to<std::string>,
			logicmill::util::allocator<std::pair<const std::string, int>>>;

	using alloc_type = logicmill::util::allocator<std::pair<const std::string, int>>;

	CHECK(std::is_same< std::pair<const std::string, int>, map_type::value_type >::value);
	CHECK(std::is_same< std::pair<const std::string, int>, alloc_type::value_type >::value);

	using other_alloc_type = logicmill::util::allocator<std::string>;
	using different_alloc_type = std::allocator<std::string>;

	alloc_type a1;
	other_alloc_type a2;
	different_alloc_type a3;

	CHECK(a1 == a2);

	CHECK(a1 != a3);
	CHECK(a2 != a3);

	map_type smap;
	smap.emplace("hello", 1);
	smap.emplace("kitty", 2);

	auto it = smap.find("hello");
	CHECK(it->second == 1);

	it = smap.find("kitty");
	CHECK(it->second == 2);
}