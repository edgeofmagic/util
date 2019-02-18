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

#ifndef LOGICMILL_ARMI_TRANSPORT_H
#define LOGICMILL_ARMI_TRANSPORT_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
// #include <logicmill/armi/client_context_base.h>
// #include <logicmill/armi/error.h>
// #include <logicmill/armi/server_context_base.h>
#include <logicmill/bstream/context.h>
#include <logicmill/util/buffer.h>
#include <system_error>

namespace logicmill
{
namespace armi
{
namespace transport
{

class server_channel
{
public:
	using ptr = SHARED_PTR_TYPE<server_channel>;

	// virtual void
	// send_reply(std::deque<util::mutable_buffer>&& req) = 0;

	virtual void
	send_reply(util::mutable_buffer&& req)
			= 0;
};

class client_channel
{
public:
	using ptr = SHARED_PTR_TYPE<client_channel>;

	using close_handler = std::function<void()>;

	virtual void close(close_handler) = 0;

	virtual void close() = 0;

	virtual void
	send_request(
			std::uint64_t             request_id,
			std::chrono::milliseconds timeout,
			util::mutable_buffer&&    req,
			std::error_code&          err)
			= 0;

	// reply by calling client_context_base::handle_reply(buffer);
	// timeout should invoke client_context_base::cancel_request(request_id, std::error_code);
};

}    // namespace transport
}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_TRANSPORT_H
