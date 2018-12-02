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

#ifndef LOGICMILL_BSTREAM_UTILS_TRAITS_H
#define LOGICMILL_BSTREAM_UTILS_TRAITS_H

#include <type_traits>
#include <utility>

namespace logicmill
{
namespace bstream
{
namespace utils
{
template<class>
struct sfinae_true_if : std::true_type
{};

template<bool... B>
struct conjunction
{};

template<bool Head, bool... Tail>
struct conjunction<Head, Tail...> : std::integral_constant<bool, Head && conjunction<Tail...>::value>
{};

template<bool B>
struct conjunction<B> : std::integral_constant<bool, B>
{};

template<class T, class... Ts>
struct index;

template<class T, class... Ts>
struct index<T, T, Ts...> : std::integral_constant<std::size_t, 0>
{};

template<class T, class U, class... Ts>
struct index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + index<T, Ts...>::value>
{};

template<class T, class U>
struct index_from;

template<class T, template<class...> class ArgCarrier, class... Args>
struct index_from<T, ArgCarrier<Args...>>
{
	static constexpr std::size_t value = index<T, Args...>::value;
};

template<std::size_t I, typename T>
struct _indexed
{
	using type = T;
};

template<typename Is, typename... Ts>
struct _indexer;

template<std::size_t... Is, typename... Ts>
struct _indexer<std::index_sequence<Is...>, Ts...> : _indexed<Is, Ts>...
{};

template<std::size_t I, typename T>
static _indexed<I, T> select(_indexed<I, T>);

template<std::size_t I, typename... Ts>
using nth_element = typename decltype(select<I>(_indexer<std::index_sequence_for<Ts...>, Ts...>{}))::type;

}    // namespace utils
}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_UTILS_TRAITS_H
