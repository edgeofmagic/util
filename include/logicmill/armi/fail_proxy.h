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
			request_id_type                     request_id,
			channel_id_type     channel_id,
			server_context_base& context)
		: m_request_id{request_id}, m_channel_id{channel_id}, m_context{context}
	{}

	fail_proxy(fail_proxy const& other)
		: m_request_id{other.m_request_id}, m_channel_id{other.m_channel_id}, m_context{other.m_context}
	{}

	fail_proxy(fail_proxy&& other)
		: m_request_id{other.m_request_id},
		  m_channel_id{other.m_channel_id},
		  m_context{other.m_context} 
	{}

	void
	operator()(std::error_code err)
	{
		bstream::ombstream os{m_context.stream_context()};
		os << m_request_id;
		os << reply_kind::fail;
		os.write_array_header(1);
		os << err;
		m_context.get_transport().send_reply(m_channel_id, os.release_mutable_buffer());
	}

private:
	request_id_type              m_request_id;
	channel_id_type     m_channel_id;
	server_context_base& m_context;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_FAIL_PROXY_H
