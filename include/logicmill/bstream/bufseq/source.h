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

#ifndef LOGICMILL_BSTREAM_BUFSEQ_SOURCE_H
#define LOGICMILL_BSTREAM_BUFSEQ_SOURCE_H

#include <algorithm>
#include <deque>
#include <logicmill/bstream/error.h>
#include <logicmill/bstream/source.h>
#include <logicmill/bstream/types.h>
#include <logicmill/util/buffer.h>
#include <vector>

namespace logicmill
{
namespace bstream
{
namespace bufseq
{

namespace detail
{
template<class Buffer>
class source_test_probe;
}

template<class Buffer>
class source : public bstream::source
{
private:
	struct signature_enable
	{};

public:
	using bstream::source::getn;
	using bstream::source::get_num;
	using base        = bstream::source;
	using buffer_type = Buffer;

	friend class detail::source_test_probe<Buffer>;

	source(byte_order order = byte_order::big_endian) : base{order}, m_bufs{}, m_size{0}, m_current{0}, m_offsets{} {}

	source(util::shared_buffer const& buf, byte_order order = byte_order::big_endian);

	source(util::shared_buffer&& buf, byte_order order = byte_order::big_endian);

	source(util::const_buffer const& buf, byte_order order = byte_order::big_endian)
		: base{order}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
	{
		m_bufs.emplace(Buffer{buf});
	}

	source(util::const_buffer&& buf, byte_order order = byte_order::big_endian)
		: base{order}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
	{
		m_bufs.emplace(Buffer{std::move(buf)});
	}

	source(util::mutable_buffer const& buf, byte_order order = byte_order::big_endian)
		: base{order}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
	{
		m_bufs.emplace(Buffer{buf});
	}

	source(util::mutable_buffer&& buf, byte_order order = byte_order::big_endian)
		: base{order}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
	{
		m_bufs.emplace(Buffer{std::move(buf)});
	}

	source(std::deque<util::shared_buffer> const& bufs, byte_order order = byte_order::big_endian);

	source(std::deque<util::shared_buffer>&& bufs, byte_order order = byte_order::big_endian);

