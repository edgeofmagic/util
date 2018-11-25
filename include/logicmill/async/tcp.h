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

class write_request
{
public:
	using ptr = std::shared_ptr< write_request >;

	virtual ~write_request() {}

	virtual std::deque< bstream::mutable_buffer > const& 
	buffers() const = 0;

	virtual std::deque< bstream::mutable_buffer >
	release() = 0;

};

class channel
{
public:

	using ptr = std::shared_ptr< channel >;
	using read_handler = std::function< void ( channel::ptr const& chan, bstream::const_buffer&& buf, std::error_code const& err ) >;
	using write_handler = std::function< void ( channel::ptr const& chan, write_request::ptr const& req, std::error_code const& err ) >;
	using connect_handler = std::function< void ( channel::ptr const& chan, std::error_code const& err ) >;

	virtual ~channel() {}

	virtual void
	start_read( read_handler&& handler ) = 0;

	virtual void
	start_read( read_handler const& handler ) = 0;

	virtual void
	stop_read() = 0;

	virtual write_request::ptr
	write( bstream::mutable_buffer&& buf, write_handler&& handler ) = 0;

	virtual write_request::ptr
	write( bstream::mutable_buffer&& buf, write_handler const& handler ) = 0;

	virtual write_request::ptr
	write( std::deque< bstream::mutable_buffer >&& bufs, write_handler&& handler ) = 0;

	virtual write_request::ptr
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

}

#if 0
namespace tcp
{

	using payload_type = std::unique_ptr< std::deque< mutable_buffer > >;

	class socket : public duplex< payload_type, socket >
	{
	public:
		using data_event_type = data_event< payload_type >;

		template< class P, class R >
		void
		on( data_event_type, P&& pload, id_type id, R&& rcpt )
		{

		}

		void
		on( cancel_event, id_type id )
		{

		}

	private:
		uv_stream_t*	m_socket;
	};

	class server
	{
		void bind( std::string const& addr_str, std::uint16_t port, std::error_code& err )
		{

		}
	};

	class socket : public duplex< payload_type, server >
	{
	public:

		void bind( ip::endpoint const& ep, std::error_code& err )
		{


		}
	private:

		uv_tcp_t	m_socket;

	};

	class socket
	{
	public:
		using ptr = std::shared_ptr< socket >;

		using connect_handler = std::function< void ( socket::ptr connection, std::error_code& err ) >;
		using read_handler = std::function< void ( )

		using bind_handler
		virtual ~socket() {}

		virtual void
		bind( ip::endpoint const& ep, std::error_code& err, connect_handler handler ) = 0;

		virtual void
		close( std::error_code& err ) = 0;

		virtual void

		
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

} // namespace tcp

#endif

} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_TCP_H