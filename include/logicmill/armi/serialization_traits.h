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

#ifndef LOGICMILL_ARMI_SERIALIZATION_TRAITS_H
#define LOGICMILL_ARMI_SERIALIZATION_TRAITS_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <logicmill/armi/types.h>
#include <system_error>

namespace logicmill
{
namespace armi
{
namespace serialization
{

struct traits
{
/*	
	using serializer_type     = ...; // output archive/stream
	using deserializer_type   = ...; // input archive/stream

	template<class T>
	static T
	read(deserializer_type& input)
	{
		// Deserialize type T from input and return it by value.
		// Should be the equivalent of (assuming T is default-constructible):
		T value;
		read(intput, value);
		return value;
	}

	template<class T>
	static void
	read(deserializer_type& input, T& value)
	{
		// Deserialize type T from input into the value parameter.
	}

	template<class T>
	static T
	write(serializer_type& output, T const& value)
	{
		// Serialize value to output.
	}

	static void
	write_sequence_prefix(serializer_type& os, std::size_t count)
	{
		// Some serialization frameworks want to encapsulate sequence by
		// inserting a prefix that includes a count of the items in the sequence.
		// Minimally, just write the count as an integer.
	}

	static std::size_t
	read_sequence_prefix(deserializer_type& is)
	{
		// See write_sequence_prefix; read the prefix and 
		// return the count. Minimally, read an integer.
	}

	static std::unique_ptr<serializer_type>
	create_serializer()
	{
		// Create a unique pointer to a serializer and return it.
	}

	static void
	clear(deserializer_type& is)
	{
		// Reset the state of the deserializer to the same as newly-constructed.
	}
*/
};

}    // namespace serialization
}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_SERIALIZATION_TRAITS_H
