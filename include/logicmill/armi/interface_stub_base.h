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

#ifndef LOGICMILL_ARMI_INTERFACE_STUB_BASE_H
#define LOGICMILL_ARMI_INTERFACE_STUB_BASE_H

#include <cstdint>
// #include <logicmill/async/channel.h>
#include <logicmill/armi/transport.h>
#include <logicmill/bstream/ibstream.h>

namespace logicmill
{
namespace armi
{
class server_context_base;
class method_stub_base;

class interface_stub_base
{
public:
	virtual ~interface_stub_base() {}

protected:
	friend class server_context_base;

	interface_stub_base(server_context_base& context) : m_context{context} {}

	server_context_base&
	context()
	{
		return m_context;
	}

	virtual std::size_t
	method_count() const noexcept
			= 0;

	virtual method_stub_base&
	get_method_stub(std::size_t index)
			= 0;

	void
	process(std::uint64_t req_id, logicmill::armi::transport::server_channel::ptr const& chan, bstream::ibstream& is);

	static void
	request_failed(
			std::uint64_t                         request_id,
			transport::server_channel::ptr const& transp,
			bstream::context_base::ptr const&     stream_context,
			std::error_code                       err);

	server_context_base& m_context;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_INTERFACE_STUB_BASE_H
