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
	template<class SerializationTraits, class TransportTraits>                                                           \
	class REMOTE_CONTEXT                                                                                                 \
	{                                                                                                                    \
	public:                                                                                                              \
		using serialization_traits = SerializationTraits;                                                                \
		using transport_traits     = TransportTraits;                                                                    \
		template<class _Target, class _ClientContextBase>                                                                \
		class _proxy;                                                                                                    \
		template<class _Target, class _ServerContextBase>                                                                \
		class _stub;                                                                                                     \
		using target_type              = TARGET_TYPE;                                                                    \
		using client_context_base_type = logicmill::armi::client_context_base<serialization_traits, transport_traits>;   \
		using client_context_type      = logicmill::armi::client_context<_proxy<TARGET_TYPE, client_context_base_type>>; \
		using server_context_base_type = logicmill::armi::server_context_base<serialization_traits, transport_traits>;   \
		using server_context_type      = logicmill::armi::server_context<_stub<TARGET_TYPE, server_context_base_type>>;  \
	};                                                                                                                   \
	ARMI_PROXY_STUB_SPEC_(REMOTE_CONTEXT, TARGET_TYPE, MEMBER_FUNC_LIST)
/**/

#define ARMI_PROXY_STUB_SPEC_(REMOTE_CONTEXT, TARGET_TYPE, MEMBER_FUNC_LIST)                                           \
	template<class SerializationTraits, class TransportTraits>                                                         \
	template<template<class...> class ClientContextBaseTemplate, class _SerTraits, class _TransTraits>                 \
	class REMOTE_CONTEXT<SerializationTraits, TransportTraits>::                                                       \
			_proxy<TARGET_TYPE, ClientContextBaseTemplate<_SerTraits, _TransTraits>>                                   \
	{                                                                                                                  \
	public:                                                                                                            \
		using client_context_base_type = ClientContextBaseTemplate<_SerTraits, _TransTraits>;                          \
		using target_type              = TARGET_TYPE;                                                                  \
		_proxy(client_context_base_type* context_base) : m_context{context_base} {}                                    \
                                                                                                                       \
	private:                                                                                                           \
		client_context_base_type* m_context;                                                                           \
                                                                                                                       \
	public:                                                                                                            \
		BOOST_PP_SEQ_FOR_EACH_I(ARMI_DO_FUNCTION_PROXY_, TARGET_TYPE, MEMBER_FUNC_LIST)                                \
	};                                                                                                                 \
	template<class SerializationTraits, class TransportTraits>                                                         \
	template<template<class...> class ServerContextBaseTemplate, class _SerTraits, class _TransTraits>                 \
	class REMOTE_CONTEXT<SerializationTraits, TransportTraits>::                                                       \
			_stub<TARGET_TYPE, ServerContextBaseTemplate<_SerTraits, _TransTraits>>                                    \
		: public logicmill::armi::interface_stub<TARGET_TYPE, ServerContextBaseTemplate<_SerTraits, _TransTraits>>     \
	{                                                                                                                  \
	public:                                                                                                            \
		using server_context_base_type = ServerContextBaseTemplate<_SerTraits, _TransTraits>;                          \
		using target_type              = TARGET_TYPE;                                                                  \
		using base                                                                                                     \
				= logicmill::armi::interface_stub<TARGET_TYPE, ServerContextBaseTemplate<_SerTraits, _TransTraits>>;   \
		_stub(server_context_base_type* context_base)                                                                  \
			: base{context_base, BOOST_PP_SEQ_FOR_EACH_I(ARMI_DO_FUNCTION_PTR_, TARGET_TYPE, MEMBER_FUNC_LIST)}        \
		{}                                                                                                             \
	};
/**/

#define ARMI_DO_FUNCTION_PTR_(r, TARGET_TYPE, N, MEMBER_FUNC) BOOST_PP_COMMA_IF(N) & TARGET_TYPE::MEMBER_FUNC
/**/

#define ARMI_DO_FUNCTION_PROXY_(r, TARGET_TYPE, N, MEMBER_FUNC)                                                        \
	logicmill::armi::member_func_proxy<                                                                                \
			client_context_base_type,                                                                                  \
			logicmill::traits::remove_member_func_cv_noexcept<decltype(&TARGET_TYPE::MEMBER_FUNC)>::type>              \
			MEMBER_FUNC{m_context, N};
/**/

#endif    // LOGICMILL_ARMI_MACROS_H
