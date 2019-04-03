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
#include <logicmill/armi/serialization_traits.h>
#include <logicmill/armi/transport_traits.h>
#include <logicmill/armi/adapters/bridge.h>
#include <logicmill/armi/types.h>
#include <memory>
#include <unordered_set>

namespace logicmill
{
namespace armi
{

template<class SerializationTraits, class TransportTraits>
class server_context_base
{
public:

	using serialization_traits = SerializationTraits;
	using transport_traits = TransportTraits;
	using bridge_type = typename armi::adapters::bridge<serialization_traits, transport_traits>;
	using serializer_param_type = typename bridge_type::serializer_param_type;

	template<class _T, class _U, class _V, class _Enable>
	friend class member_func_stub;

	template<class Target, class ServerContextBase>
	friend class member_func_stub_base;

	template<class Target, class ServerContextBase>
	friend class interface_stub_base;

	server_context_base()
	{}

	virtual ~server_context_base() {}


protected:

	virtual void
	send_reply(channel_id_type channel, serializer_param_type reply)
			= 0;

	virtual bool
	is_valid_channel(channel_id_type channel)
			= 0;

	virtual void
	close(channel_id_type channel)
			= 0;

};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H
