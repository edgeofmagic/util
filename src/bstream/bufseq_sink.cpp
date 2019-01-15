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

#include <logicmill/bstream/bufseq/sink.h>

using namespace logicmill;
using namespace bstream;

void
bufseq::sink::really_overflow(size_type n, std::error_code& err)
{
	err.clear();
	if (m_base == nullptr)
	{
		assert(m_bufs.size() == 0);
		assert(m_current == 0);
		assert(get_high_watermark() == 0);
		assert(m_did_jump = false);
		assert(m_dirty = false);
		assert(m_base_offset = 0);
		// m_bufs.emplace_back( util::mutable_buffer{ m_segment_capacity, m_broker } );
		m_bufs.emplace_back(m_factory->create(m_segment_capacity));
		reset_ptrs();
	}
	assert(ppos() == (m_current + 1) * m_segment_capacity);
	m_bufs[m_current].size(m_segment_capacity);
	if (m_current == m_bufs.size() - 1)    // last buffer in deque, extend deque
	{
		m_bufs.emplace_back(m_factory->create(m_segment_capacity));
	}
	else
	{
		assert(get_high_watermark() > ppos());
	}
	++m_current;
	reset_ptrs();
}

bool
bufseq::sink::is_valid_position(position_type pos) const
{
	return pos >= 0;
}

bufseq::sink&
bufseq::sink::clear() noexcept
{
	reset_high_water_mark();
	m_bufs.resize(1);
	m_current = 0;
	m_bufs[m_current].size(0);
	reset_ptrs();
	m_did_jump = false;
	m_dirty    = false;
	return *this;
}

bufseq::sink::buffers&
bufseq::sink::get_buffers()
{
	if (m_dirty)
	{
		flush();
	}
	set_size();
	return m_bufs;
}

void
bufseq::sink::set_size()
{
	auto hwm = get_high_watermark();
	if (hwm == 0)
	{
		assert(m_bufs.size() == 1);
		assert(m_current == 0);
		m_bufs[m_current].size(0);
	}
	else
	{
		auto hwm_segment = hwm / m_segment_capacity;
		auto hwm_seg_pos = hwm % m_segment_capacity;
		if (hwm_seg_pos == 0)
		{
			assert(hwm_segment > 0);
			// set ptrs to end of previous buffer; will immediately overflow
			--hwm_segment;
			hwm_seg_pos = m_segment_capacity;
		}
		assert(hwm_segment == m_bufs.size() - 1);
		assert(hwm_seg_pos > 0 && hwm_seg_pos <= m_segment_capacity);

		m_bufs[hwm_segment].size(hwm_seg_pos);
	}
}


bufseq::sink::buffers
bufseq::sink::release_buffers()
{
	if (m_dirty)
	{
		flush();
	}

	set_size();

	reset_high_water_mark();
	m_did_jump    = false;
	m_dirty       = false;
	m_base_offset = 0;
	m_current     = 0;
	set_ptrs(nullptr, nullptr, nullptr);
	return std::move(m_bufs);
}

void
bufseq::sink::really_jump(std::error_code& err)
{
	err.clear();
	assert(m_did_jump);
	assert(is_valid_position(m_jump_to));
	assert(!m_dirty);

	auto hwm = get_high_watermark();

	if (hwm < m_jump_to)
	{
		auto gap = m_jump_to - hwm;

		if (ppos() != hwm)
		{
			locate(hwm, err);
			if (err)
				goto exit;
		}

		really_fill(0, gap);
	}
	else
	{
		locate(m_jump_to, err);
		if (err)
			goto exit;
	}

	assert(ppos() == m_jump_to);

exit:
	m_did_jump = false;
}

void
bufseq::sink::locate(position_type pos, std::error_code& err)
{
	err.clear();
	assert(pos <= get_high_watermark());
	if (pos == 0)
	{
		assert(m_bufs.size() == 1);
		m_current = 0;
		reset_ptrs();
	}
	else
	{
		auto new_current = pos / m_segment_capacity;
		auto seg_pos     = pos % m_segment_capacity;
		if (seg_pos == 0)
		{
			assert(new_current > 0);
			// set ptrs to end of previous buffer; will immediately overflow
			--new_current;
			seg_pos = m_segment_capacity;
		}
		if (new_current >= m_bufs.size())
		{
			err = make_error_code(std::errc::invalid_argument);
		}
		else
		{
			m_current     = new_current;
			m_base_offset = m_current * m_segment_capacity;
			auto new_base = m_bufs[m_current].data();
			set_ptrs(new_base, new_base + seg_pos, new_base + m_segment_capacity);
		}
	}
}
