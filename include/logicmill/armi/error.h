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

#ifndef LOGICMILL_ARMI_ERROR_H
#define LOGICMILL_ARMI_ERROR_H

#include <string>
#include <system_error>

namespace logicmill
{
namespace armi
{

enum class errc : int
{
	ok = 0,
	no_event_loop,
	channel_not_connected,
	no_target_provided,
	uncaught_server_exception,
	exception_thrown_by_reply_handler,
	exception_thrown_by_member_func_stub,
	invalid_member_func_id,
	invalid_interface_id,
	invalid_argument_count,
	invalid_argument_type,
	invalid_error_category,
	invalid_channel_id,
	context_closed,
	channel_closed,
	transport_closed,

};

std::error_category const&
error_category() noexcept;

std::error_condition
make_error_condition(errc e);

std::error_code
make_error_code(errc e);


}    // namespace armi
}    // namespace logicmill

namespace std
{

template<>
struct is_error_condition_enum<logicmill::armi::errc> : public true_type
{};

}    // namespace std

#endif    // LOGICMILL_ARMI_ERROR_H
