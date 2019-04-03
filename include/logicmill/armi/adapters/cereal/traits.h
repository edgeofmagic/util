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

#ifndef LOGICMILL_ARMI_ADAPTER_CEREAL_TRAITS_H
#define LOGICMILL_ARMI_ADAPTER_CEREAL_TRAITS_H

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <functional>
#include <logicmill/armi/serialization_traits.h>
#include <logicmill/armi/types.h>

namespace logicmill
{
namespace armi
{
namespace adapters
{
namespace serialization
{
struct cereal {};
}    // namespace serialization
}    // namespace adapters
}    // namespace armi
}    // namespace logicmill

template<>
struct logicmill::armi::serialization::traits<logicmill::armi::adapters::serialization::cereal>
{
	using serializer_type = cereal::BinaryOutputArchive;
	using deserializer_type = cereal::BinaryInputArchive;
	using serializer_param_type = serializer_type&;
	using deserializer_param_type = deserializer_type&;

	template<class T>
	static T
	read(deserializer_param_type arch)
	{
		T value arch(value);
		return value;
	}

	template<class T>
	static void
	read(deserializer_param_type arch, T& value)
	{
		arch(value);
	}

	template<class T>
	static T
	write(serializer_param_type arch, T const& value)
	{
		arch(value);
	}

	static void
	write_sequence_prefix(serializer_param_type arch, std::size_t count)
	{
		arch(count);
	}

	static std::size_t
	read_sequence_prefix(deserializer_param_type arch)
	{
		std::size_t count;
		arch(count);
		return count;
	}
};

#endif    // LOGICMILL_ARMI_ADAPTER_CEREAL_TRAITS_H
