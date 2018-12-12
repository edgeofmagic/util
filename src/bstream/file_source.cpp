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

#include <fcntl.h>
#include <logicmill/bstream/file/source.h>
#include <unistd.h>

using namespace logicmill;
using namespace bstream;

file::source::source(size_type buffer_size)
	: m_buf{buffer_size}, m_filename{}, m_is_open{false}, m_flags{O_RDONLY}, m_fd{-1}, m_size{0}
{}

file::source::source(std::string const& filename, std::error_code& err, int flag_overrides, size_type buffer_size)
	: m_buf{buffer_size},
	  m_filename{filename},
	  m_is_open{false},
	  m_flags{O_RDONLY | flag_overrides},
	  m_fd{-1},
	  m_size{0}
{
	really_open(err);
}

file::source::source(std::string const& filename, int flag_overrides, size_type buffer_size)
	: m_buf{buffer_size},
	  m_filename{filename},
	  m_is_open{false},
	  m_flags{O_RDONLY | flag_overrides},
	  m_fd{-1},
	  m_size{0}
{
	std::error_code err;
	really_open(err);
	if (err)
	{
		throw std::system_error{err};
	}
}

size_type
file::source::really_underflow(std::error_code& err)
{
	err.clear();
	assert(m_next == m_end);
	m_base_offset       = gpos();
	m_next              = m_base;
	size_type available = load_buffer(err);
	if (err)
	{
		available = 0;
	}
	m_buf.size(available);
	m_end = m_next + available;
	return available;
}

void
file::source::close(std::error_code& err)
{
	err.clear();
	if (m_is_open)
	{
		auto close_result = ::close(m_fd);
		if (close_result < 0)
		{
			err = std::error_code{errno, std::generic_category()};
			goto exit;
		}

		m_is_open = false;
	}

exit:
	return;
}

void
file::source::close()
{
	if (m_is_open)
	{
		auto close_result = ::close(m_fd);
		if (close_result < 0)
		{
			throw std::system_error{std::error_code{errno, std::generic_category()}};
		}

		m_is_open = false;
	}
}

size_type
file::source::load_buffer(std::error_code& err)
{
	err.clear();
	assert(m_next == m_base);

	auto read_result = ::read(m_fd, const_cast<byte_type*>(m_base), m_buf.capacity());
	if (read_result < 0)
	{
		err         = std::error_code{errno, std::generic_category()};
		read_result = 0;
		goto exit;
	}

exit:
	return read_result;
}

void
file::source::really_open(std::error_code& err)
{
	err.clear();

	off_t seek_result = 0;

	if (m_is_open)
	{
		close(err);
		if (err)
			goto exit;
		m_is_open = false;
	}

	m_fd = ::open(m_filename.c_str(), m_flags);
	if (m_fd < 0)
	{
		err = std::error_code{errno, std::generic_category()};
		goto exit;
	}

	seek_result = ::lseek(m_fd, 0, SEEK_END);
	if (seek_result < 0)
	{
		err    = std::error_code{errno, std::generic_category()};
		m_size = bstream::npos;
		goto exit;
	}
	else
	{
		m_size = seek_result;
	}

	if (::lseek(m_fd, 0, SEEK_SET) < 0)
	{
		err = std::error_code{errno, std::generic_category()};
		goto exit;
	}

	m_is_open     = true;
	m_base_offset = 0;
	reset_ptrs();

exit:
	return;
}

size_type
file::source::really_get_size() const
{
	return m_size;
}


void
file::source::open(std::string const& filename, std::error_code& err, int flag_overrides)
{
	m_filename = filename;
	m_flags    = O_RDONLY | flag_overrides;
	really_open(err);
}

void
file::source::open(std::string const& filename, int flag_overrides)
{
	m_filename = filename;
	m_flags    = O_RDONLY | flag_overrides;
	std::error_code err;
	really_open(err);
	if (err)
	{
		throw std::system_error{err};
	}
}

position_type
file::source::really_seek(position_type pos, std::error_code& err)
{
	err.clear();
	position_type result = bstream::npos;

	result = ::lseek(m_fd, pos, SEEK_SET);

	if (result < 0)
	{
		err    = std::error_code{errno, std::generic_category()};
		result = bstream::npos;
		goto exit;
	}

	m_base_offset = result;
	reset_ptrs();
exit:
	return result;
}

position_type
file::source::really_get_position() const
{
	return gpos();
}
