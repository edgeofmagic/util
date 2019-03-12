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

#include <boost/crc.hpp>
#include <logicmill/util/buffer.h>
#include <logicmill/util/dumpster.h>

using namespace logicmill;
using namespace util;

buffer::checksum_type
buffer::checksum(size_type offset, size_type length) const
{

	boost::crc_32_type crc;
	if ((m_data != nullptr) && (offset < length) && ((offset + length) <= m_size))
	{
		crc.process_bytes(m_data + offset, length);
	}
	return crc.checksum();
}

void
buffer::dump(std::ostream& os) const
{
	util::dumpster{}.dump(os, m_data, m_size);
}

mutable_buffer::mutable_buffer(const_buffer&& cbuf)
	: buffer{cbuf.m_data, cbuf.m_size},
	  m_region{std::move(cbuf.m_region)},
	  m_capacity{static_cast<size_type>((m_region->data() + m_region->capacity()) - m_data)}
{
	cbuf.m_data = nullptr;
	cbuf.m_size = 0;
	ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
}

mutable_buffer::mutable_buffer(const_buffer&& rhs, size_type offset, size_type length)
	: m_region{std::move(rhs.m_region)}
{
	if (offset + length > rhs.m_size)
	{
		throw std::system_error{make_error_code(std::errc::invalid_argument)};
	}
	m_data     = rhs.m_data + offset;
	m_size     = length;
	m_capacity = (m_region->data() + m_region->capacity()) - m_data;
	rhs.m_data = nullptr;
	rhs.m_size = 0;
}

mutable_buffer::mutable_buffer(const_buffer&& rhs, size_type offset, size_type length, std::error_code& err)
	: m_region{std::move(rhs.m_region)}
{
	if (offset + length > rhs.m_size)
	{
		err = make_error_code(std::errc::invalid_argument);
		goto exit;
	}
	m_data     = rhs.m_data + offset;
	m_size     = length;
	m_capacity = (m_region->data() + m_region->capacity()) - m_data;
	rhs.m_data = nullptr;
	rhs.m_size = 0;

exit:
	return;
}
