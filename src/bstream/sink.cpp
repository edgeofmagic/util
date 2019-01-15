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

#include <logicmill/bstream/sink.h>

using namespace logicmill;
using namespace bstream;

bstream::sink::sink(byte_type* data, size_type size, byte_order order)
	: m_base_offset{0},
	  m_high_watermark{0},
	  m_jump_to{0},
	  m_base{data},
	  m_next{data},
	  m_end{data + size},
	  m_dirty_start{data},
	  m_dirty{false},
	  m_did_jump{false},
	  m_reverse{is_reverse(order)}
{}

bstream::sink::sink(byte_order order)
	: m_base_offset{0},
	  m_high_watermark{0},
	  m_jump_to{0},
	  m_base{nullptr},
	  m_next{nullptr},
	  m_end{nullptr},
	  m_dirty_start{nullptr},
	  m_dirty{false},
	  m_did_jump{false},
	  m_reverse{is_reverse(order)}
{}

bstream::sink::sink(sink&& rhs)
	: m_base_offset{rhs.m_base_offset},
	  m_high_watermark{rhs.m_high_watermark},
	  m_jump_to{rhs.m_jump_to},
	  m_base{rhs.m_base},
	  m_next{rhs.m_next},
	  m_end{rhs.m_end},
	  m_dirty_start{rhs.m_dirty_start},
	  m_dirty{rhs.m_dirty},
	  m_did_jump{rhs.m_did_jump},
	  m_reverse{rhs.m_reverse}
{
	rhs.m_base_offset    = 0;
	rhs.m_high_watermark = 0;
	rhs.m_jump_to        = 0;
	rhs.m_base           = nullptr;
	rhs.m_next           = nullptr;
	rhs.m_end            = nullptr;
	rhs.m_dirty_start    = nullptr;
	rhs.m_dirty          = false;
	rhs.m_did_jump       = false;
}

void
bstream::sink::flush(std::error_code& err)
{
	err.clear();
	if (m_dirty)
	{
		really_flush(err);
		if (err)
			goto exit;

		set_high_watermark();
		m_dirty = false;
	}

exit:
	return;
}

void
bstream::sink::flush()
{
	if (m_dirty)
	{
		std::error_code err;
		really_flush(err);
		if (err)
		{
			throw std::system_error{err};
		}
		set_high_watermark();
		m_dirty = false;
	}
}

void
bstream::sink::put(byte_type byte, std::error_code& err)
{
	err.clear();
	if (m_did_jump)
	{
		really_jump(err);
		if (err)
			goto exit;
	}
	if (m_base == nullptr || m_next >= m_end)
	{
		assert(m_next == m_end);
		overflow(1, err);
		if (err)
			goto exit;
		assert(!m_dirty);
	}
	if (!m_dirty)
	{
		m_dirty_start = m_next;
	}
	*m_next++ = byte;
	m_dirty   = true;

exit:
	return;
}

void
bstream::sink::put(byte_type byte)
{
	if (m_did_jump)
	{
		std::error_code err;
		really_jump(err);
		if (err)
			throw std::system_error{err};
	}
	if (m_base == nullptr || m_next >= m_end)
	{
		assert(m_next == m_end);
		overflow(1);
		assert(!m_dirty);
	}
	if (!m_dirty)
	{
		m_dirty_start = m_next;
		m_dirty       = true;
	}
	*m_next++ = byte;
}

void
bstream::sink::putn(const byte_type* src, size_type n, std::error_code& err)
{
	err.clear();
	if (n < 1)
		goto exit;

	if (m_did_jump)
	{
		really_jump(err);
		if (err)
			goto exit;
	}

	if (n <= static_cast<size_type>(m_end - m_next))    // optimize for common case ( no overflow )
	{
		if (!m_dirty)
		{
			m_dirty_start = m_next;
			m_dirty       = true;
		}
		::memcpy(m_next, src, n);
		m_next += n;
	}
	else
	{
		auto p     = src;
		auto limit = src + n;
		while (p < limit)
		{
			if (m_base == nullptr || m_next >= m_end)
			{
				assert(m_next == m_end);    // just checking
				overflow(limit - p, err);
				if (err)
					goto exit;
				assert(!m_dirty);
			}
			if (!m_dirty)
			{
				m_dirty_start = m_next;
				m_dirty       = true;
			}
			size_type chunk_size = std::min(static_cast<size_type>(m_end - m_next), static_cast<size_type>(limit - p));
			::memcpy(m_next, p, chunk_size);
			p += chunk_size;
			m_next += chunk_size;
		}
	}

exit:
	return;
}

void
bstream::sink::putn(const byte_type* src, size_type n)
{
	if (n > 0)
	{
		if (m_did_jump)
		{
			std::error_code err;
			really_jump(err);
			if (err)
				throw std::system_error{err};
		}

		if (n <= static_cast<size_type>(m_end - m_next))    // optimize for common case ( no overflow )
		{
			if (!m_dirty)
			{
				m_dirty_start = m_next;
				m_dirty       = true;
			}
			::memcpy(m_next, src, n);
			m_next += n;
		}
		else
		{
			auto p     = src;
			auto limit = src + n;
			while (p < limit)
			{
				if (m_base == nullptr || m_next >= m_end)
				{
					assert(m_next == m_end);    // just checking
					overflow(limit - p);
					assert(!m_dirty);
				}
				if (!m_dirty)
				{
					m_dirty_start = m_next;
					m_dirty       = true;
				}
				size_type chunk_size
						= std::min(static_cast<size_type>(m_end - m_next), static_cast<size_type>(limit - p));
				::memcpy(m_next, p, chunk_size);
				p += chunk_size;
				m_next += chunk_size;
			}
		}
	}
}


