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

#include <logicmill/ras/error.h>

using namespace logicmill;

class ras_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override;
	virtual std::string
	message(int ev) const noexcept override;
};

const char*
ras_category_impl::name() const noexcept
{
	return "ras";
}

std::string
ras_category_impl::message(int ev) const noexcept
{
	switch (static_cast<ras::errc>(ev))
	{
	case ras::errc::ok:
		return "success";
	case ras::errc::transport_not_set:
		return "transport not set for context";
	case ras::errc::no_event_loop:
		return "no event loop";
	case ras::errc::channel_not_connected:
		return "channel not connected";
	case ras::errc::no_implementation_instance_registered:
		return "no implementation instance registered for target type";
	case ras::errc::uncaught_server_exception:
		return "uncaught exception thrown by target implementation instance";
	case ras::errc::exception_thrown_by_reply_handler:
		return "exception thrown by reply handler";
	case ras::errc::exception_thrown_by_method_stub:
		return "exception thrown by method stub";
	case ras::errc::invalid_method_id:
		return "invalid method id in request";
	case ras::errc::invalid_interface_id:
		return "invalid interface id in request";
	case ras::errc::invalid_argument_count:
		return "invalid argument count in marshaled method or reply";
	case ras::errc::invalid_error_category:
		return "error category not found in context error category map";
	default:
		return "unknown ras error";
	}
}

std::error_category const&
ras::error_category() noexcept
{
	static ras_category_impl instance;
	return instance;
}

std::error_condition
ras::make_error_condition(ras::errc e)
{
	return std::error_condition(static_cast<int>(e), ras::error_category());
}

std::error_code
ras::make_error_code(ras::errc e)
{
	return std::error_code(static_cast<int>(e), ras::error_category());
}