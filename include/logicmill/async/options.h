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

#ifndef LOGICMILL_ASYNC_OPTIONS_H
#define LOGICMILL_ASYNC_OPTIONS_H

#include <logicmill/async/endpoint.h>
#include <memory>

namespace logicmill
{
namespace async
{

class options
{
public:
	options(ip::endpoint const& ep)
	:
	m_endpoint{ep},
	m_framing{false}
	{}

	static options
	create(ip::endpoint const& ep)
	{
		return options{ep};
	}

	ip::endpoint const& 
	endpoint() const
	{
		return m_endpoint;
	}

	options&
	framing(bool value)
	{
		m_framing = value;
		return *this;
	}

	bool
	framing() const
	{
		return m_framing;
	}

private:
	ip::endpoint m_endpoint;
	bool m_framing;
};

}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ASYNC_OPTIONS_H