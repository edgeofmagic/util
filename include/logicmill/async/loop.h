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

#ifndef LOGICMILL_ASYNC_LOOP_H
#define LOGICMILL_ASYNC_LOOP_H

#include <memory>
#include <functional>
#include <system_error>
#include <chrono>
#include <logicmill/async/timer.h>
#include <logicmill/async/resolver.h>

namespace logicmill
{
namespace async
{

class timer;

class loop
{
public:
	using ptr = std::shared_ptr< loop >;

	static loop::ptr
	create();

	static loop::ptr
	get_default();

	virtual
	~loop() {}

	virtual void
	run() = 0;

	virtual void 
	run( std::error_code& err ) = 0;

	virtual void
	stop() = 0;

	virtual void
	stop( std::error_code& err ) = 0;

	virtual void
	close() = 0;

	virtual void
	close( std::error_code& err ) = 0;

	virtual timer::ptr
	create_timer( timer::handler hf ) = 0;

	virtual timer::ptr
	create_timer( std::error_code& err, timer::handler hf ) = 0;

	virtual resolver::ptr
	create_resolver( std::string const& hostname, resolver::handler hf ) = 0;

	virtual resolver::ptr
	create_resolver( std::string const& hostname, std::error_code& err, resolver::handler hf ) = 0;
};

} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_LOOP_H