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
#include <logicmill/armi/server_context_base.h>
#include <logicmill/armi/transport.h>
#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/ombstream.h>

namespace logicmill
{
namespace armi
{
class server_context_base;

template<class T>
class member_func_stub_base;

template<class Target>
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
	member_func_count() const noexcept
			= 0;

	virtual member_func_stub_base<Target>&
	get_member_func_stub(std::size_t index)
			= 0;

	void
	request_failed(request_id_type request_id, channel_id_type channel_id, std::error_code err)
	{
		bstream::ombstream os{m_context.stream_context()};
		os << request_id;
		os << reply_kind::fail;
		os.write_array_header(1);
		os << err;
		m_context.get_transport().send_reply(channel_id, os.release_mutable_buffer());
	}

	server_context_base& m_context;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_INTERFACE_STUB_BASE_H
