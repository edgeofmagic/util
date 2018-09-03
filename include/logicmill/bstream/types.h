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
#include <system_error>

namespace logicmill 
{
namespace bstream
{

    using size_type = std::size_t;
    using position_type = std::int64_t;
    using offset_type  = std::int64_t;
    using byte_type = std::uint8_t;
    using poly_tag_type = int;

    static constexpr poly_tag_type invalid_tag = -1;
    static constexpr position_type invalid_position = -1;

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

	inline void
	clear_error( std::error_code& ec )
	{
		static const std::error_code good = std::error_code{};
		ec = good;
	}


} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_TYPES_H