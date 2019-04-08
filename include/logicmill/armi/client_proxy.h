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

#ifndef LOGICMILL_ARMI_CLIENT_PROXY_H
#define LOGICMILL_ARMI_CLIENT_PROXY_H

#include <logicmill/armi/client_proxy_base.h>
#include <logicmill/traits.h>

namespace logicmill
{
namespace armi
{

template<class Proxy>
class client_proxy;

template<
		template<class...> class ProxyTemplate,
		class Target,
		template<class...> class ClientProxyBaseTemplate,
		class SerializationTraits,
		class AsyncIOTraits>
class client_proxy<ProxyTemplate<Target, ClientProxyBaseTemplate<SerializationTraits, AsyncIOTraits>>>
	: public ClientProxyBaseTemplate<SerializationTraits, AsyncIOTraits>
{
public:
	using base                 = ClientProxyBaseTemplate<SerializationTraits, AsyncIOTraits>;
	using serialization_traits = SerializationTraits;
	using async_io_traits     = AsyncIOTraits;
	using proxy_type           = ProxyTemplate<Target, base>;

	class target_reference
	{
	public:
		friend class client_proxy;

	protected:
		target_reference(client_proxy* client, channel_id_type channel) : m_client{client}, m_channel{channel} {}

	public:
		target_reference() : m_client{nullptr}, m_channel{null_channel} {}

		target_reference(target_reference const& other) : m_client{other.m_client}, m_channel{other.m_channel} {}

		target_reference&
		operator=(target_reference const& other)
		{
			m_client = other.m_client;
			m_channel = other.m_channel;
			return *this;
		}

		target_reference&
		timeout(std::chrono::milliseconds t)
		{
			m_client->set_transient_timeout(t);
		}

		const proxy_type* operator->() const
		{
			return m_client->proxy(m_channel);
		}

		bool
		is_valid()
		{
			return m_client->is_valid_channel(m_channel);
		}

		void
		close()
		{
			m_client->close(m_channel);
		}

		explicit operator bool() const
		{
			return is_valid();
		}

	private:
		client_proxy* m_client;
		channel_id_type m_channel;
	};

	target_reference
	create_target_reference(channel_id_type channel)
	{
		return target_reference(this, channel);
	}

	client_proxy() : base{}, m_proxy{this} {}

private:
	const proxy_type*
	proxy(channel_id_type channel) const
	{
		base::set_transient_channel(channel);
		return &m_proxy;
	}

	const proxy_type m_proxy;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_PROXY_H
