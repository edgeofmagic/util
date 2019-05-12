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

#include <util/error.h>


class util_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override
	{
		return "util";
	}

	virtual std::string
	message(int ev) const noexcept override
	{
		switch (static_cast<util::errc>(ev))
		{
			case util::errc::ok:
				return "success";
			case util::errc::invalid_error_category:
				return "error category not found in error context";
			default:
				return "unknown util error";
		}
	}
};

std::error_category const&
util::error_category() noexcept
{
	static util_category_impl instance;
	return instance;
}

std::error_condition
util::make_error_condition(errc e)
{
	return std::error_condition(static_cast<int>(e), util::error_category());
}

std::error_code
util::make_error_code(errc e)
{
	return std::error_code(static_cast<int>(e), util::error_category());
}
