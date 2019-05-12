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
#include <util/error_context.h>
#include <util/error.h>

using namespace util;

namespace foo
{

enum class errc
{
	ok = 0,
	end_of_the_world_as_we_know_it,
	sun_exploded,
	aliens_invaded,
};

class foo_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override
	{
		return "foo";
	}

	virtual std::string
	message(int ev) const noexcept override
	{
		switch (static_cast<foo::errc>(ev))
		{
			case foo::errc::ok:
				return "success";
			case foo::errc::end_of_the_world_as_we_know_it:
				return "end of the world as we know it";
			case foo::errc::sun_exploded:
				return "sun exploded";
			case foo::errc::aliens_invaded:
				return "aliens invaded";
			default:
				return "unknown foo error";
		}
	}
};

inline std::error_category const&
error_category() noexcept
{
	static foo::foo_category_impl instance;
	return instance;
}

inline std::error_condition
make_error_condition(errc e)
{
	return std::error_condition(static_cast<int>(e), foo::error_category());
}

inline std::error_code
make_error_code(errc e)
{
	return std::error_code(static_cast<int>(e), foo::error_category());
}

}    // namespace foo

namespace bar
{

enum class errc
{
	ok = 0,
	end_of_the_world_as_we_know_it,
	sun_exploded,
	aliens_invaded,
};

class bar_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override
	{
		return "bar";
	}

	virtual std::string
	message(int ev) const noexcept override
	{
		switch (static_cast<bar::errc>(ev))
		{
			case bar::errc::ok:
				return "success";
			case bar::errc::end_of_the_world_as_we_know_it:
				return "end of the world as we know it";
			case bar::errc::sun_exploded:
				return "sun exploded";
			case bar::errc::aliens_invaded:
				return "aliens invaded";
			default:
				return "unknown bar error";
		}
	}
};

inline std::error_category const&
error_category() noexcept
{
	static bar::bar_category_impl instance;
	return instance;
}

inline std::error_condition
make_error_condition(errc e)
{
	return std::error_condition(static_cast<int>(e), bar::error_category());
}

inline std::error_code
make_error_code(errc e)
{
	return std::error_code(static_cast<int>(e), bar::error_category());
}

}    // namespace bar

template<>
struct std::is_error_condition_enum<foo::errc> : public true_type
{};

template<>
struct std::is_error_condition_enum<bar::errc> : public true_type
{};


namespace cat
{
UTIL_DEFINE_ERROR_CONTEXT(fcontext, foo::error_category, bar::error_category);
UTIL_DEFINE_ERROR_CONTEXT(gcontext, foo::error_category);
}

TEST_CASE("error_context [ smoke ] { error category context macro }")
{
	auto bar_idx = cat::fcontext::index_of_category(bar::error_category());
	CHECK(bar_idx == 3);
	auto& foo_cat = cat::fcontext::category_from_index(2);
	CHECK(foo_cat == foo::error_category());
	auto sys_idx = util::default_error_context::index_of_category(std::system_category());
	CHECK(sys_idx == 0);
}

TEST_CASE("error_context [ smoke ] { invalid category failure }")
{
	auto& foo_cat = cat::gcontext::category_from_index(2);
	CHECK(foo_cat == foo::error_category());
	
	bool exception_caught{false};
	try
	{
		auto bar_idx = cat::gcontext::index_of_category(bar::error_category());
		CHECK(false);
	}
	catch(const std::system_error& e)
	{
		CHECK(e.code() == util::errc::invalid_error_category);
		exception_caught = true;
	}
	CHECK(exception_caught);
	
}
