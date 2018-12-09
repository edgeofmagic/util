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
 * furnished to do so, subjerrt to the following conditions:
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

#include <logicmill/armi/fail_proxy.h>
// #include <logicmill/armi/error_code_adaptor.h>
#include <logicmill/bstream/ombstream.h>
#include <logicmill/armi/server_context_base.h>
#include <logicmill/armi/types.h>

using namespace logicmill;
using namespace armi;

void
fail_proxy::operator()(std::error_code err)
{
	bstream::ombstream os{m_context.stream_context()};
	// auto os = m_context.create_reply_stream();
	os << m_req_ord;
	os << reply_kind::fail;
	os.write_array_header(1);
	os << err;
	// error_code_adaptor::put(os, m_context, err);
	m_context.send_reply(m_channel, os.release_mutable_buffer());
}
