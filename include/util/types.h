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

#ifndef UTIL_TYPES_H
#define UTIL_TYPES_H

#include <cstdint>
#include <limits>

namespace util
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

}    // namespace util

#endif    // UTIL_TYPES_H