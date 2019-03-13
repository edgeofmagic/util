#ifndef UTIL_MEMBUF_H
#define UTIL_MEMBUF_H

#include <functional>
#include <iosfwd>
#include <iostream>
#include <logicmill/util/buffer.h>
#include <logicmill/util/dumpster.h>
#include <streambuf>
#include <system_error>
#include <vector>

#define SB_DEBUG_ON 0

#if (SB_DEBUG_ON)

#define SB_DEBUG(...)                                                                                                  \
	{                                                                                                                  \
		std::cout << __VA_ARGS__ << std::endl;                                                                         \
		print_state();                                                                                                 \
	}

#else

#define SB_DEBUG(...)

#endif

#ifndef UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE
#define UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE 1024
#endif

namespace logicmill
{
namespace util
{

class omembuf : public std::streambuf
{
public:
	friend class imembuf;

	using buffer_type = logicmill::util::mutable_buffer;

	omembuf(buffer_type&& buf) : m_buf(std::move(buf)), m_high_water_mark{}
	{
		pubimbue(std::locale::classic());
		set_ptrs();
	}

	omembuf(std::size_t capacity) : m_buf(capacity), m_high_water_mark{}
	{
		pubimbue(std::locale::classic());
		set_ptrs();
	}

	omembuf&
	clear() noexcept
	{
		reset_ptrs();
		reset_high_water_mark();
		return *this;
	}

	buffer_type const&
	get_buffer()
	{
		m_buf.size(endoff());
		return m_buf;
	}

	buffer_type
	release_buffer()
	{
		m_buf.size(endoff());
		reset_high_water_mark();
		null_ptrs();
		return std::move(m_buf);
	}

	void
	print_state()
	{
		std::cout << "[ " << (void*)pbase() << ", " << poff() << ", " << endoff() << " ]"
				  << " hwm: " << endoff() << std::endl;
		m_buf.size(endoff());
		m_buf.dump(std::cout);
		std::cout.flush();
	}

protected:
	// force a hard lower bound to avoid non-resizing dilemma in accommodate_put, when cushioned == required == 1
	static constexpr std::streamsize min_alloc_size
			= (UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE > 16) ? UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE : 16;

	virtual std::streamsize
	xsputn(const char_type* bytes, std::streamsize n) override
	{
		SB_DEBUG("enter omembuf::xsputn( " << n << " ): ")

		accommodate_put(n);
		::memcpy(pptr(), bytes, n);
		pbump(n);

		SB_DEBUG("leave omembuf::xsputn, pos " << ppos() << ": ")

		return n;
	}

	virtual int_type
	overflow(int_type byte = traits_type::eof()) override
	{
		SB_DEBUG("enter omembuf::overflow( " << byte << " ): ")

		int_type result = traits_type::eof();

		if (byte != traits_type::eof())
		{
			accommodate_put(1);
			*pptr() = byte;
			pbump(1);
			result = byte;
		}

		SB_DEBUG("leave omembuf::overflow, result " << result << ": ")

		return result;
	}

	virtual std::streampos
	seekpos(std::streampos pos, std::ios_base::openmode which = std::ios_base::out) override
	{
		SB_DEBUG("enter omembuf::seekpos( " << pos << " ): ")

		std::streampos result{std::streamoff{-1}};
		if ((which & std::ios_base::out) != 0)
		{
			high_water_mark();
			result = set_position(pos);
		}

		return result;
	}

	virtual std::streampos
	seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::out) override
	{
		std::streampos result{std::streamoff{-1}};

		if ((which & std::ios_base::out) != 0)
		{
			auto hwm = high_water_mark();

			auto current_pos = ppos();
			std::streampos new_pos{};
			switch (way)
			{
				case std::ios_base::seekdir::beg:
				{
					new_pos = off;
				}
				break;

				case std::ios_base::seekdir::cur:
				{
					new_pos = current_pos + off;
				}
				break;

				case std::ios_base::seekdir::end:
				{
					new_pos = hwm + off;
				}
				break;
			}

			result = set_position(new_pos);
		}
		return result;
	}

	std::streampos
	set_position(std::streampos new_pos)
	{
		std::streamoff new_off{new_pos};
		SB_DEBUG("enter omembuf::set_position( " << new_off << " ): ")

		std::streampos result{std::streamoff{-1}};

		if (new_off >= 0)
		{
			std::streampos current_pos = ppos();
			std::streamoff displacement = new_pos - current_pos;

			auto hwm = high_water_mark();

			if (new_off > std::streamoff{hwm})
			{
				// std::streamoff fill_size = new_pos - hwm;
				// accommodate_put(fill_size);
				// ::memset(pptr(), 0, fill_size);
				// pbump(displacement);
				// high_water_mark();
				// assert(ppos() == new_pos);
				// result = new_pos;

			}
			else
			{
				pbump(displacement);
				result = new_pos;
			}

		}

		SB_DEBUG("after omembuf::set_position( " << pos << " ): ")

		return result;
	}

	void
	set_ptrs()
	{
		auto base = reinterpret_cast<char_type*>(m_buf.data());
		setp(base, base + m_buf.capacity());
	}

	void
	reset_ptrs()
	{
		auto base = reinterpret_cast<char_type*>(m_buf.data());
		setp(base, base + m_buf.capacity());
	}

