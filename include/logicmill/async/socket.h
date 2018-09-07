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

#ifndef LOGICMILL_ASYNC_SOCKET_H
#define LOGICMILL_ASYNC_SOCKET_H

#include <memory>
#include <functional>
#include <system_error>
#include <chrono>

namespace logicmill
{
namespace async
{

class loop;

namespace tcp
{
	class socket
	{
	public:
		
	}
}

class timer
{
public:
	using ptr = std::shared_ptr< timer >;
	using handler = std::function< void ( timer::ptr ) >;

	virtual ~timer() {}

	virtual void
	start( std::chrono::milliseconds timeout ) = 0;

	virtual void
	start( std::chrono::milliseconds timeout, std::error_code& err ) = 0;

	virtual void
	stop() = 0;

	virtual void
	stop( std::error_code& err ) = 0;

	virtual void
	close() = 0;

	virtual std::shared_ptr< loop >
	owner() = 0;

};

} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_SOCKET_H