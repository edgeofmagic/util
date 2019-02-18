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

#ifndef LOGICMILL_ARMI_FAIL_PROXY_H
#define LOGICMILL_ARMI_FAIL_PROXY_H

#include <cstdint>
// #include <logicmill/async/channel.h>
#include <logicmill/armi/server_context_base.h>
#include <logicmill/armi/transport.h>
#include <system_error>

namespace logicmill
{
namespace armi
{
class server_context_base;

class fail_proxy
{
public:
	fail_proxy(
			std::uint64_t                     req_ord,
			transport::server_channel::ptr const&     chan,
			bstream::context_base::ptr const& stream_context)
		: m_req_ord{req_ord}, m_channel{chan}, m_stream_context{stream_context}
	{}

	fail_proxy(fail_proxy const& other)
		: m_req_ord{other.m_req_ord}, m_channel{other.m_channel}, m_stream_context{other.m_stream_context}
	{}

	fail_proxy(fail_proxy&& other)
		: m_req_ord{other.m_req_ord},
		  m_channel{std::move(other.m_channel)},
		  m_stream_context{std::move(other.m_stream_context)} // TODO: don't move after this is fixed (to ref)
	{}

	void
	operator()(std::error_code ec);

private:
	std::uint64_t              m_req_ord;
	transport::server_channel::ptr     m_channel;
	bstream::context_base::ptr m_stream_context;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_FAIL_PROXY_H
