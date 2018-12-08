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

#include <logicmill/armi/interface_stub_base.h>
#include <logicmill/armi/method_stub_base.h>
#include <logicmill/armi/fail_proxy.h>
#include <logicmill/armi/error.h>


using namespace logicmill;
using namespace armi;

void 
interface_stub_base::process(std::uint64_t req_ord, async::channel::ptr const& chan, bstream::ibstream& is)
{
	auto method_id = is.read_as<std::size_t>();
	if (method_id >= method_count())
	{
		fail_proxy{m_context, req_ord, chan}(make_error_code(armi::errc::invalid_method_id));
	}
	else
	{
		get_method_stub(method_id).dispatch(req_ord, chan, is);
	}
}
