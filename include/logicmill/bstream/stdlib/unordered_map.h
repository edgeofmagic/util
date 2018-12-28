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

#ifndef LOGICMILL_BSTREAM_STDLIB_UNORDERED_MAP_H
#define LOGICMILL_BSTREAM_STDLIB_UNORDERED_MAP_H

#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/obstream.h>
#include <unordered_map>

namespace logicmill
{
namespace bstream
{

template<class K, class V, class Hash, class Equal, class Alloc>
struct value_deserializer<
		std::unordered_map<K, V, Hash, Equal, Alloc>,
		typename std::enable_if_t<is_ibstream_readable<K>::value && is_ibstream_readable<V>::value>>
{
	std::unordered_map<K, V, Hash, Equal, Alloc>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::unordered_map<K, V, Hash, Equal, Alloc>
	get(ibstream& is)
	{
		using pair_type = std::pair<K, V>;
		using map_type  = std::unordered_map<K, V, Hash, Equal, Alloc>;

		auto     length = is.read_array_header();
		map_type result;
		result.reserve(length);
		for (auto i = 0u; i < length; ++i)
		{
			result.insert(is.read_as<pair_type>());
		}
		return result;
	}
};

template<class K, class V, class Hash, class Equal, class Alloc>
struct value_deserializer<
		std::unordered_multimap<K, V, Hash, Equal, Alloc>,
		typename std::enable_if_t<is_ibstream_readable<K>::value && is_ibstream_readable<V>::value>>
{
	std::unordered_multimap<K, V, Hash, Equal, Alloc>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::unordered_multimap<K, V, Hash, Equal, Alloc>
	get(ibstream& is)
	{
		using pair_type = std::pair<K, V>;
		using map_type  = std::unordered_multimap<K, V, Hash, Equal, Alloc>;

		auto     length = is.read_array_header();
		map_type result;
		result.reserve(length);
		for (auto i = 0u; i < length; ++i)
		{
			result.insert(is.read_as<pair_type>());
		}
		return result;
	}
};

template<class K, class V, class Hash, class Equal, class Alloc>
struct serializer<std::unordered_map<K, V, Hash, Equal, Alloc>>
{
	static obstream&
	put(obstream& os, std::unordered_map<K, V, Hash, Equal, Alloc> const& map)
	{
		os.write_array_header(map.size());
		for (auto it = map.begin(); it != map.end(); ++it)
		{
			os << *it;
		}
		return os;
	}
};

template<class K, class V, class Hash, class Equal, class Alloc>
struct serializer<std::unordered_multimap<K, V, Hash, Equal, Alloc>>
{
	static obstream&
	put(obstream& os, std::unordered_multimap<K, V, Hash, Equal, Alloc> const& map)
	{
		os.write_array_header(map.size());
		for (auto it = map.begin(); it != map.end(); ++it)
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

#endif    // LOGICMILL_BSTREAM_STDLIB_UNORDERED_MAP_H