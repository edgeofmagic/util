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
 * File:   remote_context.h
 * Author: David Curtis
 *
 * Created on December 2, 2017, 8:14 AM
 */

#ifndef LOGICMILL_RAS_REMOTE_CONTEXT_H
#define LOGICMILL_RAS_REMOTE_CONTEXT_H

#include <logicmill/bstream/context.h>
#include <logicmill/ras/client_context.h>
#include <logicmill/ras/error.h>
#include <logicmill/ras/server_context.h>

namespace logicmill
{
namespace ras
{
namespace remote_context
{
template<class Proxy, class Stub, class... Interfaces>
class context
{
public:
	using proxys              = typename traits::apply_args<Proxy, Interfaces...>::type;
	using stubs               = typename traits::apply_args<Stub, Interfaces...>::type;
	using client_context_type = logicmill::ras::client_context<proxys>;
	using server_context_type = logicmill::ras::server_context<stubs>;

	context(async::loop::ptr lp = async::loop::get_default(), bstream::context_base const& stream_context = ras::get_default_stream_context())
	: m_loop{lp}, m_stream_context{stream_context}
	{}

	void
	loop(async::loop::ptr lp)
	{
		m_loop = lp;
	}

	void
	stream_context(bstream::context_base const& stream_context)
	{
		m_stream_context = stream_context;
	}

	async::loop::ptr
	loop() const
	{
		return m_loop;
	}

	bstream::context_base const&
	stream_context() const
	{
		return m_stream_context;
	}

	client_context_type&
	client()
	{
		if (!m_client_context)
		{
			m_client_context = std::make_unique<client_context_type>(m_loop, m_stream_context);
		}
		return *m_client_context;
	}

	server_context_type&
	server()
	{
		if (!m_server_context)
		{
			m_server_context = std::make_unique<server_context_type>(m_loop, m_stream_context);
		}
		return *m_server_context;
	}


private:
	async::loop::ptr                     m_loop;
	bstream::cloned_context              m_stream_context;
	std::unique_ptr<client_context_type> m_client_context;
	std::unique_ptr<server_context_type> m_server_context;
};

}    // namespace remote_context
}    // namespace ras
}    // namespace logicmill


#endif /* LOGICMILL_RAS_REMOTE_CONTEXT_H */
