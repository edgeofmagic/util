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

#ifndef LOGICMILL_ARMI_METHOD_STUB_BASE_H
#define LOGICMILL_ARMI_METHOD_STUB_BASE_H

#include <cstdint>
#include <logicmill/armi/server_context_base.h>
#include <logicmill/armi/transport.h>
#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/ombstream.h>

namespace logicmill
{
namespace armi
{

template<class Target>
class member_func_stub_base
{
public:
	using target_ptr_type = std::shared_ptr<Target>;

	member_func_stub_base(server_context_base& context) : m_context{context} {}

	virtual ~member_func_stub_base() {}
	virtual void
	dispatch(request_id_type req_id, channel_id_type channel_id, bstream::ibstream& is, target_ptr_type const& target)
			const = 0;

protected:
	void
	request_failed(request_id_type request_id, channel_id_type channel_id, std::error_code err) const
	{
		bstream::ombstream os{m_context.stream_context()};
		os << request_id;
		os << reply_kind::fail;
		os.write_array_header(1);
		os << err;
		m_context.get_transport().send_reply(channel_id, os.release_mutable_buffer());
	}

	server_context_base&
	context() const
	{
		return m_context;
	}

private:
	server_context_base& m_context;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_METHOD_STUB_BASE_H
