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
 * File:   fail_proxy.h
 * Author: David Curtis
 *
 * Created on January 4, 2018, 7:59 PM
 */

#ifndef LOGICMILL_ARMI_FAIL_PROXY_H
#define LOGICMILL_ARMI_FAIL_PROXY_H

#include <cstdint>
#include <logicmill/async/channel.h>
#include <system_error>

namespace logicmill
{
namespace armi
{
class server_context_base;

class fail_proxy
{
public:
	fail_proxy(server_context_base& context, std::uint64_t req_ord, logicmill::async::channel::ptr const& chan)
		: m_context{context}, m_req_ord{req_ord}, m_channel{chan}
	{}

	inline fail_proxy(fail_proxy const& other) : m_context{other.m_context}, m_req_ord{other.m_req_ord}, m_channel{other.m_channel} {}

	void
	operator()(std::error_code ec);

private:
	server_context_base& m_context;
	std::uint64_t        m_req_ord;
	async::channel::ptr  m_channel;
};

}    // namespace armi
}    // namespace logicmill

#endif /* LOGICMILL_ARMI_FAIL_PROXY_H */
