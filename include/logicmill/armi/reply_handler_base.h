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
 * File:   reply_handler_base.h
 * Author: David Curtis
 *
 * Created on October 15, 2017, 12:57 PM
 */

#ifndef LOGICMILL_ARMI_REPLY_HANDLER_BASE_H
#define LOGICMILL_ARMI_REPLY_HANDLER_BASE_H

#include <memory>
#include <logicmill/bstream/ibstream.h>
#include <system_error>

namespace logicmill
{
namespace armi
{
	
	class reply_handler_base
	{
	public:
		virtual ~reply_handler_base() {}
		using ptr = std::unique_ptr<reply_handler_base>;
		virtual void handle_reply(bstream::ibstream& is) = 0;
		virtual void cancel(std::error_code ec) = 0;
	};
	
} // namespace armi
} // namespace logicmill


#endif /* LOGICMILL_ARMI_REPLY_HANDLER_BASE_H */

