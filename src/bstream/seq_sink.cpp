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

#include <logicmill/bstream/seq_sink.h>

using namespace logicmill;
using namespace bstream;
using namespace sequential;

bstream::sequential::sink::sink(byte_type* data, size_type size, byte_order order)
	: m_base_offset{0},
	  m_base{data},
	  m_next{data},
	  m_end{data + size},
	  m_dirty_start{data},
	  m_dirty{false},
	  m_reverse{is_reverse(order)}
{}

bstream::sequential::sink::sink(byte_order order)
	: m_base_offset{0},
	  m_base{nullptr},
	  m_next{nullptr},
	  m_end{nullptr},
	  m_dirty_start{nullptr},
	  m_dirty{false},
	  m_reverse{is_reverse(order)}
{}

bstream::sequential::sink::sink(sink&& rhs)
	: m_base_offset{rhs.m_base_offset},
	  m_base{rhs.m_base},
	  m_next{rhs.m_next},
	  m_end{rhs.m_end},
	  m_dirty_start{rhs.m_dirty_start},
	  m_dirty{rhs.m_dirty},
	  m_reverse{rhs.m_reverse}
{
	rhs.m_base_offset    = 0;
	rhs.m_base           = nullptr;
	rhs.m_next           = nullptr;
	rhs.m_end            = nullptr;
	rhs.m_dirty_start    = nullptr;
	rhs.m_dirty          = false;
}

void
bstream::sequential::sink::flush(std::error_code& err)
{
	err.clear();
	if (m_dirty)
	{
		really_flush(err);
		if (err)
			goto exit;
		m_dirty = false;
	}

exit:
	return;
}

void
bstream::sequential::sink::flush()
{
	if (m_dirty)
	{
		std::error_code err;
		really_flush(err);
		if (err)
		{
			throw std::system_error{err};
		}
		m_dirty = false;
	}
}

void
bstream::sequential::sink::put(byte_type byte, std::error_code& err)
{
	err.clear();
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
bstream::sequential::sink::put(byte_type byte)
{
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
bstream::sequential::sink::putn(const byte_type* src, size_type n, std::error_code& err)
{
	err.clear();
	if (n < 1)
		goto exit;
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
bstream::sequential::sink::putn(const byte_type* src, size_type n)
{
	if (n > 0)
	{
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
bstream::sequential::sink::filln(const byte_type fill_byte, size_type n, std::error_code& err)
{
	err.clear();
	if (n < 1)
		goto exit;
	really_fill(fill_byte, n);

exit:
	return;
}

void
bstream::sequential::sink::really_fill(byte_type fill_byte, size_type n)
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
bstream::sequential::sink::filln(const byte_type fill_byte, size_type n)
{
	if (n > 0)
	{
		really_fill(fill_byte, n);
	}
}


// size_type
// bstream::sequential::sink::size() const
// {
// 	// don't actually set_high_watermark here... at some point flush() will do that
// 	// note: this means that ppos() and size may exceed high watermark, but only if m_dirty is false
// 	return ( m_dirty && ( ppos() > get_high_watermark() ) ) ? ppos() : get_high_watermark();
// }


void
bstream::sequential::sink::overflow(size_type requested, std::error_code& err)
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
bstream::sequential::sink::overflow(size_type requested)
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
bstream::sequential::sink::really_overflow(size_type, std::error_code& err)
{
	err = make_error_code(std::errc::no_buffer_space);
}

void
bstream::sequential::sink::really_flush(std::error_code& err)
{
	err.clear();
	assert(m_dirty && m_next > m_dirty_start);
}

size_type
bstream::sequential::sink::really_get_size() const
{
	return ppos();
}
