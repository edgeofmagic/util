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

#define ARMI_CONTEXT(context_name, target_class, stream_context, ...)                                                  \
	ARMI_CONTEXT_(context_name, target_class, stream_context, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define ARMI_CONTEXT_(context_name, target_class, stream_context, member_func_list)                                    \
	class context_name                                                                                                 \
	{                                                                                                                  \
	public:                                                                                                            \
		template<class T>                                                                                              \
		class _proxy;                                                                                                  \
		template<class T>                                                                                              \
		class _stub;                                                                                                   \
		using target_interface    = target_class;                                                                      \
		using client_context_type = logicmill::armi::client_context<_proxy<target_class>, stream_context>;             \
		using server_context_type = logicmill::armi::server_context<_stub<target_class>, stream_context>;              \
	};                                                                                                                 \
	ARMI_PROXY_STUB_SPEC_(context_name, target_class, member_func_list)
/**/

#define ARMI_PROXY_STUB_SPEC_(context_name, target_class, member_func_list)                                            \
	template<>                                                                                                         \
	class context_name::_proxy<target_class>                                                                           \
	{                                                                                                                  \
	private:                                                                                                           \
		context_name::client_context_type& m_context;                                                                  \
                                                                                                                       \
	public:                                                                                                            \
		using target_type = target_class;                                                                              \
		_proxy(context_name::client_context_type& context) : m_context{context} {}                                     \
		BOOST_PP_SEQ_FOR_EACH_I(ARMI_DO_FUNCTION_PROXY_, target_class, member_func_list)                               \
	};                                                                                                                 \
	template<>                                                                                                         \
	class context_name::_stub<target_class> : public logicmill::armi::interface_stub<target_class>                     \
	{                                                                                                                  \
	public:                                                                                                            \
		using target_type = target_class;                                                                              \
		_stub(logicmill::armi::server_context_base& context)                                                           \
			: logicmill::armi::interface_stub<target_type>(                                                            \
					  context,                                                                                         \
					  BOOST_PP_SEQ_FOR_EACH_I(ARMI_DO_FUNCTION_PTR_, target_class, member_func_list))                  \
		{}                                                                                                             \
	};
/**/

#define ARMI_DO_FUNCTION_PTR_(r, target_class, n, member_func) BOOST_PP_COMMA_IF(n) & target_class ::member_func
/**/

#define ARMI_DO_FUNCTION_PROXY_(r, target_class, n, member_func)                                                       \
	logicmill::armi::member_func_proxy<                                                                                \
			logicmill::traits::remove_member_func_cv_noexcept<decltype(&target_class ::member_func)>::type>            \
			member_func{m_context, n};
/**/

#endif    // LOGICMILL_ARMI_MACROS_H
