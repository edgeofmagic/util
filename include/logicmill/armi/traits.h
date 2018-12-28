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

#ifndef LOGICMILL_ARMI_TRAITS_H
#define LOGICMILL_ARMI_TRAITS_H

#include <functional>
#include <logicmill/traits.h>
#include <type_traits>

namespace logicmill
{
namespace armi
{

template<class F, class Enable = void>
struct is_error_safe_reply : public std::false_type
{};

template<class F, class Enable = void>
struct is_reply : public std::false_type
{};

template<class... Args>
struct is_reply<std::function<void(Args...)>, void> : public std::true_type
{};

template<class... Args>
struct is_error_safe_reply<
		std::function<void(std::error_code, Args...)>,
		typename std::enable_if_t<traits::conjunction<
				std::is_default_constructible<std::remove_cv_t<std::remove_reference_t<Args>>>::value...>::value>>
	: public std::true_type
{};

template<>
struct is_error_safe_reply<std::function<void(std::error_code)>> : public std::true_type
{};

}    // namespace armi
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_TRAITS_H
