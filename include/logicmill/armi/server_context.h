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

#ifndef LOGICMILL_ARMI_SERVER_CONTEXT_H
#define LOGICMILL_ARMI_SERVER_CONTEXT_H

// #include <logicmill/armi/server_context_builder.h>
#include <logicmill/traits.h>

namespace logicmill
{
namespace armi
{
template<class T, class StreamContext>
class server_context;

template<template<class...> class Stub, class U, class StreamContext>
class server_context<Stub<U>, StreamContext> : public logicmill::armi::server_context_base
{
public:
	using base                = logicmill::armi::server_context_base;
	using stub_type   = Stub<U>;
	using target_type = U;
	using impl_ptr = std::shared_ptr<target_type>;

	using ptr = SHARED_PTR_TYPE<server_context>;

	server_context(transport::server& transport_server) : base{transport_server, StreamContext::get()}, m_stub{*this} {}

	// void
	// register_impl(std::shared_ptr<target_type> const& impl_ptr)
	// {
	// 	m_impl = impl_ptr;
	// }

	// std::shared_ptr<target_type>
	// get_impl()
	// {
	// 	return std::static_pointer_cast<target_type>(m_impl);
	// }

	// virtual interface_stub_base&
	// get_type_erased_stub() override
	// {
	// 	return m_stub;
	// }

	// virtual std::shared_ptr<void> const&
	// get_type_erased_impl() override
	// {
	// 	return m_impl;
	// }

	void
	handle_request(channel_id_type channel_id, bstream::ibstream& is, impl_ptr const& impl)
	{
		// request_id_type        request_id  = is.read_as<request_id_type>();
		// std::size_t if_index = is.read_as<std::size_t>();
			// auto& stub = get_type_erased_stub();
			m_stub.process(channel_id, is, impl);
	}


private:
	// std::shared_ptr<void> m_impl;
	stub_type	m_stub;
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_SERVER_CONTEXT_H
