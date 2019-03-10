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

#ifndef LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H
#define LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H

#include <cstdint>
#include <functional>
#include <logicmill/armi/transport.h>
#include <logicmill/armi/types.h>
#include <logicmill/bstream/ombstream.h>
#include <memory>
#include <unordered_set>

namespace logicmill
{
namespace armi
{

class server_context_base
{
public:
	template<class _T, class _U, class _V>
	friend class member_func_stub;

	server_context_base(transport::server& transport_server, bstream::context_base const& stream_context)
		: m_transport{transport_server}, m_stream_context{stream_context}
	{}

	virtual ~server_context_base() {}

	bstream::context_base const&
	stream_context()
	{
		return m_stream_context;
	}

	transport::server&
	get_transport() const
	{
		return m_transport;
	}

protected:
	transport::server&         m_transport;
	bstream::context_base const& m_stream_context;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_SERVER_CONTEXT_BASE_H
