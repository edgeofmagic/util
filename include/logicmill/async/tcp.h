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

#ifndef LOGICMILL_ASYNC_TCP_H
#define LOGICMILL_ASYNC_TCP_H

#include <memory>
#include <functional>
#include <system_error>
#include <chrono>
#include <deque>
#include <logicmill/bstream/buffer.h>

namespace logicmill
{
namespace async
{

class loop;

namespace tcp
{

class channel
{
public:

	using ptr = std::shared_ptr< channel >;
	using read_handler = std::function< void ( channel::ptr const& chan, bstream::const_buffer&& buf, std::error_code const& err ) >;
	using write_handler = std::function< void ( channel::ptr const& chan, std::deque< bstream::mutable_buffer >&& bufs, std::error_code const& err ) >;
	using connect_handler = std::function< void ( channel::ptr const& chan, std::error_code const& err ) >;

	virtual ~channel() {}

	virtual void
	start_read( read_handler&& handler ) = 0;

	virtual void
	start_read( read_handler const& handler ) = 0;

	virtual void
	stop_read() = 0;

	virtual void
	write( bstream::mutable_buffer&& buf, write_handler&& handler ) = 0;

	virtual void
	write( bstream::mutable_buffer&& buf, write_handler const& handler ) = 0;

	virtual void
	write( std::deque< bstream::mutable_buffer >&& bufs, write_handler&& handler ) = 0;

	virtual void
	write( std::deque< bstream::mutable_buffer >&& bufs, write_handler const& handler ) = 0;

	virtual void
	close() = 0;

};

class listener
{
public:

	using ptr = std::shared_ptr< listener >;
	using connection_handler = std::function< void ( listener::ptr const& sp, tcp::channel::ptr const& chan, std::error_code const& err ) >;

	virtual void
	close() = 0;
	
};

} // namespace tcp
} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_TCP_H