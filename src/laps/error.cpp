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

#include <logicmill/laps/types.h>

using namespace logicmill;
using namespace laps;

class laps_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override;

	virtual std::string
	message(int ev) const noexcept override;
};

const char*
laps_category_impl::name() const noexcept
{
	return "laps";
}

std::string
laps_category_impl::message(int ev) const noexcept
{
	switch (static_cast<laps::errc>(ev))
	{

		case laps::errc::ok:
			return "ok";
		case laps::errc::not_writable:
			return "not writable";
		case laps::errc::already_reading:
			return "already reading";
		case laps::errc::not_reading:
			return "not reading";
		case laps::errc::cannot_resume_read:
			return "cannot resume read (no read handler)";
		default:
			return "unknown laps error";
	}
}

std::error_category const&
laps::error_category() noexcept
{
	static laps_category_impl instance;
	return instance;
}

std::error_condition
laps::make_error_condition(laps::errc e)
{
	return std::error_condition(static_cast<int>(e), laps::error_category());
}

std::error_code
laps::make_error_code(laps::errc e)
{
	return std::error_code(static_cast<int>(e), laps::error_category());
}
