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

#ifndef LOGICMILL_ARMI_METHOD_STUB_BASE_H
#define LOGICMILL_ARMI_METHOD_STUB_BASE_H

#include <cstdint>
// #include <logicmill/async/channel.h>
#include <logicmill/armi/transport.h>
#include <logicmill/bstream/ibstream.h>

namespace logicmill
{
namespace armi
{

class method_stub_base
{
public:
	virtual ~method_stub_base() {}
	virtual void
	dispatch(std::uint64_t req_id, transport::server_channel::ptr const& chan, bstream::ibstream& is) const = 0;

	static void
	request_failed(
			std::uint64_t                         request_id,
			transport::server_channel::ptr const& transp,
			bstream::context_base::ptr const&     stream_context,
			std::error_code                       err)
	{
		bstream::ombstream os{stream_context};
		os << request_id;
		os << reply_kind::fail;
		os.write_array_header(1);
		os << err;
		transp->send_reply(os.release_mutable_buffer());
	}
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_METHOD_STUB_BASE_H
