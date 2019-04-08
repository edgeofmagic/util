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

#ifndef LOGICMILL_ARMI_INTERFACE_STUB_H
#define LOGICMILL_ARMI_INTERFACE_STUB_H

#include <logicmill/armi/interface_stub_builder.h>

namespace logicmill
{
namespace armi
{

template<class Target, class ServerStubBase>
class interface_stub;

template<
		class Target,
		template<class...> class ServerStubBaseTemplate,
		class SerializationTraits,
		class AsyncIOTraits>
class interface_stub<Target, ServerStubBaseTemplate<SerializationTraits, AsyncIOTraits>>
	: public interface_stub_builder<Target, ServerStubBaseTemplate<SerializationTraits, AsyncIOTraits>>
{
public:
	using server_stub_base_type = ServerStubBaseTemplate<SerializationTraits, AsyncIOTraits>;
	using base                     = interface_stub_builder<Target, server_stub_base_type>;
	using target_ptr                 = std::shared_ptr<Target>;
	using base::member_func_count;
	using base::get_member_func_stub;
	using interface_stub_base<Target, server_stub_base_type>::request_failed;

	using serialization_traits    = SerializationTraits;
	using async_io_traits        = AsyncIOTraits;
	using deserializer_type       = typename serialization_traits::deserializer_type;
	using serializer_type         = typename serialization_traits::serializer_type;
	using bridge_type             = logicmill::armi::adapters::bridge<serialization_traits, async_io_traits>;
	using deserializer_param_type = typename bridge_type::deserializer_param_type;


	template<class... Args>
	interface_stub(server_stub_base_type* server, Args... args)
		: base{server, typename make_indices<sizeof...(Args)>::type(), args...}
	{}

	void
	process(channel_id_type channel, deserializer_param_type request, target_ptr impl)
	{
		auto request_id     = serialization_traits::template read<request_id_type>(request);
		auto member_func_id = serialization_traits::template read<std::size_t>(request);

		if (member_func_id >= member_func_count())
		{
			request_failed(request_id, channel, make_error_code(armi::errc::invalid_member_func_id));
		}
		else
		{
			get_member_func_stub(member_func_id).dispatch(request_id, channel, request, impl);
		}
	}
};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_INTERFACE_STUB_H
