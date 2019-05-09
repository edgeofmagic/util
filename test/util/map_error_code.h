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

#ifndef ARMI_ADAPTER_ASIO_MAP_ERROR_CODE_H
#define ARMI_ADAPTER_ASIO_MAP_ERROR_CODE_H

#include <boost/asio/error.hpp>
#include <boost/system/system_error.hpp>
#include <iostream>
#include <system_error>
#include <unordered_map>

namespace asio_adapter
{

inline std::error_code
map_error_code(boost::system::error_code const& error)
{
	struct category_adapter : public std::error_category
	{
		category_adapter(const boost::system::error_category& boost_category) : m_category(boost_category) {}

		const char*
		name() const noexcept
		{
			return std::string{m_category.name()}.append("boost").c_str();
		}

		std::string
		message(int ev) const
		{
			return m_category.message(ev);
		}

	private:
		const boost::system::error_category& m_category;
	};

	static thread_local std::unordered_map<const boost::system::error_category*, category_adapter>
			boost_category_to_adapter;

	if (error.category() == boost::system::generic_category())
	{
		return std::error_code{error.value(), std::generic_category()};
	}
	else if (error.category() == boost::system::system_category())
	{
		return std::error_code{error.value(), std::system_category()};
	}
	else
	{
		auto  result   = boost_category_to_adapter.emplace(&error.category(), error.category());
		auto& category = result.first->second;
		return std::error_code{error.value(), category};
	}
}

}    // namespace asio_adapter

#endif    // ARMI_ADAPTER_ASIO_MAP_ERROR_CODE_H