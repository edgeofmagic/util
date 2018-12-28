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

#ifndef LOGICMILL_BSTREAM_TYPES_H
#define LOGICMILL_BSTREAM_TYPES_H

#include <cstdint>
#include <limits>
#include <system_error>

namespace logicmill
{
namespace bstream
{

/** \brief The type used to represent a single byte in a stream or buffer.
 */
using byte_type = std::uint8_t;

/** \brief The type used to represent the size or capacity of a stream or buffer.
 */
using size_type = std::size_t;

/** \brief The type used to represent the absolute position of a byte in 
 * in a stream or buffer.
 */
using position_type = std::uint64_t;

/** \brief The type used to represent the a relative offset from an 
 * absolute position in a stream or buffer.
 */
using offset_type = std::int64_t;

/** \brief The type used to represent a checksum value calculated for
 * a buffer.
 */
using checksum_type = std::uint32_t;

/** \brief A value representing an invalid size.
 */
static constexpr size_type npos = std::numeric_limits<size_type>::max();


// using size_type = std::size_t;
// using position_type = std::int64_t;
// using byte_type = std::uint8_t;
using poly_tag_type = int;

static constexpr poly_tag_type invalid_tag = -1;

enum class seek_anchor
{
	begin,
	current,
	end
};

enum class open_mode
{
	truncate,
	append,
	at_end,
	at_begin,
};

struct as_shared_buffer
{};
struct as_const_buffer
{};
struct as_mutable_buffer
{};

namespace detail
{

template<int N>
struct canonical_type;

template<>
struct canonical_type<1>
{
	using type = std::uint8_t;
};

template<>
struct canonical_type<2>
{
	using type = std::uint16_t;
};

template<>
struct canonical_type<4>
{
	using type = std::uint32_t;
};

template<>
struct canonical_type<8>
{
	using type = std::uint64_t;
};

}    // namespace detail

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_TYPES_H