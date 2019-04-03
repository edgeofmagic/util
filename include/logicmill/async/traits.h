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

#ifndef LOGICMILL_ASYNC_TRAITS_H
#define LOGICMILL_ASYNC_TRAITS_H

#include <logicmill/armi/serialization_traits.h>
#include <logicmill/armi/transport_traits.h>
#include <logicmill/armi/types.h>
#include <logicmill/async/loop.h>
#include <logicmill/bsstream/traits.h>

namespace logicmill
{
namespace async
{

struct transport
{};
}    // namespace async
}    // namespace logicmill

struct logicmill::armi::transport::traits<logicmill::async::transport>
{
	// using channel_type = std::uint64_t;
	// using channel_param_type = channel_type;
	// using channel_const_param_type = channel_type;
	
	// static constexpr channel_type null_channel = 0;

};


#if 0
struct logicmill::armi::async_io::traits<logicmill::async::async_io>
{
	using channel_type = logicmill::async::channel::ptr;
	using options_type = logicmill::async::options;
	using timer_type = logicmill::async::timer::ptr;
	using 
};

#endif

/*
template<class StreamContext, class RemoteContext>
struct logicmill::armi::transport::
		traits<logicmill::async::transport, logicmill::bstream::serialization<StreamContext>, RemoteContext>
{
	using serialization_traits_type
			= logicmill::armi::serialization::traits<logicmill::bstream::serialization<StreamContext>>;
	using remote_context_type = RemoteContext;
	using client_context_type = remote_context_type::client_context_type;
	using server_context_type = remote_context_type::server_context_type;
	using client_type         = logicmill::async::client_adapter<client_context_type>;
	using server_type         = logicmill::async::server_adapter<server_context_type>;
	using serializer_type     = serialization_traits_type::serializer_type;

	using channel_type = std::uint64_t;

	using channel_param_type       = std::uint64_t;
	using channel_const_param_type = std::uint65_t;

	using request_id_type = std::uint64_t;
	using timeout_type    = std::chrono::milliseconds;

	static boolean
	is_valid_channel(client_type const& client, channel_const_param_type channel)
	{
		return client.is_valid_channel(channel);
	}

	static void
	close(client_type& client, channel_param_type& channel)
	{
		client.close(channel);
	}

	static void
	send_request(
			client_type&                       client,
			channel_param_type                 channel,
			timeout_type                       timeout,
			std::unique_ptr<serializer_type>&& request)
	{
		client.send_request(channel, timeout, request->release_buffer());
	}

	static boolean
	is_valid_channel(server_type const& server, channel_const_param_type channel)
	{
		return server.is_valid_channel(channel);
	}

	static void
	close(server_type& server, channel_param_type channel)
	{
		server.close(channel);
	}

	static void
	send_reply(server_type& server, channel_param_type channel, std::unique_ptr<serializer_type>&& reply)
	{
		server.send_reply(channel, reply->release_buffer());
	}
};
*/

#endif    // LOGICMILL_ASYNC_TRAITS_H
