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

#ifndef LOGICMILL_ARMI_BSTREAM_ADAPTER_TRAITS_H
#define LOGICMILL_ARMI_BSTREAM_ADAPTER_TRAITS_H

#include <logicmill/armi/error.h>
// #include <logicmill/armi/types.h>
// #include <logicmill/async/loop.h>
#include <logicmill/bstream.h>
#include <logicmill/bstream/imbstream.h>
#include <logicmill/bstream/ombstream.h>

namespace logicmill
{
namespace bstream
{

class default_armi_stream_context
{
public:
	using context_type = bstream::context<>;

	static bstream::context_options
	options()
	{
		return bstream::context_options{}.error_categories({&armi::error_category()});
	}

	static bstream::context_base const& get()
	{
		static const context_type instance{options()};
		return instance;
	}

	// BSTRM_CONTEXT_ACCESSOR();
};


template<class StreamContext = default_armi_stream_context>
struct serialization_traits
{
	using serializer_type         = logicmill::bstream::ombstream;
	using serializer_param_type   = serializer_type&;    // lvalue non-const reference
	using deserializer_type       = logicmill::bstream::imbstream;
	using deserializer_param_type = deserializer_type&;
	using stream_context_type     = StreamContext;

	template<class T>
	static T
	read(deserializer_param_type is)
	{
		return is.read_as<T>();
	}

	template<class T>
	static void
	read(deserializer_param_type is, T& value)
	{
		is.read_as(value);
	}

	template<class T>
	static void
	write(serializer_type& os, T const& value)
	{
		os << value;
	}

	static void
	write_sequence_prefix(serializer_type& os, std::size_t count)
	{
		os.write_array_header(count);
	}

	static std::size_t
	read_sequence_prefix(deserializer_param_type is)
	{
		return is.read_array_header();
	}

	static std::unique_ptr<serializer_type>
	create_serializer()
	{
		return std::make_unique<serializer_type>(stream_context_type::get());
	}

	static void
	clear(serializer_type& os)
	{
		os.clear();
	}
};

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_BSTREAM_ADAPTER_TRAITS_H