	void
	null_ptrs()
	{
		setp(nullptr, nullptr);
	}

	void
	accommodate_put(std::streamsize n)
	{
		assert(std::less_equal<char*>()(pptr(), epptr()));
		std::streamsize remaining = epptr() - pptr();
		if (remaining < n)
		{
			std::streamoff  current        = poff();
			std::streamsize required       = current + n;
			std::streamsize cushioned_size = (required * 3) / 2;
			m_buf.expand(std::max(min_alloc_size, cushioned_size));
			reset_ptrs();
			pbump(current);
		}
	}

	std::streamoff
	poff() const
	{
		return static_cast<std::streamoff>(pptr() - pbase());
	}

	std::streampos
	ppos() const
	{
		return std::streampos{poff()};
	}

	std::streampos
	endpos() const
	{
		return high_water_mark();
	}

	std::streamoff
	endoff() const
	{
		return std::streamoff{endpos()};
	}

	std::streampos
	high_water_mark() const
	{
		if (poff() > std::streamoff{m_high_water_mark})
		{
			m_high_water_mark = ppos();
		}

		assert(std::streamoff{m_high_water_mark} <= m_buf.capacity());
		assert(std::streamoff{m_high_water_mark} >= 0);

		return m_high_water_mark;
	}

	void
	reset_high_water_mark()
	{
		m_high_water_mark = std::streampos{};
	}

	logicmill::util::mutable_buffer m_buf;
	mutable std::streampos          m_high_water_mark;
};

class imembuf : public std::streambuf
{
public:
	using buffer_type = logicmill::util::const_buffer;

	imembuf(buffer_type&& buf) : m_buf(std::move(buf))
	{
		set_ptrs();
	}

	imembuf&
	rewind()
	{
		gbump(-goff());
		return *this;
	}

	buffer_type const&
	get_buffer() const
	{
		return m_buf;
	}

	buffer_type
	release_buffer()
	{
		null_ptrs();
		return std::move(m_buf);
	}

	std::streamoff
	position()
	{
		return goff();
	}

	std::streampos
	advance(std::streamoff n)
	{
		std::streampos result{std::streamoff{-1}};
		if (n <= remaining())
		{
			gbump(static_cast<int>(n));
			result = gpos();
		}
		return result;
	}

	std::streamsize
	size()
	{
		return static_cast<std::streamsize>(egptr() - eback());
	}

	std::streamsize
	remaining()
	{
		return static_cast<std::streamsize>(egptr() - gptr());
	}

	void
	print_state() const
	{
		std::cout << "[ " << (void*)eback() << ", " << goff() << ", " << endoff() << " ]" << std::endl;
		m_buf.dump(std::cout);
		std::cout.flush();
	}

protected:
	void
	set_ptrs()
	{
		char_type* base = reinterpret_cast<char_type*>(const_cast<byte_type*>(m_buf.data()));
		setg(base, base, base + m_buf.size());
	}

	void
	null_ptrs()
	{
		setg(nullptr, nullptr, nullptr);
	}

	virtual std::streamsize
	xsgetn(char_type* s, std::streamsize n) override
	{
		std::streamsize result{0};
		std::streamsize copy_size = std::min(n, remaining());
		if (copy_size > 0)
		{
			::memcpy(s, gptr(), copy_size);
			gbump(copy_size);
			result = copy_size;
		}

		return result;
	}

	virtual std::streampos
	seekpos(std::streampos pos, std::ios_base::openmode which = std::ios_base::in) override
	{
		SB_DEBUG("enter imembuf::seekpos( " << pos << " ): ")

		std::streampos result{std::streamoff{-1}};
		if ((which & std::ios_base::in) != 0)
		{
			result = set_position(pos);
		}
		return result;
	}

	std::streamoff
	goff() const
	{
		return static_cast<std::streamoff>(gptr() - eback());
	}

	std::streampos
	gpos() const
	{
		return std::streampos{goff()};
	}

	std::streamoff
	endoff() const
	{
		return static_cast<std::streamoff>(egptr() - eback());
	}

	std::streampos
	endpos() const
	{
		return std::streampos{endoff()};
	}

	virtual std::streampos
	seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in) override
	{
		std::streampos result{std::streamoff{-1}};

		if ((which & std::ios_base::in) != 0)
		{
			std::streampos new_pos{};

			switch (way)
			{
				case std::ios_base::seekdir::beg:
				{
					new_pos = std::streampos{off};
				}
				break;

				case std::ios_base::seekdir::cur:
				{
					new_pos = gpos() + off;
				}
				break;

				case std::ios_base::seekdir::end:
				{
					new_pos = endpos() + off;
				}
				break;
			}

			result = set_position(new_pos);
		}

		return result;
	}

	std::streampos
	set_position(std::streampos new_pos)
	{
		std::streamoff new_off{new_pos};
		SB_DEBUG("enter imembuf::set_position(" << new_off << "): ")

		std::streampos result{std::streamoff{-1}};
		if (new_off >= 0 && new_off <= endoff())
		{
			gbump(new_pos - gpos());
			result = new_pos;
		}

		SB_DEBUG("after imembuf::set_position( " << new_off << " ): ")

		return result;
	}

	util::const_buffer m_buf;
};

}    // namespace util
}    // namespace logicmill

#endif    // UTIL_MEMBUF_H
