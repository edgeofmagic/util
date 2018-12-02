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

#ifndef LOGICMILL_TRAITS_H
#define LOGICMILL_TRAITS_H

#include <tuple>
#include <type_traits>
#include <utility>

namespace logicmill
{
namespace traits
{
template<bool... B>
struct conjunction
{};

template<bool Head, bool... Tail>
struct conjunction<Head, Tail...> : std::integral_constant<bool, Head && conjunction<Tail...>::value>
{};

template<bool B>
struct conjunction<B> : std::integral_constant<bool, B>
{};

template<class T, template<class...> class Template>
struct is_specialization : std::false_type
{};

template<template<class...> class Template, class... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type
{};

template<std::size_t I = 0, class FuncT, class... Tp>
inline typename std::enable_if_t<I == sizeof...(Tp)>
for_each(std::tuple<Tp...>&, FuncT)
{}

template<std::size_t I = 0, class FuncT, class... Tp>
inline typename std::enable_if_t < I<sizeof...(Tp)>
for_each(std::tuple<Tp...>& t, FuncT f)
{
	f(std::get<I>(t));
	for_each<I + 1, FuncT, Tp...>(t, f);
}

}    // namespace traits
}    // namespace logicmill

#endif    // LOGICMILL_TRAITS_H