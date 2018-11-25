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

#ifndef LOGICMILL_ASYNC_RESOLVE_REQUEST_UV_H
#define LOGICMILL_ASYNC_RESOLVE_REQUEST_UV_H

#include <uv.h>
#include <logicmill/async/resolve_request.h>
#include "uv_error.h"

class resolve_request_uv;

struct resolve_request_data
{
	std::shared_ptr< resolve_request_uv >		m_impl_ptr;
};


class resolve_request_uv : public logicmill::async::resolve_request, public std::enable_shared_from_this< resolve_request_uv >
{
public:

	using ptr = std::shared_ptr< resolve_request_uv >;

	resolve_request_uv( std::string const& hostname, logicmill::async::resolve_request::handler handler );

	void 
	start( uv_loop_t* lp, ptr const& self, std::error_code& err );

	virtual ~resolve_request_uv();

	static void 
	on_resolve( uv_getaddrinfo_t* req, int status, struct addrinfo* result );

	void
	wipe();

	virtual std::shared_ptr< logicmill::async::loop >
	owner() override;

	virtual void
	cancel() override;

	virtual std::string const&
	hostname() const override;

	uv_getaddrinfo_t									m_uv_req;
	resolve_request_data								m_data;
	std::string											m_hostname;
	logicmill::async::resolve_request::handler			m_handler;

};

#endif /* LOGICMILL_ASYNC_RESOLVE_REQUEST_UV_H */