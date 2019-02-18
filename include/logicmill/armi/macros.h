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
#include <logicmill/util/preprocessor.h>

#define ARMI_CONTEXT(context_name, ...) ARMI_CONTEXT_(context_name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define ARMI_CONTEXT_(context_name, ifaces_seq)                                                                        \
	class context_name                                                                                                 \
	{                                                                                                                  \
	public:                                                                                                            \
		template<class T>                                                                                              \
		class _proxy;                                                                                                  \
		template<class T>                                                                                              \
		class _stub;                                                                                                   \
		using context_type = logicmill::armi::                                                                         \
				remote_context<_proxy<void>, _stub<void>, BOOST_PP_SEQ_FOR_EACH_I(ARMI_LIST_IFACE_, _, ifaces_seq)>;   \
		using client_context_type = context_type::client_context_type;                                                 \
		using server_context_type = context_type::server_context_type;                                                 \
		context_name(bstream::context_base::ptr const& stream_context = armi::get_default_stream_context())            \
			: m_context{/*cp, */ stream_context}                                                                       \
		{}                                                                                                             \
		SHARED_PTR_TYPE<client_context_type> create_client(/* transport::client_connection::ptr const& cp = nullptr */)            \
		{                                                                                                              \
			return m_context.create_client(/* cp */);                                                                  \
		}                                                                                                              \
		SHARED_PTR_TYPE<server_context_type> create_server(/* transport::server_connection::ptr const& cp = nullptr */)            \
		{                                                                                                              \
			return m_context.create_server(/* cp */);                                                                  \
		}                                                                                                              \
		SHARED_PTR_TYPE<client_context_type> const& client(/* transport::client_connection::ptr const& cp = nullptr */)            \
		{                                                                                                              \
			return m_context.client(/* cp */);                                                                         \
		}                                                                                                              \
		SHARED_PTR_TYPE<server_context_type> const& server(/* transport::server_connection::ptr const& cp = nullptr */)            \
		{                                                                                                              \
			return m_context.server(/* cp */);                                                                         \
		}                                                                                                              \
	private:                                                                                                           \
		context_type m_context;                                                                                        \
	};
/**/

#define ARMI_INTERFACES(...) ARMI_INTERFACES_(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define ARMI_INTERFACES_(ifaces_seq)                                                                                   \
	template<class T>                                                                                                  \
	class _proxy;                                                                                                      \
	template<class T>                                                                                                  \
	class _stub;                                                                                                       \
	using context_type = logicmill::armi::                                                                             \
			remote_context<_proxy<void>, _stub<void>, BOOST_PP_SEQ_FOR_EACH_I(ARMI_LIST_IFACE_, _, ifaces_seq)>;
/**/

#define ARMI_LIST_IFACE_(r, data, n, iface) BOOST_PP_COMMA_IF(n) iface
/**/

#define ARMI_INTERFACE(context_name, iface, ...)                                                                       \
	ARMI_INTERFACE_(context_name, iface, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                        \
	/**/

#define ARMI_INTERFACE_(context_name, iface, methods_seq)                                                              \
	template<>                                                                                                         \
	class context_name::_proxy<iface> : public logicmill::armi::interface_proxy                                        \
	{                                                                                                                  \
	public:                                                                                                            \
		using target_type = iface;                                                                                     \
		_proxy(logicmill::armi::client_context_base& context, std::size_t index)                            \
			: logicmill::armi::interface_proxy{context, index}                                                         \
		{}                                                                                                             \
		_proxy&                                                                                                        \
		timeout(std::chrono::milliseconds t)                                                                           \
		{                                                                                                              \
			transient_timeout(t);                                                                                      \
			return *this;                                                                                              \
		}                                                                                                              \
		BOOST_PP_SEQ_FOR_EACH_I(ARMI_DO_METHOD_PROXY_, iface, methods_seq)                                             \
	};                                                                                                                 \
	template<>                                                                                                         \
	class context_name::_stub<iface> : public logicmill::armi::interface_stub                                          \
	{                                                                                                                  \
	public:                                                                                                            \
		using target_type = iface;                                                                                     \
		_stub(logicmill::armi::server_context_base& context, std::size_t index)                                        \
			: logicmill::armi::interface_stub(                                                                         \
					  context,                                                                                         \
					  index,                                                                                           \
					  BOOST_PP_SEQ_FOR_EACH_I(ARMI_DO_METHOD_LIST_, iface, methods_seq))                               \
		{}                                                                                                             \
	};                                                                                                                 \
	/**/

#define ARMI_DO_METHOD_LIST_(r, iface, n, method) BOOST_PP_COMMA_IF(n) & iface ::method
/**/

#define ARMI_DO_METHOD_PROXY_(r, iface, n, method)                                                                     \
	logicmill::armi::method_proxy<decltype(&iface ::method)> method{context(), index(), n};
/**/

#endif    // LOGICMILL_ARMI_MACROS_H
