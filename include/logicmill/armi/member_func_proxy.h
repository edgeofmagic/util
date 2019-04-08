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

#ifndef LOGICMILL_ARMI_METHOD_PROXY_H
#define LOGICMILL_ARMI_METHOD_PROXY_H

#include <logicmill/armi/adapters/bridge.h>
#include <logicmill/armi/client_proxy_base.h>
#include <logicmill/armi/reply_handler.h>
#include <logicmill/armi/types.h>
#include <logicmill/bstream/ombstream.h>
#include <logicmill/util/promise.h>
#include <type_traits>

namespace logicmill
{
namespace armi
{

template<class ClientProxyBase, class StrippedMemberFuncPtr>
class member_func_proxy;

template<
		template<class...> class ClientProxyBaseTemplate,
		class SerializationTraits,
		class AsyncIOTraits,
		class Target,
		class PromiseType,
		class... Args>
struct member_func_proxy<
		ClientProxyBaseTemplate<SerializationTraits, AsyncIOTraits>,
		util::promise<PromiseType> (Target::*)(Args...)>
{
public:
	using client_proxy_base_type = ClientProxyBaseTemplate<SerializationTraits, AsyncIOTraits>;
	using serialization_traits     = SerializationTraits;
	using async_io_traits         = AsyncIOTraits;
	using bridge_type              = logicmill::armi::adapters::bridge<serialization_traits, async_io_traits>;
	using reply_handler_type       = reply_handler<bridge_type, util::promise<PromiseType>>;
	using serializer_param_type    = typename bridge_type::serializer_param_type;


	member_func_proxy(client_proxy_base_type* client, std::size_t member_func_id)
		: m_client{client}, m_member_func_id{member_func_id}
	{}

	util::promise<PromiseType>
	operator()(Args... args) const
	{
		auto                       timeout = get_timeout();
		util::promise<PromiseType> p;
		auto                       request_id = m_client->next_request_id();
		m_client->add_handler(request_id, std::make_unique<reply_handler_type>(p));

		bridge_type::new_serializer([=](typename bridge_type::serializer_param_type request) {
			serialization_traits::write(request, request_id);
			serialization_traits::write(request, m_member_func_id);
			serialization_traits::write_sequence_prefix(request, sizeof...(Args));

			append(request, args...);

			m_client->send_request(request_id, timeout, request);
		});
		return p;
	}

	std::chrono::milliseconds
	get_timeout() const
	{
		millisecs timeout{m_client->get_and_clear_transient_timeout()};
		if (timeout.count() <= 0)
		{
			timeout = m_client->get_default_timeout();
		}
		return timeout;
	}

private:
	void
	append(serializer_param_type request) const
	{}

	template<class First_, class... More_>
	void
	append(serializer_param_type request, First_ first, More_... more) const
	{
		serialization_traits::write(request, first);
		append(request, more...);
	}

	client_proxy_base_type* m_client;
	std::size_t               m_member_func_id;
};

template<class ClientProxyBase, class MemberFuncPtr>
class stripped_member_func_proxy
	: public member_func_proxy<ClientProxyBase, typename traits::remove_member_func_cv_noexcept<MemberFuncPtr>::type>
{
public:
	using client_proxy_base_type = ClientProxyBase;
	using base                     = member_func_proxy<
            client_proxy_base_type,
            typename traits::remove_member_func_cv_noexcept<MemberFuncPtr>::type>;
	stripped_member_func_proxy(client_proxy_base_type* client, std::size_t member_func_id)
		: base{client, member_func_id}
	{}
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_METHOD_PROXY_H
