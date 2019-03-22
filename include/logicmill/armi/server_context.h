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

#ifndef LOGICMILL_ARMI_SERVER_CONTEXT_H
#define LOGICMILL_ARMI_SERVER_CONTEXT_H

#include <logicmill/armi/server_context_base.h>
#include <logicmill/traits.h>

namespace logicmill
{
namespace armi
{
template<class Stub>
class server_context;

template<
		template<class...> class StubTemplate,
		class Target,
		template<class...> class ServerContextBaseTemplate,
		class SerializationTraits,
		class TransportTraits>
class server_context<StubTemplate<Target, ServerContextBaseTemplate<SerializationTraits, TransportTraits>>>
	: public ServerContextBaseTemplate<SerializationTraits, TransportTraits>
{
public:
	using base                     = ServerContextBaseTemplate<SerializationTraits, TransportTraits>;
	using stub_type                = StubTemplate<Target, base>;
	using target_type              = Target;
	using impl_ptr                 = std::shared_ptr<target_type>;
	using serialization_traits     = SerializationTraits;
	using deserializer_type        = typename serialization_traits::deserializer_type;
	using serializer_type          = typename serialization_traits::serializer_type;
	using transport_traits         = TransportTraits;
	using channel_type             = typename transport_traits::channel_type;
	using channel_param_type       = typename transport_traits::channel_param_type;
	using channel_const_param_type = typename transport_traits::channel_const_param_type;
	using ptr                      = util::shared_ptr<server_context>;

	server_context() : base{}, m_stub{this} {}

	void
	handle_request(channel_param_type channel, deserializer_type& request, impl_ptr const& impl)
	{
		m_stub.process(channel, request, impl);
	}

private:
	stub_type m_stub;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_SERVER_CONTEXT_H
