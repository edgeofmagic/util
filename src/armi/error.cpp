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

#include <logicmill/armi/error.h>

using namespace logicmill;

class armi_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override;
	virtual std::string
	message(int ev) const noexcept override;
};

const char*
armi_category_impl::name() const noexcept
{
	return "armi";
}

std::string
armi_category_impl::message(int ev) const noexcept
{
	switch (static_cast<armi::errc>(ev))
	{
		case armi::errc::ok:
			return "success";
		case armi::errc::no_event_loop:
			return "no event loop";
		case armi::errc::channel_not_connected:
			return "channel not connected";
		case armi::errc::no_target_provided:
			return "no target instance provided";
		case armi::errc::uncaught_server_exception:
			return "uncaught exception thrown by target implementation instance";
		case armi::errc::exception_thrown_by_reply_handler:
			return "exception thrown by reply handler";
		case armi::errc::exception_thrown_by_member_func_stub:
			return "exception thrown by member function stub";
		case armi::errc::invalid_member_func_id:
			return "invalid member function id in request";
		case armi::errc::invalid_interface_id:
			return "invalid interface id in request";
		case armi::errc::invalid_argument_count:
			return "invalid argument count in marshaled member function or reply";
		case armi::errc::invalid_error_category:
			return "error category not found in context error category map";
		case armi::errc::invalid_channel_id:
			return "invalid channel id";
		case armi::errc::context_closed:
			return "context closed";
		case armi::errc::channel_closed:
			return "channel closed";
		case armi::errc::transport_closed:
			return "transport closed";
		default:
			return "unknown armi error";
	}
}

std::error_category const&
armi::error_category() noexcept
{
	static armi_category_impl instance;
	return instance;
}

std::error_condition
armi::make_error_condition(armi::errc e)
{
	return std::error_condition(static_cast<int>(e), armi::error_category());
}

std::error_code
armi::make_error_code(armi::errc e)
{
	return std::error_code(static_cast<int>(e), armi::error_category());
}