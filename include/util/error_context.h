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

#ifndef UTIL_ERROR_CONTEXT_H
#define UTIL_ERROR_CONTEXT_H

#ifndef BOOST_PP_VARIADICS
#define BOOST_PP_VARIADICS 1
#endif

#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/paren.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <cstdint>
#include <system_error>
#include <unordered_map>
#include <util/error.h>
#include <vector>

#define UTIL_DEFINE_ERROR_CONTEXT(...)                                                                                 \
	BOOST_PP_IF(                                                                                                       \
			BOOST_PP_GREATER(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), 1),                                                  \
			UTIL_DEFINE_ERROR_CONTEXT_WITH_ARGS_,                                                                      \
			UTIL_DEFINE_ERROR_CONTEXT_NO_ARGS_)                                                                        \
	(__VA_ARGS__)

#define UTIL_DEFINE_ERROR_CONTEXT_WITH_ARGS_(CONTEXT_NAME, ...)                                                        \
	UTIL_DEFINE_ERROR_CONTEXT_WITH_ARGS_SEQ_(CONTEXT_NAME, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define UTIL_DEFINE_ERROR_CONTEXT_WITH_ARGS_SEQ_(CONTEXT_NAME, ERR_CAT_SEQ)                                            \
	class CONTEXT_NAME                                                                                                 \
	{                                                                                                                  \
	public:                                                                                                            \
		using index_type = util::error_context::index_type;                                                            \
		static std::error_category const&                                                                              \
		category_from_index(index_type index)                                                                          \
		{                                                                                                              \
			return get_context().category_from_index(index);                                                           \
		}                                                                                                              \
		static index_type                                                                                              \
		index_of_category(std::error_category const& category)                                                         \
		{                                                                                                              \
			return get_context().index_of_category(category);                                                          \
		}                                                                                                              \
                                                                                                                       \
	private:                                                                                                           \
		static util::error_context const&                                                                              \
		get_context()                                                                                                  \
		{                                                                                                              \
			static util::error_context cntxt(                                                                          \
					{&std::system_category(),                                                                          \
					 &std::generic_category(),                                                                         \
					 &util::error_category() BOOST_PP_SEQ_FOR_EACH_I(UTIL_DO_ERROR_CAT_, _, ERR_CAT_SEQ)});            \
			return cntxt;                                                                                              \
		}                                                                                                              \
	};


#define UTIL_DO_ERROR_CAT_(R, DATA, I, ELEM) , &ELEM BOOST_PP_LPAREN() BOOST_PP_RPAREN()

#define UTIL_DEFINE_ERROR_CONTEXT_NO_ARGS_(CONTEXT_NAME)                                                               \
	class CONTEXT_NAME                                                                                                 \
	{                                                                                                                  \
	public:                                                                                                            \
		using index_type = util::error_context::index_type;                                                            \
		static std::error_category const&                                                                              \
		category_from_index(index_type index)                                                                          \
		{                                                                                                              \
			return get_context().category_from_index(index);                                                           \
		}                                                                                                              \
		static index_type                                                                                              \
		index_of_category(std::error_category const& category)                                                         \
		{                                                                                                              \
			return get_context().index_of_category(category);                                                          \
		}                                                                                                              \
                                                                                                                       \
	private:                                                                                                           \
		static util::error_context const&                                                                              \
		get_context()                                                                                                  \
		{                                                                                                              \
			static util::error_context cntxt(                                                                          \
					{&std::system_category(), &std::generic_category(), &util::error_category()});                     \
			return cntxt;                                                                                              \
		}                                                                                                              \
	};


namespace util
{

class error_context
{
public:
	using index_type      = int;
	using category_vector = std::vector<const std::error_category*>;
	using category_map    = std::unordered_map<const std::error_category*, index_type>;

	error_context() {}

	error_context(category_vector&& categories);

	error_context(category_vector const& categories);

	std::error_category const&
	category_from_index(index_type index) const;

	index_type
	index_of_category(std::error_category const& category) const;

private:
	category_vector m_category_vector;
	category_map    m_category_map;
};


UTIL_DEFINE_ERROR_CONTEXT(default_error_context);

}    // namespace util

#endif    // UTIL_ERROR_CONTEXT_H