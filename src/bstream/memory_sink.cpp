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

#include <logicmill/bstream/memory/sink.h>

using namespace logicmill;
using namespace bstream;

void
memory::sink::really_overflow(size_type n, std::error_code& err)
{
	err.clear();
	assert(std::less_equal<byte_type*>()(m_next, m_end));
	assert((m_next - m_base) + n > m_buf.size());
	auto      pos      = ppos();
	size_type required = (m_next - m_base) + n;
	resize(required, err);
	if (!err)
	{
		auto new_base = m_buf.data();
		set_ptrs(new_base, new_base + pos, new_base + m_buf.capacity());
	}
}

void
memory::sink::resize(size_type size, std::error_code& err)
{
	err.clear();
	size_type cushioned_size = (size * 3) / 2;

	if (!m_buf.is_expandable())
	{
		err = make_error_code(std::errc::no_buffer_space);
		goto exit;
	}

	// force a hard lower bound to avoid non-resizing dilemma in resizing, when cushioned == size == 1
	m_buf.expand(std::max(16UL, cushioned_size));

	if (m_buf.capacity() < cushioned_size)
	{
		err = make_error_code(std::errc::no_buffer_space);
		goto exit;
	}

exit:
	return;
}

bool
memory::sink::is_valid_position(position_type pos) const
{
	bool result = false;
	if (m_buf.is_expandable())
	{
		result = pos >= 0;
	}
	else
	{
		result = (pos >= 0) && (pos <= (m_end - m_base));
	}
	return result;
}

memory::sink&
memory::sink::clear() noexcept
{
	reset_ptrs();
	reset_high_water_mark();
	m_did_jump = false;
	m_dirty    = false;
	return *this;
}

const_buffer
memory::sink::get_buffer()
{
	if (m_dirty)
	{
		flush();
	}
	m_buf.size(get_high_watermark());
	return const_buffer{m_buf};
}

mutable_buffer&
memory::sink::get_buffer_ref()
{
	if (m_dirty)
	{
		flush();
	}
	m_buf.size(get_high_watermark());
	return m_buf;
}

const_buffer
memory::sink::release_buffer()
{
	if (m_dirty)
	{
		flush();
	}
	m_buf.size(get_high_watermark());
	reset_high_water_mark();
	m_did_jump = false;
	m_dirty    = false;
	set_ptrs(nullptr, nullptr, nullptr);
	return const_buffer{std::move(m_buf)};
}

mutable_buffer
memory::sink::release_mutable_buffer()
{
	if (m_dirty)
	{
		flush();
	}
	m_buf.size(get_high_watermark());
	reset_high_water_mark();
	m_did_jump = false;
	m_dirty    = false;
	set_ptrs(nullptr, nullptr, nullptr);
	return std::move(m_buf);
}
