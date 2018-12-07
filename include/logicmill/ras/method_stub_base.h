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
 * File:   method_stub_base.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 10:46 PM
 */

#ifndef LOGICMILL_RAS_METHOD_STUB_BASE_H
#define LOGICMILL_RAS_METHOD_STUB_BASE_H

#include <cstdint>
#include <logicmill/bstream/ibstream.h>
#include <logicmill/async/channel.h>

namespace logicmill
{
namespace ras
{
	class method_stub_base
	{
	public:
		virtual ~method_stub_base() {}
		virtual void dispatch(std::uint64_t req_ord, async::channel::ptr const& chan, bstream::ibstream& is) const = 0;
	};
} // namespace ras
} // namespace logicmill

#endif /* LOGICMILL_RAS_METHOD_STUB_BASE_H */