void
bstream::sink::filln(const byte_type fill_byte, size_type n, std::error_code& err)
{
	err.clear();
	if (n < 1)
		goto exit;

	if (m_did_jump)
	{
		really_jump(err);
		if (err)
			goto exit;
	}

	really_fill(fill_byte, n);

exit:
	return;
}

void
bstream::sink::really_fill(byte_type fill_byte, size_type n)
{
	assert(n > 0);
	if (n <= static_cast<size_type>(m_end - m_next))    // optimize for common case ( no overflow )
	{
		if (!m_dirty)
		{
			m_dirty_start = m_next;
			m_dirty       = true;
		}
		::memset(m_next, fill_byte, n);
		m_next += n;
	}
	else
	{
		size_type remaining = n;
		while (remaining > 0)
		{
			if (m_base == nullptr || m_next >= m_end)
			{
				assert(m_next == m_end);    // just checking
				overflow(remaining);
				assert(!m_dirty);
			}
			if (!m_dirty)
			{
				m_dirty_start = m_next;
				m_dirty       = true;
			}
			size_type chunk_size = std::min(static_cast<size_type>(m_end - m_next), remaining);
			::memset(m_next, fill_byte, chunk_size);
			remaining -= chunk_size;
			m_next += chunk_size;
		}
	}
}

void
bstream::sink::filln(const byte_type fill_byte, size_type n)
{
	if (n > 0)
	{
		if (m_did_jump)
		{
			std::error_code err;
			really_jump(err);
			if (err)
				throw std::system_error{err};
		}
		really_fill(fill_byte, n);
	}
}


// size_type
// bstream::sink::size() const
// {
// 	// don't actually set_high_watermark here... at some point flush() will do that
// 	// note: this means that ppos() and size may exceed high watermark, but only if m_dirty is false
// 	return ( m_dirty && ( ppos() > get_high_watermark() ) ) ? ppos() : get_high_watermark();
// }

position_type
bstream::sink::new_position(offset_type offset, seek_anchor where) const
{
	position_type result = npos;

	switch (where)
	{
		case seek_anchor::current:
		{
			result = ppos() + offset;
		}
		break;

		case seek_anchor::end:
		{
			result = size() + offset;
		}
		break;

		case seek_anchor::begin:
		{
			result = offset;
		}
		break;
	}

	return result;
}

position_type
bstream::sink::position(offset_type offset, seek_anchor where)
{
	auto new_pos = new_position(offset, where);

	if (!is_valid_position(new_pos))
	{
		throw std::system_error{make_error_code(std::errc::invalid_argument)};
	}

	if (new_pos != ppos())
	{
		if (m_dirty)
		{
			flush();
		}
		m_did_jump = true;
		m_jump_to  = new_pos;
	}

	return new_pos;
}

position_type
bstream::sink::position(offset_type offset, seek_anchor where, std::error_code& err)
{
	err.clear();

	auto new_pos = new_position(offset, where);

	if (!is_valid_position(new_pos))
	{
		err = make_error_code(std::errc::invalid_argument);
		goto exit;
	}

	if (new_pos != ppos())
	{
		if (m_dirty)
		{
			flush(err);
			if (err)
				goto exit;
		}
		m_did_jump = true;
		m_jump_to  = new_pos;
	}

exit:
	return new_pos;
}

position_type
bstream::sink::position() const
{
	return m_did_jump ? m_jump_to : ppos();
}

void
bstream::sink::overflow(size_type requested, std::error_code& err)
{
	flush(err);
	if (err)
		goto exit;

	really_overflow(requested, err);
	if (err)
		goto exit;

	assert(m_end > m_next);

exit:
	return;
}

void
bstream::sink::overflow(size_type requested)
{
	flush();
	std::error_code err;

	really_overflow(requested, err);
	if (err)
	{
		throw std::system_error{err};
	}

	assert(m_end > m_next);
}

void
bstream::sink::really_jump(std::error_code& err)
{
	err.clear();
	assert(m_did_jump);
	assert(is_valid_position(m_jump_to));
	assert(!m_dirty);    // previously flushed

	auto hwm = get_high_watermark();

	if (hwm < m_jump_to)
	{
		auto gap = m_jump_to - hwm;

		if (ppos() != hwm)
		{
			m_next = m_base + hwm;
		}

		really_fill(0, gap);

		assert(ppos() == m_jump_to);
	}
	else
	{
		m_next = m_base + m_jump_to;
		assert(ppos() == m_jump_to);
	}
	m_did_jump = false;
}

void
bstream::sink::really_overflow(size_type, std::error_code& err)
{
	err = make_error_code(std::errc::no_buffer_space);
}

void
bstream::sink::really_flush(std::error_code& err)
{
	err.clear();
	assert(m_dirty && m_next > m_dirty_start);
}

size_type
bstream::sink::really_get_size() const
{
	return (m_dirty && (ppos() > get_high_watermark())) ? ppos() : get_high_watermark();
}

bool
bstream::sink::is_valid_position(position_type pos) const
{
	return (pos >= 0) && (pos <= (m_end - m_base));
}
