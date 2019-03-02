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

template<class Target> // Target is target class type
class interface_stub : public interface_stub_builder<Target>
{
public:
	using impl_ptr = std::shared_ptr<Target>;
	using interface_stub_builder<Target>::method_count;
	using interface_stub_builder<Target>::get_method_stub;
	using interface_stub_base<Target>::request_failed;

	template<class... Args>
	interface_stub(server_context_base& context, Args... args)
		: interface_stub_builder<Target>(context, typename make_indices<sizeof...(Args)>::type(), args...)
	{}

	void
	process(channel_id_type channel_id, bstream::ibstream& is, impl_ptr impl)
	{
		auto request_id  = is.read_as<request_id_type>();
		auto method_id = is.read_as<std::size_t>();
		if (method_id >= method_count())
		{
			request_failed(request_id, channel_id, make_error_code(armi::errc::invalid_method_id));
		}
		else
		{
			get_method_stub(method_id).dispatch(request_id, channel_id, is, impl);
		}
	}

};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_INTERFACE_STUB_H
