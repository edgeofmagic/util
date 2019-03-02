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

#ifndef LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H
#define LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H

#include <cstdint>
#include <functional>
// #include <logicmill/armi/interface_stub_base.h>
#include <logicmill/armi/transport.h>
#include <logicmill/armi/types.h>
// #include <logicmill/async/channel.h>
// #include <logicmill/async/loop.h>
#include <logicmill/bstream/ombstream.h>
#include <memory>
#include <unordered_set>

namespace logicmill
{
namespace armi
{

class fail_proxy;

class server_context_base
{ 
public:
	// using ptr = SHARED_PTR_TYPE<server_context_base>;

	template<class _T, class _U>
	friend class method_stub;

	server_context_base(transport::server& transport_server, bstream::context_base::ptr const& stream_context)
	: m_transport{transport_server}, m_stream_context{stream_context}
	{}

	virtual ~server_context_base() {}

	bstream::context_base::ptr const&
	stream_context()
	{
		return m_stream_context;
	}

	transport::server&
	get_transport() const
	{
		return m_transport;
	}

	// void
	// request_failed(std::uint64_t request_id, transport::server_channel::wptr const& transp, std::error_code err)
	// {
	// 	bstream::ombstream os{stream_context()};
	// 	os << request_id;
	// 	os << reply_kind::fail;
	// 	os.write_array_header(1);
	// 	os << err;
	// 	if (!transp.expired())
	// 		transp.lock()->send_reply(os.release_mutable_buffer());
	// }

	// void
	// handle_request(bstream::ibstream& is, channel_id_type channel_id);

	// std::unique_ptr<bstream::ombstream>
	// create_reply_stream()
	// {
	// 	return std::make_unique<bstream::ombstream>(m_stream_context);
	// }

protected:
	// void
	// cleanup();


	// virtual interface_stub_base&
	// get_type_erased_stub()
	// 		= 0;

	// virtual std::shared_ptr<void> const&
	// get_type_erased_impl()
	// 		= 0;

	transport::server& m_transport;
	bstream::context_base::ptr m_stream_context;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H
