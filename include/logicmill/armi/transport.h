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
#include <logicmill/armi/types.h>
#include <logicmill/bstream/context.h>
#include <logicmill/util/buffer.h>
#include <system_error>

namespace logicmill
{
namespace armi
{
namespace transport
{

class client
{
public:
	virtual bool
	is_valid_channel(channel_id_type channel_id)
			= 0;

	virtual void
	close(channel_id_type channel_id)
			= 0;

	virtual void
	send_request(
			channel_id_type           channel_id,
			request_id_type           request_id,
			std::chrono::milliseconds timeout,
			util::mutable_buffer&&    req)
			= 0;
};

class server
{
public:
	virtual bool
	is_valid_channel(channel_id_type channel)
			= 0;

	virtual void
	close(channel_id_type channel_id)
			= 0;

	virtual void
	send_reply(channel_id_type channel, util::mutable_buffer&& req)
			= 0;
};

}    // namespace transport
}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_TRANSPORT_H
