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
 * File:   interface_stub.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 9:12 PM
 */

#ifndef LOGICMILL_RAS_INTERFACE_STUB_H
#define LOGICMILL_RAS_INTERFACE_STUB_H

#include <logicmill/ras/interface_stub_builder.h>


namespace logicmill
{
namespace ras
{
	class interface_stub : public interface_stub_builder
	{
	public:
		template<class... Args>
		inline interface_stub(server_context_base& context, std::size_t index, Args... args)
		: interface_stub_builder(context, index, typename make_indices<sizeof...(Args)>::type(), args...)
		{}
	};		

} // namespace ras
} // namespace logicmill

#endif /* LOGICMILL_RAS_INTERFACE_STUB_H */

