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

#ifndef LOGICMILL_ARMI_MACROS_H
#define LOGICMILL_ARMI_MACROS_H

#ifndef BOOST_PP_VARIADICS
#define BOOST_PP_VARIADICS 1
#endif

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/list/for_each_i.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <logicmill/traits.h>
#include <logicmill/util/preprocessor.h>
#include <type_traits>

#define ARMI_CONTEXT(REMOTE_CONTEXT, TARGET_TYPE, ...)                                                                 \
	ARMI_CONTEXT_(REMOTE_CONTEXT, TARGET_TYPE, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define ARMI_CONTEXT_(REMOTE_CONTEXT, TARGET_TYPE, MEMBER_FUNC_LIST)                                                     \
	template<class SerializationTraits, class AsyncIOTraits>                                                           \
	class REMOTE_CONTEXT                                                                                                 \
	{                                                                                                                    \
	public:                                                                                                              \
		using serialization_traits = SerializationTraits;                                                                \
		using async_io_traits     = AsyncIOTraits;                                                                    \
		template<class _Target, class _ClientProxyBase>                                                                \
		class _proxy;                                                                                                    \
		template<class _Target, class _ServerStubBase>                                                                \
		class _stub;                                                                                                     \
		using target_type              = TARGET_TYPE;                                                                    \
		using client_proxy_base_type = logicmill::armi::client_proxy_base<serialization_traits, async_io_traits>;   \
		using client_proxy_type      = logicmill::armi::client_proxy<_proxy<TARGET_TYPE, client_proxy_base_type>>; \
		using server_stub_base_type = logicmill::armi::server_stub_base<serialization_traits, async_io_traits>;   \
		using server_stub_type      = logicmill::armi::server_stub<_stub<TARGET_TYPE, server_stub_base_type>>;  \
	};                                                                                                                   \
	ARMI_PROXY_STUB_SPEC_(REMOTE_CONTEXT, TARGET_TYPE, MEMBER_FUNC_LIST)
/**/

#define ARMI_PROXY_STUB_SPEC_(REMOTE_CONTEXT, TARGET_TYPE, MEMBER_FUNC_LIST)                                           \
	template<class SerializationTraits, class AsyncIOTraits>                                                         \
	template<template<class...> class ClientProxyBaseTemplate, class _SerTraits, class _TransTraits>                 \
	class REMOTE_CONTEXT<SerializationTraits, AsyncIOTraits>::                                                       \
			_proxy<TARGET_TYPE, ClientProxyBaseTemplate<_SerTraits, _TransTraits>>                                   \
	{                                                                                                                  \
	public:                                                                                                            \
		using client_proxy_base_type = ClientProxyBaseTemplate<_SerTraits, _TransTraits>;                          \
		using target_type              = TARGET_TYPE;                                                                  \
		_proxy(client_proxy_base_type* client) : m_client{client} {}                                    \
                                                                                                                       \
	private:                                                                                                           \
		client_proxy_base_type* m_client;                                                                           \
                                                                                                                       \
	public:                                                                                                            \
		BOOST_PP_SEQ_FOR_EACH_I(ARMI_DO_FUNCTION_PROXY_, TARGET_TYPE, MEMBER_FUNC_LIST)                                \
	};                                                                                                                 \
	template<class SerializationTraits, class AsyncIOTraits>                                                         \
	template<template<class...> class ServerStubBaseTemplate, class _SerTraits, class _TransTraits>                 \
	class REMOTE_CONTEXT<SerializationTraits, AsyncIOTraits>::                                                       \
			_stub<TARGET_TYPE, ServerStubBaseTemplate<_SerTraits, _TransTraits>>                                    \
		: public logicmill::armi::interface_stub<TARGET_TYPE, ServerStubBaseTemplate<_SerTraits, _TransTraits>>     \
	{                                                                                                                  \
	public:                                                                                                            \
		using server_stub_base_type = ServerStubBaseTemplate<_SerTraits, _TransTraits>;                          \
		using target_type              = TARGET_TYPE;                                                                  \
		using base                                                                                                     \
				= logicmill::armi::interface_stub<TARGET_TYPE, ServerStubBaseTemplate<_SerTraits, _TransTraits>>;   \
		_stub(server_stub_base_type* server)                                                                  \
			: base{server, BOOST_PP_SEQ_FOR_EACH_I(ARMI_DO_FUNCTION_PTR_, TARGET_TYPE, MEMBER_FUNC_LIST)}        \
		{}                                                                                                             \
	};
/**/

#define ARMI_DO_FUNCTION_PTR_(r, TARGET_TYPE, N, MEMBER_FUNC) BOOST_PP_COMMA_IF(N) & TARGET_TYPE::MEMBER_FUNC
/**/

#define ARMI_DO_FUNCTION_PROXY_(r, TARGET_TYPE, N, MEMBER_FUNC)                                                        \
	logicmill::armi::member_func_proxy<                                                                                \
			client_proxy_base_type,                                                                                  \
			logicmill::traits::remove_member_func_cv_noexcept<decltype(&TARGET_TYPE::MEMBER_FUNC)>::type>              \
			MEMBER_FUNC{m_client, N};
/**/

#endif    // LOGICMILL_ARMI_MACROS_H
