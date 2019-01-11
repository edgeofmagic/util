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

namespace logicmill
{
namespace bstream
{
namespace memory
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

	source() : base{}, m_buf{} {}

	source(shared_buffer const& buf);

	source(shared_buffer&& buf);

	source(const_buffer const& buf) : base{}, m_buf{buf}
	{
		set_ptrs(m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size());
	}

	source(const_buffer&& buf) : base{}, m_buf{std::move(buf)}
	{
		set_ptrs(m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size());
	}

	source(mutable_buffer&& buf) : base{}, m_buf{std::move(buf)}
	{
		set_ptrs(m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size());
	}

	source(buffer const& buf) : base{}, m_buf{buf}
	{
		set_ptrs(m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size());
	}

	const_buffer
	get_buffer()
	{
		return m_buf;
	}

	const_buffer&
	get_buffer_ref()
	{
		return m_buf;
	}

	Buffer
	release_buffer()
	{
		set_ptrs(nullptr, nullptr, nullptr);
		return std::move( m_buf );
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
		return m_buf.size();
	}

	virtual position_type
	really_seek(position_type pos, std::error_code& err) override
	{
		err.clear();
		m_next = m_base + pos;
		return pos;
	}

	virtual position_type
	really_get_position() const override
	{
		return gpos();
	}

protected:
	Buffer m_buf;
};


template<>
inline source<const_buffer>::source(shared_buffer const& buf) : base{}, m_buf{buf}
{
	set_ptrs(m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size());
}

template<>
inline source<const_buffer>::source(shared_buffer&& buf)
	: base{}, m_buf{buf}    // construct by copy (as base type buffer); can't move shared_buffer to const_buffer
{
	set_ptrs(m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size());
}

template<>
inline source<shared_buffer>::source(shared_buffer const& buf) : base{}, m_buf{buf}
{
	set_ptrs(m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size());
}

template<>
inline source<shared_buffer>::source(shared_buffer&& buf) : base{}, m_buf{std::move(buf)}
{
	set_ptrs(m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size());
}

template<>
inline shared_buffer
source<const_buffer>::get_shared_slice(size_type n, std::error_code& err)
{
	shared_buffer result{m_buf, gpos(), n, err};
	if (!err)
	{
		gbump(n);
	}
	return result;
}

template<>
inline shared_buffer
source<const_buffer>::get_shared_slice(size_type n)
{
	shared_buffer result{m_buf, gpos(), n};
	gbump(n);
	return result;
}

template<>
inline shared_buffer
source<shared_buffer>::get_shared_slice(size_type n, std::error_code& err)
{
	shared_buffer result{m_buf, gpos(), n, err};
	if (!err)
	{
		gbump(n);
	}
	return result;
}

template<>
inline shared_buffer
source<shared_buffer>::get_shared_slice(size_type n)
{
	shared_buffer result{m_buf, gpos(), n};
	gbump(n);
	return result;
}

template<>
inline const_buffer
source<const_buffer>::get_slice(size_type n, std::error_code& err)
{
	const_buffer result{m_buf, gpos(), n, err};
	if (!err)
	{
		gbump(n);
	}
	return result;
}

template<>
inline const_buffer
source<const_buffer>::get_slice(size_type n)
{
	const_buffer result{m_buf, gpos(), n};
	gbump(n);
	return result;
}

template<>
inline const_buffer
source<shared_buffer>::get_slice(size_type n, std::error_code& err)
{
	const_buffer result{m_buf, gpos(), n, err};
	if (!err)
	{
		gbump(n);
	}
	return result;
}

template<>
inline const_buffer
source<shared_buffer>::get_slice(size_type n)
{
	const_buffer result{m_buf, gpos(), n};
	gbump(n);
	return result;
}

}    // namespace memory
}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_MEMORY_SOURCE_H