	source(std::deque<util::const_buffer> const& bufs, byte_order order = byte_order::big_endian)
		: base{order}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
	{
		size_type index = 0;
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			m_offsets[index++] = m_size;
			m_size += it->size();
			m_bufs.emplace_back(*it);
		}
		set_ptrs(
				m_bufs[m_current].data(),
				m_bufs[m_current].data(),
				m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	source(std::deque<util::const_buffer>&& bufs, byte_order order = byte_order::big_endian)
		: base{order}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
	{
		size_type index = 0;
		using iter_type = std::deque<util::const_buffer>::iterator;
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			std::move_iterator<iter_type> move_it{it};
			m_offsets[index++] = m_size;
			m_size += it->size();
			m_bufs.emplace_back(*move_it);
		}
		bufs.clear();
		set_ptrs(
				m_bufs[m_current].data(),
				m_bufs[m_current].data(),
				m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	source(std::deque<util::mutable_buffer>&& bufs, byte_order order = byte_order::big_endian)
		: base{order}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
	{
		size_type index = 0;
		using iter_type = std::deque<util::mutable_buffer>::iterator;
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			std::move_iterator<iter_type> move_it{it};
			m_offsets[index++] = m_size;
			m_size += it->size();
			m_bufs.emplace_back(*move_it);
		}
		bufs.clear();
		set_ptrs(
				m_bufs[m_current].data(),
				m_bufs[m_current].data(),
				m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	void
	use(std::deque<util::mutable_buffer>&& bufs)
	{
		m_bufs.clear();
		m_offsets.clear();
		m_size    = 0;
		m_current = 0;
		m_offsets.resize(bufs.size(), 0);

		size_type index = 0;
		using iter_type = std::deque<util::mutable_buffer>::iterator;
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			std::move_iterator<iter_type> move_it{it};
			m_offsets[index++] = m_size;
			m_size += it->size();
			m_bufs.emplace_back(*move_it);
		}
		bufs.clear();
		set_ptrs(
				m_bufs[m_current].data(),
				m_bufs[m_current].data(),
				m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	void
	use(std::deque<util::const_buffer>&& bufs)
	{
		m_bufs.clear();
		m_offsets.clear();
		m_size    = 0;
		m_current = 0;
		m_offsets.resize(bufs.size(), 0);

		size_type index = 0;
		using iter_type = std::deque<util::const_buffer>::iterator;
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			std::move_iterator<iter_type> move_it{it};
			m_offsets[index++] = m_size;
			m_size += it->size();
			m_bufs.emplace_back(*move_it);
		}
		bufs.clear();
		set_ptrs(
				m_bufs[m_current].data(),
				m_bufs[m_current].data(),
				m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	void
	use(std::deque<util::shared_buffer>&& bufs);

	void
	use(std::deque<util::shared_buffer> const& bufs);    // TODO: here

	void
	use(util::mutable_buffer&& buf)
	{
		m_bufs.clear();
		m_offsets.clear();
		m_size    = buf.size();
		m_current = 0;
		m_offsets.resize(1, 0);
		m_bufs.emplace_back(std::move(buf));
		set_ptrs(
				m_bufs[m_current].data(),
				m_bufs[m_current].data(),
				m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	void
	use(util::const_buffer&& buf)
	{
		m_bufs.clear();
		m_offsets.clear();
		m_size    = buf.size();
		m_current = 0;
		m_offsets.resize(1, 0);
		m_bufs.emplace_back(std::move(buf));
		set_ptrs(
				m_bufs[m_current].data(),
				m_bufs[m_current].data(),
				m_bufs[m_current].data() + m_bufs[m_current].size());
	}

	void
	use(util::shared_buffer&& buf);


	void
	trim()
	{
		while (m_current > 0)
		{
			m_bufs.pop_front();
			m_offsets.pop_front();
			--m_current;
		}
		assert(m_current == 0);
		refresh_offsets();
	}

	void
	append(std::deque<util::shared_buffer>&& bufs);

	void
	append(std::deque<util::shared_buffer> const& bufs);

	void
	append(std::deque<util::const_buffer>&& bufs)
	{
		if (m_bufs.empty())
		{
			use(std::move(bufs));
		}
		else
		{
			for (auto it = bufs.begin(); it != bufs.end(); ++it)
			{
				std::move_iterator<std::deque<util::const_buffer>::iterator> mit{it};
				m_offsets.push_back(m_size);
				m_size += it->size();
				m_bufs.emplace_back(*mit);
			}
			bufs.clear();
		}
	}

	void
	append(std::deque<util::mutable_buffer>&& bufs)
	{
		if (m_bufs.empty())
		{
			use(std::move(bufs));
		}
		else
		{
			for (auto it = bufs.begin(); it != bufs.end(); ++it)
			{
				std::move_iterator<std::deque<util::mutable_buffer>::iterator> mit{it};
				m_offsets.push_back(m_size);
				m_size += it->size();
				m_bufs.emplace_back(*mit);
			}
			bufs.clear();
		}
	}

	void
	append(util::shared_buffer&& buf);

	void
	append(util::shared_buffer const& buf);

	void
	append(util::const_buffer&& buf)
	{
		if (m_bufs.empty())
		{
			use(std::move(buf));
		}
		else
		{
			m_offsets.push_back(m_size);
			m_size += buf.size();
			m_bufs.emplace_back(std::move(buf));
		}
	}

	void
	append(util::mutable_buffer&& buf)
	{
		if (m_bufs.empty())
		{
			use(std::move(buf));
		}
		else
		{
			m_offsets.push_back(m_size);
			m_size += buf.size();
			m_bufs.emplace_back(std::move(buf));
		}
	}

	void
	refresh_offsets()
	{
		size_type index = 0;
		size_type size  = 0;
		using iter_type = typename std::deque<buffer_type>::iterator;
		for (auto it = m_bufs.begin(); it != m_bufs.end(); ++it)
		{
			m_offsets[index++] = size;
			size += it->size();
		}
		m_base_offset = m_offsets[m_current];
		m_size        = size;
	}

	std::deque<Buffer>&
	get_buffers_ref()
	{
		return m_bufs;
	}

	std::deque<Buffer>
	release_buffers()
	{
		m_size    = 0;
		m_current = 0;
		m_offsets.clear();
		set_ptrs(nullptr, nullptr, nullptr);
		return std::move(m_bufs);
	}

	virtual util::shared_buffer
	get_shared_slice(size_type n, std::error_code& err) override;

	virtual util::shared_buffer
	get_shared_slice(size_type n) override;

	virtual util::const_buffer
	get_slice(size_type n, std::error_code& err) override;

	virtual util::const_buffer
	get_slice(size_type n) override;

	std::deque<util::shared_buffer>
	get_segmented_slice(size_type n);

	std::deque<util::shared_buffer>
	get_segmented_slice(size_type n, std::error_code& err);

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
		bool      found{false};
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
				found     = true;
				break;
			}
		}
		assert(found);
		m_base        = m_bufs[m_current].data();
		m_end         = m_base + m_bufs[m_current].size();
		m_base_offset = m_offsets[m_current];
		m_next        = m_base + (pos - m_base_offset);
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
			m_base        = m_bufs[m_current].data();
			m_end         = m_base + m_bufs[m_current].size();
			m_base_offset = m_offsets[m_current];
			m_next        = m_base;
			result        = m_bufs[m_current].size();
		}
		return result;
	}

	virtual void
	really_rewind() override
	{
		m_current     = 0;
		m_base        = m_bufs[m_current].data();
		m_end         = m_base + m_bufs[m_current].size();
		m_base_offset = m_offsets[m_current];
		m_next        = m_base;
	}

protected:
	std::deque<Buffer>    m_bufs;
	size_type             m_size;
	size_type             m_current;
	std::deque<size_type> m_offsets;
};


template<>
inline source<util::const_buffer>::source(std::deque<util::shared_buffer> const& bufs, byte_order order)
	: base{order}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
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
inline source<util::const_buffer>::source(std::deque<util::shared_buffer>&& bufs, byte_order order)
	: base{order}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
// construct by copy (as base type buffer); can't move util::shared_buffer to util::const_buffer
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
inline source<util::shared_buffer>::source(std::deque<util::shared_buffer> const& bufs, byte_order order)
	: base{order}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
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
inline source<util::shared_buffer>::source(std::deque<util::shared_buffer>&& bufs, byte_order order)
	: base{order}, m_bufs{}, m_size{0}, m_current{0}, m_offsets(bufs.size(), 0)
{
	size_type index = 0;
	using iter_type = std::deque<util::shared_buffer>::iterator;
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
inline source<util::shared_buffer>::source(util::shared_buffer const& buf, byte_order order)
	: base{order}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
{
	m_bufs.emplace_back(buf);
}

template<>
inline source<util::shared_buffer>::source(util::shared_buffer&& buf, byte_order order)
	: base{order}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
{
	m_bufs.emplace_back(std::move(buf));
}

template<>
inline source<util::const_buffer>::source(util::shared_buffer const& buf, byte_order order)
	: base{order}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
{
	m_bufs.emplace_back(util::const_buffer{buf});
}

template<>
inline source<util::const_buffer>::source(util::shared_buffer&& buf, byte_order order)
	: base{order}, m_bufs{}, m_size{buf.size()}, m_current{0}, m_offsets(1, 0)
{
	m_bufs.emplace_back(util::const_buffer{buf});
}

template<>
inline util::shared_buffer
source<util::const_buffer>::get_shared_slice(size_type n, std::error_code& err)
{
	return base::get_shared_slice(n, err);
}

template<>
inline util::shared_buffer
source<util::const_buffer>::get_shared_slice(size_type n)
{
	return base::get_shared_slice(n);
}

template<>
inline util::shared_buffer
source<util::shared_buffer>::get_shared_slice(size_type n, std::error_code& err)
{
	err.clear();
	util::shared_buffer result;

	if (n < 1)
	{
		goto exit;
	}

	// If at the end of the segment and more is available, underflow
	if (m_next == m_end && m_current < m_bufs.size() - 1)
	{
		std::error_code err;
		auto            available = really_underflow(err);
		if (available < 1)
		{
			err = make_error_code(bstream::errc::read_past_end_of_stream);
			goto exit;
		}
	}

	if (n <= m_end - m_next)
	{
		result = util::shared_buffer{m_bufs[m_current], static_cast<position_type>(m_next - m_base), n, err};
		if (!err)
		{
			gbump(n);
		}
	}
	else
	{
		result = base::get_shared_slice(n, err);
	}

exit:
	return result;
}

template<>
inline util::shared_buffer
source<util::shared_buffer>::get_shared_slice(size_type n)
{
	util::shared_buffer result;

	if (n > 0)
	{
		// If at the end of the segment and more is available, underflow
		if (m_next == m_end && n > 0 && m_current < m_bufs.size() - 1)
		{
			std::error_code err;
			auto            available = really_underflow(err);
			if (err)
			{
				throw std::system_error{err};
			}
			if (available < 1)
			{
				throw std::system_error{make_error_code(bstream::errc::read_past_end_of_stream)};
			}
		}

		if (n <= m_end - m_next)
		{
			result = util::shared_buffer{m_bufs[m_current], static_cast<position_type>(m_next - m_base), n};
			gbump(n);
		}
		else
		{
			result = base::get_shared_slice(n);
		}
	}
	return result;
}

template<>
inline util::const_buffer
source<util::const_buffer>::get_slice(size_type n, std::error_code& err)
{
	return base::get_slice(n, err);
}

template<>
inline util::const_buffer
source<util::const_buffer>::get_slice(size_type n)
{
	return base::get_slice(n);
}

template<>
inline util::const_buffer
source<util::shared_buffer>::get_slice(size_type n, std::error_code& err)
{
	return base::get_slice(n, err);
}

template<>
inline util::const_buffer
source<util::shared_buffer>::get_slice(size_type n)
{
	return base::get_slice(n);
}

template<>
inline std::deque<util::shared_buffer>
source<util::shared_buffer>::get_segmented_slice(size_type n, std::error_code& err)
{
	err.clear();
	std::deque<util::shared_buffer> result;

	if (n < 1)
		goto exit;

	if (n > remaining())
	{
		throw std::system_error{make_error_code(bstream::errc::read_past_end_of_stream)};
	}

	if (m_next == m_end)
	{
		assert(m_current < m_bufs.size() - 1);
		assert(remaining() > 0);
		auto available = really_underflow(err);
		if (err)
		{
			throw std::system_error{err};
		}
		if (available < 1)    //  shouldn't happen
		{
			throw std::system_error{make_error_code(bstream::errc::read_past_end_of_stream)};
		}
	}
	while (n > 0)
	{
		auto grab = std::min(n, available());
		result.emplace_back(get_shared_slice(grab, err));
		if (err)
			goto exit;
		n -= grab;
	}
exit:
	return result;
}

template<>
inline std::deque<util::shared_buffer>
source<util::shared_buffer>::get_segmented_slice(size_type n)
{
	std::deque<util::shared_buffer> result;
	std::error_code                 err;
	result = get_segmented_slice(n, err);
	if (err)
	{
		throw std::system_error{err};
	}
	return result;
}

template<>
inline std::deque<util::shared_buffer>
source<util::const_buffer>::get_segmented_slice(size_type n)
{
	std::deque<util::shared_buffer> result;
	result.emplace_back(util::shared_buffer{base::get_slice(n)});
	return result;
}

template<>
inline std::deque<util::shared_buffer>
source<util::const_buffer>::get_segmented_slice(size_type n, std::error_code& err)
{
	std::deque<util::shared_buffer> result;
	result.emplace_back(util::shared_buffer{base::get_slice(n, err)});
	return result;
}

template<>
inline std::deque<util::shared_buffer>
source<util::mutable_buffer>::get_segmented_slice(size_type n)
{
	std::deque<util::shared_buffer> result;
	result.emplace_back(util::shared_buffer{base::get_slice(n)});
	return result;
}

template<>
inline std::deque<util::shared_buffer>
source<util::mutable_buffer>::get_segmented_slice(size_type n, std::error_code& err)
{
	std::deque<util::shared_buffer> result;
	result.emplace_back(util::shared_buffer{base::get_slice(n, err)});
	return result;
}


template<>
inline void
source<util::mutable_buffer>::use(std::deque<util::shared_buffer>&& bufs)
{
	m_bufs.clear();
	m_offsets.clear();
	m_size    = 0;
	m_current = 0;
	m_offsets.resize(bufs.size(), 0);

	size_type index = 0;
	for (auto it = bufs.begin(); it != bufs.end(); ++it)
	{
		m_offsets[index++] = m_size;
		m_size += it->size();
		m_bufs.emplace_back(*it);
	}
	bufs.clear();
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline void
source<util::const_buffer>::use(std::deque<util::shared_buffer>&& bufs)
{
	m_bufs.clear();
	m_offsets.clear();
	m_size    = 0;
	m_current = 0;
	m_offsets.resize(bufs.size(), 0);

	size_type index = 0;
	for (auto it = bufs.begin(); it != bufs.end(); ++it)
	{
		m_offsets[index++] = m_size;
		m_size += it->size();
		m_bufs.emplace_back(*it);
	}
	bufs.clear();
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline void
source<util::shared_buffer>::use(std::deque<util::shared_buffer>&& bufs)
{
	m_bufs.clear();
	m_offsets.clear();
	m_size    = 0;
	m_current = 0;
	m_offsets.resize(bufs.size(), 0);

	size_type index = 0;
	using iter_type = std::deque<util::shared_buffer>::iterator;
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
inline void
source<util::mutable_buffer>::use(util::shared_buffer&& buf)
{
	m_bufs.clear();
	m_offsets.clear();
	m_size    = buf.size();
	m_current = 0;
	m_offsets.resize(1, 0);
	m_bufs.emplace_back(buf);
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline void
source<util::const_buffer>::use(util::shared_buffer&& buf)
{
	m_bufs.clear();
	m_offsets.clear();
	m_size    = buf.size();
	m_current = 0;
	m_offsets.resize(1, 0);
	m_bufs.emplace_back(buf);
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline void
source<util::shared_buffer>::use(util::shared_buffer&& buf)
{
	m_bufs.clear();
	m_offsets.clear();
	m_size    = buf.size();
	m_current = 0;
	m_offsets.resize(1, 0);
	m_bufs.emplace_back(std::move(buf));
	set_ptrs(m_bufs[m_current].data(), m_bufs[m_current].data(), m_bufs[m_current].data() + m_bufs[m_current].size());
}

template<>
inline void
source<util::shared_buffer>::append(std::deque<util::shared_buffer>&& bufs)
{
	if (m_bufs.empty())
	{
		use(std::move(bufs));
	}
	else
	{
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			std::move_iterator<std::deque<util::shared_buffer>::iterator> mit{it};
			m_offsets.push_back(m_size);
			m_size += it->size();
			m_bufs.emplace_back(*mit);
		}
		bufs.clear();
	}
}

template<>
inline void
source<util::shared_buffer>::append(std::deque<util::shared_buffer> const& bufs)
{
	if (m_bufs.empty())
	{
		use(bufs);
	}
	else
	{
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			m_offsets.push_back(m_size);
			m_size += it->size();
			m_bufs.emplace_back(*it);
		}
	}
}

template<>
inline void
source<util::shared_buffer>::append(util::shared_buffer&& buf)
{
	if (m_bufs.empty())
	{
		use(buf);
	}
	else
	{
		m_offsets.push_back(m_size);
		m_size += buf.size();
		m_bufs.emplace_back(std::move(buf));
	}
}

template<>
inline void
source<util::shared_buffer>::append(util::shared_buffer const& buf)
{
	if (m_bufs.empty())
	{
		use(buf);
	}
	else
	{
		m_offsets.push_back(m_size);
		m_size += buf.size();
		m_bufs.emplace_back(buf);
	}
}


template<>
inline void
source<util::const_buffer>::append(std::deque<util::shared_buffer>&& bufs)
{
	if (m_bufs.empty())
	{
		use(bufs);
	}
	else
	{
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			m_offsets.push_back(m_size);
			m_size += it->size();
			m_bufs.emplace_back(*it);
		}
	}
}

template<>
inline void
source<util::const_buffer>::append(std::deque<util::shared_buffer> const& bufs)
{
	if (m_bufs.empty())
	{
		use(bufs);
	}
	else
	{
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			m_offsets.push_back(m_size);
			m_size += it->size();
			m_bufs.emplace_back(*it);
		}
	}
}

template<>
inline void
source<util::const_buffer>::append(util::shared_buffer&& buf)
{
	if (m_bufs.empty())
	{
		use(buf);
	}
	else
	{
		m_offsets.push_back(m_size);
		m_size += buf.size();
		m_bufs.emplace_back(buf);
	}
}

template<>
inline void
source<util::const_buffer>::append(util::shared_buffer const& buf)
{
	if (m_bufs.empty())
	{
		use(buf);
	}
	else
	{
		m_offsets.push_back(m_size);
		m_size += buf.size();
		m_bufs.emplace_back(buf);
	}
}


template<>
inline void
source<util::mutable_buffer>::append(std::deque<util::shared_buffer>&& bufs)
{
	if (m_bufs.empty())
	{
		use(bufs);
	}
	else
	{
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			m_offsets.push_back(m_size);
			m_size += it->size();
			m_bufs.emplace_back(*it);
		}
	}
}

template<>
inline void
source<util::mutable_buffer>::append(std::deque<util::shared_buffer> const& bufs)
{
	if (m_bufs.empty())
	{
		use(bufs);
	}
	else
	{
		for (auto it = bufs.begin(); it != bufs.end(); ++it)
		{
			m_offsets.push_back(m_size);
			m_size += it->size();
			m_bufs.emplace_back(*it);
		}
	}
}

template<>
inline void
source<util::mutable_buffer>::append(util::shared_buffer&& buf)
{
	if (m_bufs.empty())
	{
		use(buf);
	}
	else
	{
		m_offsets.push_back(m_size);
		m_size += buf.size();
		m_bufs.emplace_back(buf);
	}
}

template<>
inline void
source<util::mutable_buffer>::append(util::shared_buffer const& buf)
{
	if (m_bufs.empty())
	{
		use(buf);
	}
	else
	{
		m_offsets.push_back(m_size);
		m_size += buf.size();
		m_bufs.emplace_back(buf);
	}
}


}    // namespace bufseq
}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_BUFSEQ_SOURCE_H