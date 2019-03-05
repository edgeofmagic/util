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

#include <logicmill/traits.h>

namespace logicmill
{
namespace armi
{
// template<class T>
// class client_context;

template<class Proxy, class StreamContext>
class client_context : public client_context_base 
{
public:
	using base = client_context_base;
	using proxy_type = Proxy;

	class client_channel
	{
	public:

		friend class client_context;

	protected:

		client_channel(client_context& context, channel_id_type id)
		: m_context{context}, m_id{id} {}

	public:

		client_channel(client_channel const& other)
		: m_context{other.m_context}, m_id{other.m_id} {}

		client_channel&
		operator=(client_channel const& other)
		{
			m_context = other.m_context;
			m_id = other.m_id;
		}		

		client_channel&
		timeout(std::chrono::milliseconds t)
		{
			m_context.set_transient_timeout(t);
		}

		proxy_type*
		operator->()
		{
			return m_context.proxy(m_id);
		}

		bool
		is_valid()
		{
			return m_context.is_valid_channel_id(m_id);
		}

		void
		close()
		{
			m_context.close(m_id);
		}
		
		explicit operator bool() const 
		{
			return is_valid(); 
		}

	private:

		client_context& m_context;
		channel_id_type m_id;
	};

	client_channel
	create_channel(channel_id_type id)
	{
		return client_channel(*this, id);
	}

	client_context(transport::client& transport_client)
		: base{transport_client, StreamContext::get()}, m_proxy{*this}
	{}

private:

	proxy_type*
	proxy(channel_id_type channel_id)
	{
		set_transient_channel_id(channel_id);
		return &m_proxy;
	}

	proxy_type m_proxy;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_CLIENT_CONTEXT_H
