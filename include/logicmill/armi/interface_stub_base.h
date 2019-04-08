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

#ifndef LOGICMILL_ARMI_INTERFACE_STUB_BASE_H
#define LOGICMILL_ARMI_INTERFACE_STUB_BASE_H

#include <cstdint>
#include <logicmill/armi/adapters/bridge.h>
#include <logicmill/armi/server_stub_base.h>
#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/ombstream.h>

namespace logicmill
{
namespace armi
{
template<class SerializationTraits, class AsyncIOTraits>
class server_stub_base;

template<class Target, class ServerStubBase>
class member_func_stub_base;

template<class Target, class ServerStubBase>
class interface_stub_base;

template<
		class Target,
		template<class...> class ServerStubBaseTemplate,
		class SerializationTraits,
		class AsyncIOTraits>
class interface_stub_base<Target, ServerStubBaseTemplate<SerializationTraits, AsyncIOTraits>>
{
public:
	using server_stub_base_type = ServerStubBaseTemplate<SerializationTraits, AsyncIOTraits>;
	using serialization_traits     = SerializationTraits;
	using async_io_traits         = AsyncIOTraits;
	using deserializer_type        = typename serialization_traits::deserializer_type;
	using serializer_type          = typename serialization_traits::serializer_type;
	using bridge_type              = logicmill::armi::adapters::bridge<serialization_traits, async_io_traits>;
	using serializer_param_type    = typename bridge_type::serializer_param_type;

protected:
	friend class server_stub_type;

	interface_stub_base(server_stub_base_type* server) : m_server{server} {}

	server_stub_base_type*
	server_stub()
	{
		return m_server;
	}

	void
	request_failed(request_id_type request_id, channel_id_type channel, std::error_code err)
	{
		bridge_type::new_serializer([=](typename bridge_type::serializer_param_type reply) {
			serialization_traits::write(reply, request_id);
			serialization_traits::write(reply, reply_kind::fail);
			serialization_traits::write_sequence_prefix(reply, 1);
			serialization_traits::write(reply, err);
			m_server->send_reply(channel, reply);
		});
	}

	server_stub_base_type* m_server;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_INTERFACE_STUB_BASE_H
