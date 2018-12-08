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

/* 
 * File:   server_context.h
 * Author: David Curtis
 *
 * Created on October 15, 2017, 4:25 PM
 */

#ifndef LOGICMILL_RAS_SERVER_CONTEXT_H
#define LOGICMILL_RAS_SERVER_CONTEXT_H

#include <logicmill/ras/server_context_builder.h>
#include <logicmill/traits.h>

namespace logicmill
{
namespace ras
{	
	template<class T>
	class server_context;

	template<template <class...> class Template, class... Args>
	class server_context< Template<Args...> > : public logicmill::ras::server_context_builder<Args...>
	{
	public:
		
		using base = logicmill::ras::server_context_builder<Args...>;
		using stub_list_carrier = Template<Args...>;
		using target_list_carrier = traits::_arg_list<typename Args::target_type...>;
		
		template<class T>
		using stub_of = typename traits::replace_arg<typename traits::first_arg<stub_list_carrier>::type, T>::type;

		inline server_context(async::loop::ptr const& lp = async::loop::get_default(), bstream::context_base const& stream_context = ras::get_default_stream_context()) 
		: base{lp, stream_context}
		{}

		template<class T>
		inline void 
		register_impl(std::shared_ptr<T> impl_ptr)
		{
			logicmill::ras::server_context_base::set_impl(traits::index_from<T, target_list_carrier>::value, impl_ptr);
		}
	};	
} // namespace ras
} // namespace logicmill

#endif /* LOGICMILL_RAS_SERVER_CONTEXT_H */

