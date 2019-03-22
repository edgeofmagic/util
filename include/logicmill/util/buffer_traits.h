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

#ifndef LOGICMILL_UTIL_BUFFER_TRAITS_H
#define LOGICMILL_UTIL_BUFFER_TRAITS_H

#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <logicmill/types.h>
#include <logicmill/util/macros.h>
#include <logicmill/util/shared_ptr.h>
#include <stdexcept>
#include <system_error>

namespace logicmill
{
namespace util
{
namespace buf
{
template<class Buffer>
class traits
{
	using buffer_type           = Buffer;
	using buffer_ref_type       = buffer_type&;
	using buffer_const_ref_type = buffer_type const&;
	using element_type          = std::uint8_t;
	using pointer_type          = std::uint8_t*;
	using size_type             = std::size_t;
	using const_pointer_type    = void;

	struct is_mutable : public std::true_type
	{};
	struct has_capacity : public std::true_type
	{};
	struct can_set_size : public std::true_type
	{};
	struct can_realloc : public std::true_type
	{};
	struct owns_memory : public std::true_type
	{};
	struct scoped_release : public std::true_type
	{};
	struct data_may_return_null : public std::true_type
	{};
	struct is_copy_constructible : public std::true_type
	{};
	struct is_move_constructible : public std::true_type
	{};
	struct is_copy_assignable : public std::true_type
	{};
	struct is_move_assignable : public std::true_type
	{};
	struct has_explicit_release : public std::false_type
	{};

	template<class... Args>
	struct is_constructible_from : public std::false_type
	{};

	static std::enable_if_t<is_mutable::value, pointer_type>        data(buffer_ref_type);
	static std::enable_if_t<!is_mutable::value, const_pointer_type> data(buffer_const_ref_type);
	static size_type                                                size(buffer_const_ref_type);
	static std::enable_if_t<has_capacity::value, size_type>         capacity(buffer_const_ref_type);
	static std::enable_if_t<can_set_size::value, void>              set_size(buffer_ref_type, size_type);
	static std::enable_if_t<can_realloc::value, bool>               realloc(buffer_ref_type, size_type);
	static std::enable_if_t<has_explicit_release::value, void>      release(buffer_ref_type);

	static_assert(!(!is_mutable::value && can_realloc::value), "non-mutable buffer types can't realloc");
	static_assert(!(!is_mutable::value && can_set_size::value), "non-mutable buffer types can't set_size");
	static_assert(!(!owns_memory::value && scoped_release::value), "non-mutable buffer types can't set_size");
	static_assert(!(!owns_memory::value && has_explicit_release::value), "non-mutable buffer types can't set_size");
};
}    // namespace buf
}    // namespace util
}    // namespace logicmill

#endif    // LOGICMILL_UTIL_BUFFER_TRAITS_H
