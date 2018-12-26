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

#ifndef LOGICMILL_BSTREAM_MEMORY_SOURCE_H
#define LOGICMILL_BSTREAM_MEMORY_SOURCE_H

#include <logicmill/bstream/buffer.h>
#include <logicmill/bstream/source.h>
#include <logicmill/bstream/types.h>
#include <deque>
#include <vector>
#include <algorithm>

namespace logicmill
{
namespace bstream
{
namespace compound_memory
{

namespace detail
{
template<class Buffer>
class source_test_probe;
}

template<class Buffer>
class source : public bstream::source
{
public:
	friend class detail::source_test_probe<Buffer>;

	using bstream::source::getn;
	using base = bstream::source;

	source(shared_buffer const& buf);

	source(shared_buffer&& buf);

	source(const_buffer const& buf) : base{}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
	{
		m_bufs.emplace(Buffer{buf});
	}

	source(const_buffer&& buf) : base{}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
	{
		m_bufs.emplace(Buffer{std::move(buf)});
	}

	source(mutable_buffer const& buf) : base{}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
	{
		m_bufs.emplace(Buffer{buf});
	}

	source(mutable_buffer&& buf) : base{}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
	{
		m_bufs.emplace(Buffer{std::move(buf)});
	}

	source(std::deque<shared_buffer> const& bufs);

	source(std::deque<shared_buffer>&& bufs);

	source(std::deque<const_buffer> const& bufs) : base{}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
	{
		size_type index = 0;
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			m_offsets[index++] = m_size;
			m_size += it->size();
			m_bufs.emplace_back(*it);
		}
		set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	source(std::deque<const_buffer>&& bufs) : base{}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
	{
		size_type index = 0;
		using iter_type = std::deque<const_buffer>::iterator;
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			std::move_iterator<iter_type> move_it{it};
			m_offsets[index++] = m_size;
			m_size += it->size();
			m_bufs.emplace_back(*move_it);
		}
		bufs.clear();
		set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	source(std::deque<mutable_buffer>&& bufs) : base{}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
	{
		size_type index = 0;
		using iter_type = std::deque<mutable_buffer>::iterator;
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			std::move_iterator<iter_type> move_it{it};
			m_offsets[index++] = m_size;
			m_size += it->size();
			m_bufs.emplace_back(*move_it);
		}
		bufs.clear();
		set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	// source(std::deque<buffer> const& bufs) : base{}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
	// {
	// 	std::copy(bufs.begin(); bufs.end(), m_bufs.begin());
	// 	size_type index = 0;
	// 	for (auto it = m_bufs.begin(); it != m_bufs.end(); ++it)
	// 	{
	// 		m_offsets[index++] = m_size;
	// 		m_size += it->size();
	// 	}
	// 	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
	// }

	// const_buffer
	// get_buffer()
	// {
	// 	return m_buf;
	// }

	std::deque<Buffer>&
	get_buffers_ref()
	{
		return m_bufs;
	}

	std::deque<Buffer>
	release_buffers()
	{
		m_size = 0;
		m_current = 0;
		m_offsets.clear();
		set_ptrs(nullptr, nullptr, nullptr);
		return std::move( m_bufs );
	}

	virtual shared_buffer
	get_shared_slice(size_type n, std::error_code& err) override;

	virtual shared_buffer
	get_shared_slice(size_type n) override;

	virtual const_buffer
	get_slice(size_type n, std::error_code& err) override;

	virtual const_buffer
	get_slice(size_type n) override;

	virtual size_type
	really_get_size() const override
	{
		return m_size;
	}

	virtual position_type
	really_seek(position_type pos, std::error_code& err) override
	{
		err.clear();
		size_type left{0};
		size_type right{m_bufs.size()};
		bool found{false};
		while (left <= right)
		{
			auto middle{(left + right) / 2};
			if (pos < m_offsets[middle])
			{
				right = middle - 1;
			}
			else if (pos > m_offsets[middle] + m_bufs[middle].size())
			{
				left = middle + 1;
			}
			else
			{
				m_current = middle;
				found = true;
			}
		}
		assert(found);
		m_base = m_bufs[m_current].data();
		m_end = m_base + m_bufs[m_current].size();
		m_base_offset = m_offsets[m_current];
		m_next = m_base + (pos - m_base_offset);
		return pos;
	}

	virtual position_type
	really_get_position() const override
	{
		return gpos();
	}

