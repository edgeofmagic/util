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

#ifndef LOGICMILL_ARMI_CLIENT_CONTEXT_H
#define LOGICMILL_ARMI_CLIENT_CONTEXT_H

#include <logicmill/armi/client_context_base.h>
#include <logicmill/traits.h>

namespace logicmill
{
namespace armi
{

template<class Proxy>
class client_context;

template<
		template<class...> class ProxyTemplate,
		class Target,
		template<class...> class ClientContextBaseTemplate,
		class SerializationTraits,
		class TransportTraits>
class client_context<ProxyTemplate<Target, ClientContextBaseTemplate<SerializationTraits, TransportTraits>>>
	: public ClientContextBaseTemplate<SerializationTraits, TransportTraits>
{
public:
	using base                     = ClientContextBaseTemplate<SerializationTraits, TransportTraits>;
	using serialization_traits     = SerializationTraits;
	using transport_traits         = TransportTraits;
	using channel_type             = typename transport_traits::channel_type;
	using channel_param_type       = typename transport_traits::channel_param_type;
	using channel_const_param_type = typename transport_traits::channel_const_param_type;
	using proxy_type               = ProxyTemplate<Target, base>;

	class target_reference
	{
	public:
		friend class client_context;

	protected:
		target_reference(client_context* context, channel_const_param_type channel)
			: m_context{context}, m_channel{channel}
		{}

	public:
		target_reference() : m_context{nullptr}, m_channel{transport_traits::null_channel} {}

		target_reference(target_reference const& other) : m_context{other.m_context}, m_channel{other.m_channel} {}

		target_reference&
		operator=(target_reference const& other)
		{
			m_context = other.m_context;
			m_channel = other.m_channel;
			return *this;
		}

		target_reference&
		timeout(std::chrono::milliseconds t)
		{
			m_context->set_transient_timeout(t);
		}

		const proxy_type* operator->() const
		{
			return m_context->proxy(m_channel);
		}

		bool
		is_valid()
		{
			return m_context->is_valid_channel(m_channel);
		}

		void
		close()
		{
			m_context->close(m_channel);
		}

		explicit operator bool() const
		{
			return is_valid();
		}

	private:
		client_context* m_context;
		channel_type    m_channel;
	};

	target_reference
	create_target_reference(channel_const_param_type channel)
	{
		return target_reference(this, channel);
	}

	client_context() : base{}, m_proxy{this} {}

private:
	const proxy_type*
	proxy(channel_const_param_type channel) const
	{
		base::set_transient_channel(channel);
		return &m_proxy;
	}

	const proxy_type m_proxy;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_CONTEXT_H
