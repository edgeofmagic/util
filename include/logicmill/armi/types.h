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

/* 
 * File:   types.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 8:20 PM
 */

#ifndef LOGICMILL_ARMI_TYPES_H
#define LOGICMILL_ARMI_TYPES_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <logicmill/armi/error.h>
#include <logicmill/bstream/context.h>
#include <system_error>

namespace logicmill
{
namespace armi
{
using fail_reply = std::function<void(std::error_code ec)>;

using millisecs = std::chrono::milliseconds;

inline bstream::context_base const&
get_default_stream_context()
{
	static const bstream::context<> default_context({&armi::error_category()});
	return default_context;
}

enum class reply_kind
{
	normal,
	fail
};

template<std::size_t N>
inline bool
expected_count(std::size_t count)
{
	return count == N;
}

}    // namespace armi
}    // namespace logicmill


#endif /* LOGICMILL_ARMI_TYPES_H */