	virtual size_type
	really_underflow(std::error_code& err) override
	{
		err.clear();
		size_type result = 0;
		if (m_current < m_bufs.size() - 1)
		{
			++m_current;
			m_base = m_bufs[m_current].data();
			m_end = m_base + m_bufs[m_current].size();
			m_base_offset = m_offsets[m_current];
			m_next = m_base;
			result = m_bufs[m_current].size();
		}
		return result;
	}

	virtual void
	really_rewind() override
	{
		m_current = 0;
		m_base = m_bufs[m_current].data();
		m_end = m_base + m_bufs[m_current].size();
		m_base_offset = m_offsets[m_current];
		m_next = m_base;
	}

protected:
	std::deque<Buffer> m_bufs;
	size_type m_size;
	size_type m_current;
	std::vector<size_type> m_offsets;
};


template<>
inline source<const_buffer>::source(std::deque<shared_buffer> const& bufs)
	: base{}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
{
	size_type index = 0;
	for (auto it = bufs.begin(); it != bufs.end(); ++it)
	{
		m_offsets[index++] = m_size;
		m_size += it->size();
		m_bufs.emplace_back(*it);
	}
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline source<const_buffer>::source(std::deque<shared_buffer>&& bufs)
	: base{}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
	// construct by copy (as base type buffer); can't move shared_buffer to const_buffer
{
	size_type index = 0;
	for (auto it = bufs.begin(); it != bufs.end(); ++it)
	{
		m_offsets[index++] = m_size;
		m_size += it->size();
		m_bufs.emplace_back(*it);
	}
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline source<shared_buffer>::source(std::deque<shared_buffer> const& bufs)
	: base{}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
{
	size_type index = 0;
	for (auto it = bufs.begin(); it != bufs.end(); ++it)
	{
		m_offsets[index++] = m_size;
		m_size += it->size();
		m_bufs.emplace_back(*it);
	}
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline source<shared_buffer>::source(std::deque<shared_buffer>&& bufs) 
: base{}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
{
	size_type index = 0;
	using iter_type = std::deque<shared_buffer>::iterator;
	for (auto it = bufs.begin(); it != bufs.end(); ++it)
	{
		std::move_iterator<iter_type> move_it{it};
		m_offsets[index++] = m_size;
		m_size += it->size();
		m_bufs.emplace_back(*move_it);
	}
	bufs.clear();
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline source<shared_buffer>::source(shared_buffer const& buf)
: base{}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
{
	m_bufs.emplace_back(buf);
}

template<>
inline source<shared_buffer>::source(shared_buffer&& buf)
: base{}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
{
	m_bufs.emplace_back(std::move(buf));
}

template<>
inline source<const_buffer>::source(shared_buffer const& buf)
: base{}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
{
	m_bufs.emplace_back(const_buffer{buf});
}

template<>
inline source<const_buffer>::source(shared_buffer&& buf)
: base{}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
{
	m_bufs.emplace_back(const_buffer{buf});
}

template<>
inline shared_buffer
source<const_buffer>::get_shared_slice(size_type n, std::error_code& err)
{
	return base::get_shared_slice(n, err);
}

template<>
inline shared_buffer
source<const_buffer>::get_shared_slice(size_type n)
{
	return base::get_shared_slice(n);
}

template<>
inline shared_buffer
source<shared_buffer>::get_shared_slice(size_type n, std::error_code& err)
{
	if (n <= m_end - m_next)
	{
		shared_buffer result{m_bufs[m_current], static_cast<bstream::position_type>(m_next - m_base), n, err};
		if (err)
		{
			gbump(n);
		}
		return result;
	}
	else
	{
		return base::get_shared_slice(n, err);
	}
}

template<>
inline shared_buffer
source<shared_buffer>::get_shared_slice(size_type n)
{
	if (n <= m_end - m_next)
	{
		shared_buffer result{m_bufs[m_current], static_cast<bstream::position_type>(m_next - m_base), n};
		gbump(n);
		return result;
	}
	else
	{
		return base::get_shared_slice(n);
	}
}

template<>
inline const_buffer
source<const_buffer>::get_slice(size_type n, std::error_code& err)
{
	return base::get_shared_slice(n);
}

template<>
inline const_buffer
source<const_buffer>::get_slice(size_type n)
{
	return base::get_shared_slice(n);
}

template<>
inline const_buffer
source<shared_buffer>::get_slice(size_type n, std::error_code& err)
{
	return base::get_shared_slice(n);
}

template<>
inline const_buffer
source<shared_buffer>::get_slice(size_type n)
{
	return base::get_shared_slice(n);
}

}    // namespace memory
}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_MEMORY_SOURCE_H