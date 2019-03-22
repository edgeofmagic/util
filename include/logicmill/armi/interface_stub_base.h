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
#include <logicmill/armi/server_context_base.h>
#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/ombstream.h>

namespace logicmill
{
namespace armi
{
template<class SerializationTraits, class TransportTraits>
class server_context_base;

template<class Target, class ServerContextBase>
class member_func_stub_base;

template<class Target, class ServerContextBase>
class interface_stub_base;

template<
		class Target,
		template<class...> class ServerContextBaseTemplate,
		class SerializationTraits,
		class TransportTraits>
class interface_stub_base<Target, ServerContextBaseTemplate<SerializationTraits, TransportTraits>>
{
public:
	using server_context_base_type = ServerContextBaseTemplate<SerializationTraits, TransportTraits>;
	using serialization_traits     = SerializationTraits;
	using deserializer_type        = typename serialization_traits::deserializer_type;
	using serializer_type          = typename serialization_traits::serializer_type;
	using serializer_uptr          = std::unique_ptr<serializer_type>;

	using transport_traits         = TransportTraits;
	using channel_type             = typename transport_traits::channel_type;
	using channel_param_type       = typename transport_traits::channel_param_type;
	using channel_const_param_type = typename transport_traits::channel_const_param_type;

protected:
	friend class server_context_type;

	interface_stub_base(server_context_base_type* context_base) : m_context{context_base} {}

	server_context_base_type*
	context()
	{
		return m_context;
	}

	void
	request_failed(request_id_type request_id, channel_param_type channel, std::error_code err)
	{
		serializer_uptr reply{m_context->create_reply_serializer()};
		serialization_traits::write(*reply, request_id);
		serialization_traits::write(*reply, reply_kind::fail);
		serialization_traits::write_sequence_prefix(*reply, 1);
		serialization_traits::write(*reply, err);
		m_context->send_reply(channel, std::move(reply));
	}

	server_context_base_type* m_context;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_INTERFACE_STUB_BASE_H
