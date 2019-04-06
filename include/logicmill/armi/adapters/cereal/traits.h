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
// #include <logicmill/armi/serialization_traits.h>
#include <logicmill/armi/types.h>
#include <logicmill/util/error_context.h>
#include <logicmill/util/membuf.h>

namespace logicmill
{
namespace armi
{
namespace adapters
{
namespace cereal
{

template<class ErrorContext = logicmill::util::default_error_context>
class cerealizer
{
public:
	using archive_type       = ::cereal::BinaryOutputArchive;
	using error_context_type = ErrorContext;

	cerealizer(std::size_t initial_alloc_size) : m_omembuf{initial_alloc_size}, m_os{&m_omembuf}, m_arch{m_os} {}

	cerealizer(util::mutable_buffer&& buf) : m_omembuf{std::move(buf)}, m_os{&m_omembuf}, m_arch{m_os} {}

	template<class T>
	std::enable_if_t<!std::is_same<T, std::error_code>::value, void>
	write(T const& value)
	{
		m_arch(value);
	}

	template<class T>
	std::enable_if_t<std::is_same<T, std::error_code>::value, void>
	write(T const& err)
	{
		m_arch(error_context_type::index_of_category(err.category()));
		m_arch(err.value());
	}

	util::mutable_buffer
	release_buffer()
	{
		return m_omembuf.release_buffer();
	}

private:
	util::omembuf m_omembuf;
	std::ostream  m_os;
	archive_type  m_arch;
};

template<class ErrorContext = logicmill::util::default_error_context>
class decerealizer
{
public:
	using archive_type       = ::cereal::BinaryInputArchive;
	using error_context_type = ErrorContext;

	decerealizer(util::const_buffer&& buf) : m_imembuf{std::move(buf)}, m_is{&m_imembuf}, m_arch{m_is} {}

	template<class T>
	std::enable_if_t<!std::is_same<T, std::error_code>::value, T>
	read_as()
	{
		T value;
		m_arch(value);
		return value;
	}

	template<class T>
	std::enable_if_t<!std::is_same<T, std::error_code>::value, void>
	read_as(T& value)
	{
		m_arch(value);
	}

	template<class T>
	std::enable_if_t<std::is_same<T, std::error_code>::value, T>
	read_as()
	{
		std::error_code err;
		int             value;
		int             index;
		m_arch(index, value);
		return std::error_code{value, error_context_type::category_from_index(index)};
	}

	template<class T>
	std::enable_if_t<std::is_same<T, std::error_code>::value, void>
	read_as(T& err)
	{
		int value;
		int index;
		m_arch(index, value);
		err = std::error_code{value, error_context_type::category_from_index(index)};
	}

	util::const_buffer
	release_buffer()
	{
		return m_imembuf.release_buffer();
	}

private:
	util::imembuf m_imembuf;
	std::istream  m_is;
	archive_type  m_arch;
};

template<class ErrorContext = util::default_error_context>
struct serialization_traits
{
	using serializer_type         = cerealizer<ErrorContext>;
	using serializer_param_type   = serializer_type&;    // lvalue non-const reference
	using deserializer_type       = decerealizer<ErrorContext>;
	using deserializer_param_type = deserializer_type&;

	template<class T>
	static T
	read(deserializer_param_type is)
	{
		return is.template read_as<T>();
	}

	template<class T>
	static void
	read(deserializer_param_type is, T& value)
	{
		is.template read_as(value);
	}

	template<class T>
	static void
	write(serializer_type& os, T const& value)
	{
		os.write(value);
	}

	static void
	write_sequence_prefix(serializer_type& os, std::size_t count)
	{
		os.write(count);
	}

	static std::size_t
	read_sequence_prefix(deserializer_param_type is)
	{
		return is.template read_as<std::size_t>();
	}

	// static std::unique_ptr<serializer_type>
	// create_serializer()
	// {
	// 	return std::make_unique<serializer_type>(stream_context_type::get());
	// }

	static void
	clear(serializer_type& os)
	{
		//		os.clear();
	}
};

}    // namespace cereal
}    // namespace adapters
}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_ADAPTER_CEREAL_TRAITS_H
