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

#ifndef LOGICMILL_BSTREAM_STDLIB_PAIR_H
#define LOGICMILL_BSTREAM_STDLIB_PAIR_H

#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/obstream.h>
#include <utility>

namespace logicmill
{
namespace bstream
{

template<class T1, class T2>
struct value_deserializer<
		std::pair<T1, T2>,
		typename std::enable_if_t<std::is_move_constructible<T1>::value && std::is_move_constructible<T2>::value>>
{
	std::pair<T1, T2>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::pair<T1, T2>
	get(ibstream& is)
	{
		is.check_array_header(2);
		T1 t1{ibstream_initializer<T1>::get(is)};
		T2 t2{ibstream_initializer<T2>::get(is)};
		return std::make_pair(std::move(t1), std::move(t2));
	}
};

template<class T1, class T2>
struct value_deserializer<
		std::pair<T1, T2>,
		typename std::enable_if_t<
				(!std::is_move_constructible<T1>::value && std::is_copy_constructible<T1>::value)
				&& std::is_move_constructible<T2>::value>>
{
	std::pair<T1, T2>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::pair<T1, T2>
	get(ibstream& is)
	{
		is.check_array_header(2);
		T1 t1{ibstream_initializer<T1>::get(is)};
		T2 t2{ibstream_initializer<T2>::get(is)};
		return std::make_pair(t1, std::move(t2));
	}
};

template<class T1, class T2>
struct value_deserializer<
		std::pair<T1, T2>,
		typename std::enable_if_t<
				std::is_move_constructible<T1>::value
				&& (!std::is_move_constructible<T2>::value && std::is_copy_constructible<T2>::value)>>
{
	std::pair<T1, T2>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::pair<T1, T2>
	get(ibstream& is)
	{
		is.check_array_header(2);
		T1 t1{ibstream_initializer<T1>::get(is)};
		T2 t2{ibstream_initializer<T2>::get(is)};
		return std::make_pair(std::move(t1), t2);
	}
};

template<class T1, class T2>
struct value_deserializer<
		std::pair<T1, T2>,
		typename std::enable_if_t<
				(!std::is_move_constructible<T1>::value && std::is_copy_constructible<T1>::value)
				&& (!std::is_move_constructible<T2>::value && std::is_copy_constructible<T2>::value)>>
{
	std::pair<T1, T2>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::pair<T1, T2>
	get(ibstream& is)
	{
		is.check_array_header(2);
		T1 t1{ibstream_initializer<T1>::get(is)};
		T2 t2{ibstream_initializer<T2>::get(is)};
		return std::make_pair(t1, t2);
	}
};

template<class T1, class T2>
struct serializer<std::pair<T1, T2>>
{
	static obstream&
	put(obstream& os, std::pair<T1, T2> const& p)
	{
		os.write_array_header(2);
		os << p.first;
		os << p.second;
		return os;
	}
};

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_STDLIB_PAIR_H