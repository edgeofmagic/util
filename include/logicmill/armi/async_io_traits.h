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

#ifndef LOGICMILL_ARMI_ASYNC_IO_TRAITS_H
#define LOGICMILL_ARMI_ASYNC_IO_TRAITS_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <logicmill/armi/serialization_traits.h>
#include <logicmill/armi/types.h>
#include <system_error>

namespace logicmill
{
namespace armi
{
namespace async_io
{

template<class AsyncIO>
struct traits
{
	/*
	using channel_type             = ...; // Type that represents a channel (e.g., a socket)
	using channel_param_type       = channel_type;
	using channel_const_param_type = channel_type;

	static constexpr channel_type null_channel = 0ULL;
	*/
};

/*
{
	using client_type = ...;
	using server_type = ...;
	using channel_type = ...;
	using request_id_type = std::uint64_t;
	using timeout_type = std::chrono::milliseconds;
	using serialization_traits_type = logicmill::armi::serialization::traits<Serialization>;
	using serializer_type = serialization_traits_type::serializer_type;

	static boolean
	is_valid_channel(client_type const&, channel_type const&);

	static void
	close(client_type&, channel_type&);

	static void
	send_request(client_type&, channel_type& channel, timeout_type timeout, std::unique_ptr<serializer_type>&& request);

	static boolean
	is_valid_channel(server_type const&, channel_type const& channel);

	static void
	close(server_type&, channel_type&);

	static void
	send_reply(server_type&, channel_type& channel, timeout_type timeout, std::unique_ptr<serializer_type>&& reply);


};
*/

}    // namespace async_io
}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_ASYNC_IO_TRAITS_H
