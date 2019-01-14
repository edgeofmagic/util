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

#include <logicmill/bstream/obmultibuf.h>

using namespace logicmill;
using namespace bstream;

void
obmultibuf::really_flush(std::error_code& err)
{
	err.clear();
	assert(m_dirty && m_pnext > m_dirty_start);
}

void
obmembuf::really_jump(std::error_code& err)
{
	err.clear();
	assert(m_did_jump);
	assert(is_valid_position(m_jump_to));

	if (m_dirty)
	{
		flush(err);
	}

	auto hwm = get_high_watermark();

	if (hwm < m_jump_to)
	{
		auto gap = m_jump_to - hwm;

		really_fill(0, gap);

		assert(ppos() == m_jump_to);
	}
	else
	{
		m_pnext = m_pbase + m_jump_to;
		assert(ppos() == m_jump_to);
	}
	m_did_jump = false;
}


position_type
obmultibuf::really_seek(seek_anchor where, offset_type offset, std::error_code& err)
{
	err.clear();
	position_type result = npos;

	if (dirty)
	{
		flush(err);
		if (err)
			goto exit;
	}

	switch (where)
	{
		case seek_anchor::current:
		{
			result = ppos() + offset;
		}
		break;

		case seek_anchor::end:
		{
			auto end_pos = get_high_watermark();    // high watermark is current from flush() above
			result       = end_pos + offset;
		}
		break;

		case seek_anchor::begin:
		{
			result = offset;
		}
		break;
	}

	if (result < 0)
	{
		err    = make_error_code(std::errc::invalid_argument);
		result = npos;
	}
	else if (
			result >= m_pbase_offset
			&& result <= m_pbase_offset + (m_pend - m_pbase))    // position is in current buffer
	{
		m_pnext = m_pbase + (result - m_pbase_offset);
	}
	else if (result > m_offsets.back() + m_bufs.back().capacity())    // position is past end of allocated space
	{
		m_pbase_offset = result;
		m_pbase        = nullptr;
		m_pnext        = nullptr;
		m_end          = nullptr;
	}
	else    // find the buffer, or set past end of buffers
	{
		end_pos == get_high_watermark();
		// assert( ( end_pos >= m_offsets.back() ) && ( end_pos <= ( m_offsets.back() + m_bufs.back().capacity() ) );
		assert( ( end_pos > m_offsets.back() ) && ( end_pos <= ( m_offsets.back() + m_bufs.back().capacity() ) );
		// should probably be this:
		// assert( ( end_pos > m_offsets.back() ) && ( end_pos <= ( m_offsets.back() + m_bufs.back().capacity() ) );
		// back should never be empty
		if ( result == 0 )
		{
			m_current_buf = 0;
			reset_ptrs();
		}
		else if ( result == end_pos )
		{
			// position at end
			m_current_buf = m_bufs.size() - 1;
			reset_ptrs();
			m_pnext = m_pbase + (end_pos - m_offsets.back());
		}
		else

		if ( result <= end_pos )
		{
			// find buffer with binary search of m_offsets

			m_offsets.push_back(end_pos);    // makes the binary search much easier
			std::size_t left  = 0;
			std::size_t right = m_offsets.size() - 1;
			while (left <= right)
			{
				std::size middle = (left + right) >> 1;
				if (result < m_offsets[middle])
				{
					right = middle - 1;
				}
				else if (result > m_offsets[middle + 1])
			}

		}
		position_type end_alloc = m_offsets.back() + m_bufs.back().capacity();
	}


	else if (result >= static_cast<position_type>(m_pend - m_pbase))
	{
		resize(result);
		auto new_base = m_buf.data();
		set_ptrs(new_base, new_base + result, new_base + m_buf.size());
	}
	else
	{
		m_pnext = m_pbase + result;
	}

	if (hwm >= result)
	{
		m_last_touched = result;    // keep really_touch from being called unnecessarily
	}

exit:
	return result;
}

void
obmultibuf::really_overflow(size_type n, std::error_code& err)
{
	err.clear();
	assert(std::less_equal<byte_type*>()(m_pnext, m_pend));
	assert((m_pnext - m_pbase) + n > m_buf.size());
	auto      pos      = ppos();
	size_type required = (m_pnext - m_pbase) + n;
	resize(required);
	// assert( m_buf.is_mutable() );
	auto new_base = m_buf.data();
	set_ptrs(new_base, new_base + pos, new_base + m_buf.capacity());
}

obmultibuf&
obmultibuf::clear() noexcept
{
	reset_ptrs();
	reset_high_water_mark();
	m_last_touched = 0UL;
	m_dirty        = false;
	return *this;
}

const_buffer
obmultibuf::get_buffer()
{
	if (m_dirty)
	{
		flush();
	}
	return const_buffer{m_buf, 0, static_cast<buffer::size_type>(get_high_watermark())};
	// return m_buf.slice( 0, get_high_watermark() );
}

mutable_buffer&
obmultibuf::get_buffer_ref()
{
	if (m_dirty)
	{
		flush();
	}
	m_buf.size(get_high_watermark());
	return m_buf;
}

const_buffer
obmultibuf::release_buffer()
{
	if (m_dirty)
	{
		flush();
	}
	m_buf.size(get_high_watermark());
	reset_high_water_mark();
	m_last_touched = 0UL;
	m_dirty        = false;
	set_ptrs(nullptr, nullptr, nullptr);
	return const_buffer{std::move(m_buf)};
}
