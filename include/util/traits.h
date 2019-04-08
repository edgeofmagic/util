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

#ifndef UTIL_TRAITS_H
#define UTIL_TRAITS_H

#include <tuple>
#include <type_traits>
#include <utility>

namespace traits
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

template<std::size_t I, class ArgCarrier>
struct nth_element_from;

template<std::size_t I, template<class...> class ArgCarrier, class... Args>
struct nth_element_from<I, ArgCarrier<Args...>>
{
	using type = nth_element<I, Args...>;
};

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
inline typename std::enable_if_t<(I < sizeof...(Tp))>
for_each(std::tuple<Tp...>& t, FuncT f)
{
	f(std::get<I>(t));
	for_each<I + 1, FuncT, Tp...>(t, f);
}

template<class... Args>
struct _arg_list;

template<class... Args>
struct first_arg;

template<template<class...> class Template, class First, class... Rest>
struct first_arg<Template<First, Rest...>>
{
	using type = First;
};

template<class T, class U>
class replace_arg;

template<template<class...> class Template, class Arg1, class Arg2>
class replace_arg<Template<Arg1>, Arg2>
{
public:
	using type = Template<Arg2>;
};

template<class List, class Template>
struct apply_arg_list;

template<template<class...> class List, class... ListArgs, template<class...> class Template, class Arg>
struct apply_arg_list<List<ListArgs...>, Template<Arg>>
{
	using type = _arg_list<Template<ListArgs>...>;
};

template<class T, class... Args>
struct apply_args;

template<template<class...> class Template, class U, class... Args>
struct apply_args<Template<U>, Args...>
{
	using type = _arg_list<Template<Args>...>;
};

template<class T>
struct remove_member_func_cv_noexcept;

template<class Target, class Return, class... Args>
struct remove_member_func_cv_noexcept<Return (Target::*)(Args...)>
{
	using type = Return (Target::*)(Args...);
};

template<class Target, class Return, class... Args>
struct remove_member_func_cv_noexcept<Return (Target::*)(Args...) const>
{
	using type = Return (Target::*)(Args...);
};

template<class Target, class Return, class... Args>
struct remove_member_func_cv_noexcept<Return (Target::*)(Args...) volatile>
{
	using type = Return (Target::*)(Args...);
};

template<class Target, class Return, class... Args>
struct remove_member_func_cv_noexcept<Return (Target::*)(Args...) const volatile>
{
	using type = Return (Target::*)(Args...);
};

template<class Target, class Return, class... Args>
struct remove_member_func_cv_noexcept<Return (Target::*)(Args...) noexcept>
{
	using type = Return (Target::*)(Args...);
};

template<class Target, class Return, class... Args>
struct remove_member_func_cv_noexcept<Return (Target::*)(Args...) const noexcept>
{
	using type = Return (Target::*)(Args...);
};

template<class Target, class Return, class... Args>
struct remove_member_func_cv_noexcept<Return (Target::*)(Args...) volatile noexcept>
{
	using type = Return (Target::*)(Args...);
};

template<class Target, class Return, class... Args>
struct remove_member_func_cv_noexcept<Return (Target::*)(Args...) const volatile noexcept>
{
	using type = Return (Target::*)(Args...);
};

namespace detail
{

template<class T>
static auto
test_has_allocate(int)
		-> traits::sfinae_true_if<decltype(std::declval<T>().allocate(0))>;
template<class>
static auto
test_has_allocate(long) -> std::false_type;
}    // namespace detail

template<class T>
struct has_allocate : decltype(detail::test_has_allocate<T>(0))
{};



// template <class T>
// struct has_allocate
// {
// private:
//     template <class U> static std::false_type test(...);
//     template <class U> static std::true_type test(decltype(std::declval<U>().allocate(0)));
// public:
//     enum { value = decltype(test<T>(0))::value };
// };


namespace detail
{

template<class T>
static auto
test_has_value_type(int)
		-> traits::sfinae_true_if<typename T::value_type>;
template<class>
static auto
test_has_value_type(long) -> std::false_type;
}    // namespace detail

template<class T>
struct has_value_type : decltype(detail::test_has_value_type<T>(0))
{};


// template <class T>
// struct __has_value_type
// {
// private:
//     template <class U> static std::false_type test(...);
//     template <class U> static std::true_type test(typename U::value_type*);
// public:
//     enum { value = decltype(test<T>(0))::value };
// };

namespace detail
{

template<class Alloc, class Pointer>
static auto
test_has_deallocate(int)
		-> traits::sfinae_true_if<decltype(std::declval<Alloc>().deallocate(std::declval<Pointer>(), 0))>;

template<class>
static auto
test_has_deallocate(long) -> std::false_type;
}    // namespace detail

template<class T, class Enable = void>
struct has_deallocate : public std::false_type {};

template<class T>
struct has_deallocate<T, typename std::enable_if_t<has_allocate<T>::value>>
    : decltype(detail::test_has_deallocate<T, decltype(std::declval<T>().allocate(0))>(0))
{};


// template <class T, bool HasAllocate = has_allocate<T>::value>
// struct __has_deallocate
// {
// private:

//     typedef decltype(std::declval<T>().allocate(0)) pointer;

//     template <class Alloc, class Pointer>
//     static auto
//     test(Alloc&& a, Pointer&& p)
//     -> decltype(a.deallocate(p,0), std::true_type());

//     template <class Alloc, class Pointer>
//     static auto
//     test(const Alloc& a, Pointer&& p)
//     -> std::false_type;

// public:
//     enum { value = decltype(test<T>(std::declval<T>(), std::declval<pointer>()))::value };
// };


// template <class T>
// struct __has_deallocate<T, false>
// {
//     enum { value = false };
// };

template <class T, class Enable = void>
struct is_allocator : public std::false_type {};

template<class T>
struct is_allocator<T, typename std::enable_if_t<
	has_allocate<T>::value && has_deallocate<T>::value && has_value_type<T>::value
>> : public std::true_type {};

// template <class T>
// struct is_allocator
// {
//     enum { value =  __has_value_type<T>::value
//                 and __has_allocate<T>::value
//                 and __has_deallocate<T>::value
//     };
// };

}    // namespace traits

#endif    // UTIL_TRAITS_H