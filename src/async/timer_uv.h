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

#ifndef LOGICMILL_ASYNC_TIMER_UV_H
#define LOGICMILL_ASYNC_TIMER_UV_H

#include <uv.h>
#include <logicmill/async/timer.h>
#include "uv_error.h"

class timer_uv;

struct timer_handle_data
{
	std::shared_ptr< timer_uv > m_impl_ptr;
};

class timer_uv : public logicmill::async::timer // , public std::enable_shared_from_this< timer_uv >
{
public:

	using ptr = std::shared_ptr< timer_uv >;

	timer_uv( uv_loop_t* lp, logicmill::async::timer::handler handler );

	timer_uv( uv_loop_t* lp, std::error_code& err, logicmill::async::timer::handler handler );

	virtual ~timer_uv();

	void
	init( ptr const& self );

	virtual void
	start( std::chrono::milliseconds timeout, std::error_code& err ) override;

	virtual void
	stop( std::error_code& err ) override;

	virtual void
	close() override;

	virtual std::shared_ptr< logicmill::async::loop >
	owner() override;

	bool 
	is_active() const;

	virtual bool
	is_pending() const override;

	void
	clear();

	static void
	on_timer_close( uv_handle_t* handle );

	static void
	on_timer_expire( uv_timer_t* handle );

	uv_timer_t									m_uv_timer;
	timer_handle_data							m_data;
	logicmill::async::timer::handler			m_handler;

};

#endif /* LOGICMILL_ASYNC_TIMER_UV_H */