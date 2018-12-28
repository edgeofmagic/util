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

#ifndef LOGICMILL_BSTREAM_STDLIB_MAP_H
#define LOGICMILL_BSTREAM_STDLIB_MAP_H

#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/obstream.h>
#include <map>

namespace logicmill
{
namespace bstream
{

template<class K, class V, class Compare, class Alloc>
struct value_deserializer<
		std::map<K, V, Compare, Alloc>,
		typename std::enable_if_t<is_ibstream_readable<K>::value && is_ibstream_readable<V>::value>>
{
	std::map<K, V, Compare, Alloc>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::map<K, V, Compare, Alloc>
	get(ibstream& is)
	{
		using pair_type = std::pair<K, V>;
		using map_type  = std::map<K, V, Compare, Alloc>;
		auto     length = is.read_array_header();
		map_type result;
		for (auto i = 0u; i < length; ++i)
		{
			result.insert(is.read_as<pair_type>());
		}
		return result;
	}
};

template<class K, class V, class Compare, class Alloc>
struct value_deserializer<
		std::multimap<K, V, Compare, Alloc>,
		typename std::enable_if_t<is_ibstream_readable<K>::value && is_ibstream_readable<V>::value>>
{
	std::multimap<K, V, Compare, Alloc>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::multimap<K, V, Compare, Alloc>
	get(ibstream& is)
	{
		using pair_type = std::pair<K, V>;
		using map_type  = std::multimap<K, V, Compare, Alloc>;
		auto     length = is.read_array_header();
		map_type result;
		for (auto i = 0u; i < length; ++i)
		{
			result.insert(is.read_as<pair_type>());
		}
		return result;
	}
};

template<class K, class V, class Compare, class Alloc>
struct serializer<std::map<K, V, Compare, Alloc>>
{
	static obstream&
	put(obstream& os, std::map<K, V, Compare, Alloc> const& mp)
	{
		os.write_array_header(mp.size());
		for (auto it = mp.begin(); it != mp.end(); ++it)
		{
			os.write_array_header(2);
			os << it->first;
			os << it->second;
		}
		return os;
	}
};

template<class K, class V, class Compare, class Alloc>
struct serializer<std::multimap<K, V, Compare, Alloc>>
{
	static obstream&
	put(obstream& os, std::multimap<K, V, Compare, Alloc> const& mp)
	{
		os.write_array_header(mp.size());
		for (auto it = mp.begin(); it != mp.end(); ++it)
		{
			os.write_array_header(2);
			os << it->first;
			os << it->second;
		}
		return os;
	}
};

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_STDLIB_MAP_H