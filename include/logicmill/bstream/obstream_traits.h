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

#ifndef LOGICMILL_BSTREAM_OBSTREAM_TRAITS_H
#define LOGICMILL_BSTREAM_OBSTREAM_TRAITS_H

#include <logicmill/bstream/fwd_decls.h>
#include <logicmill/traits.h>
#include <type_traits>

namespace logicmill
{
namespace bstream
{

namespace detail
{
template<class T>
static auto
test_serialize_impl_method(int)
		-> traits::sfinae_true_if<decltype(std::declval<T>().serialize_impl(std::declval<obstream&>()))>;
template<class>
static auto
test_serialize_impl_method(long) -> std::false_type;
}    // namespace detail

template<class T>
struct has_serialize_impl_method : decltype(detail::test_serialize_impl_method<T>(0))
{};

namespace detail
{
template<class T>
static auto
test_serialize_method(int) -> traits::sfinae_true_if<decltype(std::declval<T>().serialize(std::declval<obstream&>()))>;
template<class>
static auto
test_serialize_method(long) -> std::false_type;
}    // namespace detail

template<class T>
struct has_serialize_method : decltype(detail::test_serialize_method<T>(0))
{};

namespace detail
{
template<class T>
static auto
test_serializer(int)
		-> traits::sfinae_true_if<decltype(serializer<T>::put(std::declval<obstream&>(), std::declval<T>()))>;
template<class>
static auto
test_serializer(long) -> std::false_type;
}    // namespace detail

template<class T>
struct has_serializer : decltype(detail::test_serializer<T>(0))
{};

namespace detail
{
template<class T>
static auto
test_obstream_insertion_operator(int)
		-> traits::sfinae_true_if<decltype(std::declval<obstream&>() << std::declval<T>())>;
template<class>
static auto
test_obstream_insertion_operator(long) -> std::false_type;
}    // namespace detail

template<class T>
struct has_obstream_insertion_operator : decltype(detail::test_obstream_insertion_operator<T>(0))
{};

template<class T, class Enable = void>
struct is_serializable : public std::false_type
{};

template<class T>
struct is_serializable<T, std::enable_if_t<has_serializer<T>::value>> : public std::true_type
{};

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_OBSTREAM_TRAITS_H
