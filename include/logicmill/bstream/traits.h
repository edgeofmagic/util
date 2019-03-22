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

#ifndef LOGICMILL_BSTREAM_TRAITS_H
#define LOGICMILL_BSTREAM_TRAITS_H

#include <logicmill/armi/serialization_traits.h>
#include <logicmill/armi/types.h>
#include <logicmill/bstream.h>

namespace logicmill
{
namespace bstream
{
template<class StreamContext>
struct serialization
{
	using stream_context_type = StreamContext > ;
};
}    // namespace bstream
}    // namespace logicmill

template<class StreamContext>
struct logicmill::armi::serialization::traits<logicmill::bstream::serialization<StreamContext>>
{
	using serializer_type     = logicmill::bstream::obstream;
	using deserializer_type   = logicmill::bstream::ibstream;
	using stream_context_type = StreamContext;

	template<class T>
	static T
	read(deserializer_type& is)
	{
		return is.read_as<T>();
	}

	template<class T>
	static void
	read(deserializer_type& is, T& value)
	{
		is.read_as(value);
	}

	template<class T>
	static T
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
	read_sequence_prefix(deserializer_type& is)
	{
		return is.read_array_header();
	}

	static std::unique_ptr<serializer_type>
	create_serializer()
	{
		return std::make_unique<serializer_type>(stream_context_type::get());
	}

	static void
	clear(deserializer_type& is)
	{
		is.clear();
	}
};

#endif    // LOGICMILL_BSTREAM_TRAITS_H
