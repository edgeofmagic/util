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

#ifndef UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE
#define UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE 1024
#endif

#define ASSERT_VALID_PPTRS(target)                                                                                     \
	{                                                                                                                  \
		assert((target).pbase());                                                                                      \
		assert((target).pptr() >= (target).pbase());                                                                   \
		assert((target).epptr() >= (target).pptr());                                                                   \
		assert((target).hwm() >= (target).pbase());                                                                    \
		assert((target).epptr() >= (target).hwm());                                                                    \
		assert(reinterpret_cast<char*>((target).m_buf.data()) == (target).pbase());                                    \
		assert((target).epptr() == (reinterpret_cast<char*>((target).m_buf.data()) + (target).m_buf.capacity()));      \
	}
/**/

#define ASSERT_VALID_GPTRS(target)                                                                                     \
	{                                                                                                                  \
		assert((target).eback());                                                                                      \
		assert((target).gptr() >= (target).eback());                                                                   \
		assert((target).egptr() >= (target).gptr());                                                                   \
		assert(reinterpret_cast<char*>(const_cast<logicmill::byte_type*>((target).m_buf.data())) == (target).eback()); \
		assert((target).egptr()                                                                                        \
			   == (reinterpret_cast<char*>(const_cast<logicmill::byte_type*>((target).m_buf.data()))                   \
				   + (target).m_buf.size()));                                                                          \
	}
/**/


namespace logicmill
{
namespace util
{

class omembuf : public std::streambuf
{
private:
	// force a hard lower bound to avoid non-resizing dilemma in make_room, when cushioned == required == 1
	static constexpr std::streamsize min_alloc_size
			= (UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE > 16) ? UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE : 16;

public:
	using buffer_type = logicmill::util::mutable_buffer;

	omembuf(buffer_type&& buf) : m_buf(std::move(buf)), m_high_watermark{nullptr}
	{
		pubimbue(std::locale::classic());
		auto base = reinterpret_cast<char_type*>(m_buf.data());
		setp(base, base + m_buf.capacity());
		hwm(base);
	}

	omembuf(std::size_t capacity) : m_buf(capacity), m_high_watermark{nullptr}
	{
		pubimbue(std::locale::classic());
		auto base = reinterpret_cast<char_type*>(m_buf.data());
		setp(base, base + m_buf.capacity());
		hwm(base);
	}

	omembuf(omembuf&& rhs) : m_buf{std::move(rhs.m_buf)}, m_high_watermark{rhs.m_high_watermark}
	{
		setp(reinterpret_cast<char*>(m_buf.data()), reinterpret_cast<char*>(m_buf.data()) + m_buf.capacity());
		pbump(rhs.pptr() - rhs.pbase());
		assert(pbase() == rhs.pbase());
		assert(pptr() == rhs.pptr());
		assert(epptr() == rhs.epptr());
		assert(hwm() == rhs.hwm());
		rhs.null_ptrs();
		pubimbue(std::locale::classic());
		ASSERT_VALID_PPTRS(rhs);
	}

	omembuf&
	operator=(omembuf&& rhs)
	{
		m_buf = std::move(rhs.m_buf);
		hwm(rhs.hwm());
		char* p = reinterpret_cast<char*>(m_buf.data());
		setp(p, p + m_buf.capacity());
		pbump(rhs.pptr() - rhs.pbase());
		assert(pbase() == rhs.pbase());
		assert(pptr() == rhs.pptr());
		assert(epptr() == rhs.epptr());
		assert(hwm() == rhs.hwm());
		rhs.null_ptrs();
		ASSERT_VALID_PPTRS(rhs);
		return *this;
	}

	buffer_type const&
	get_buffer()
	{
		sync_buffer_size();
		return m_buf;
	}

	buffer_type
	release_buffer()
	{
		sync_buffer_size();
		null_ptrs();
		return std::move(m_buf);
	}

	void
	print_state()
	{
		std::ptrdiff_t pptr_diff  = pptr() - pbase();
		std::ptrdiff_t epptr_diff = epptr() - pbase();
		std::ptrdiff_t hwm_diff   = hwm() - pbase();
		std::cout << "pbase: " << (void*)pbase() << ", pptr offset: " << pptr_diff << ", epptr offset: " << epptr_diff
				  << ", hwm offset: " << hwm_diff << std::endl;
		m_buf.size(hwm_diff);
		m_buf.dump(std::cout);
		std::cout.flush();
	}

	std::streamsize
	size()
	{
		sync_hwm();
		return static_cast<std::streamsize>(hwm() - pbase());
	}

	std::streamoff
	position()
	{
		return static_cast<std::streamsize>(pptr() - pbase());
	}

protected:
	char*
	hwm() const
	{
		return m_high_watermark;
	}

	void
	hwm(char* p)
	{
		m_high_watermark = p;
	}

	char*
	sync_hwm()
	{
		if (pptr() > hwm())
		{
			hwm(pptr());
		}
		return hwm();
	}

	void
	null_ptrs()
	{
		setp(nullptr, nullptr);
		hwm(nullptr);
	}

	void
	make_room(std::streamsize n)
	{
		assert(std::less_equal<char*>()(pptr(), epptr()));
		std::streamsize remaining = epptr() - pptr();
		if (remaining < n)
		{
			std::ptrdiff_t  pptr_diff      = pptr() - pbase();
			std::ptrdiff_t  hwm_diff       = hwm() - pbase();
			std::streamsize required       = pptr_diff + n;
			std::streamsize cushioned_size = (required * 3) / 2;
			m_buf.expand(std::max(min_alloc_size, cushioned_size));
			auto base = reinterpret_cast<char_type*>(m_buf.data());
			setp(base, base + m_buf.capacity());
			pbump(pptr_diff);
			hwm(pbase() + hwm_diff);
		}
		ASSERT_VALID_PPTRS(*this);
	}

	void
	sync_buffer_size()
	{
		std::size_t bufsize{0};
		if (pbase())
		{
			ASSERT_VALID_PPTRS(*this);
			sync_hwm();
			bufsize = static_cast<std::size_t>(hwm() - pbase());
		}
		m_buf.size(bufsize);
	}

	// inherited virtual overrides:

	virtual std::streamsize
	xsputn(const char_type* bytes, std::streamsize n) override
	{
		make_room(n);
		::memcpy(pptr(), bytes, n);
		pbump(n);
		return n;
	}

	virtual int_type
	overflow(int_type byte = traits_type::eof()) override
	{
		int_type result = traits_type::eof();
		if (byte != traits_type::eof())
		{
			make_room(1);
			*pptr() = byte;
			pbump(1);
			result = byte;
		}
		return result;
	}

	virtual std::streampos
	seekpos(std::streampos pos, std::ios_base::openmode which = std::ios_base::out) override
	{
		return seekoff(pos, std::ios_base::beg, which);
	}

	virtual std::streampos
	seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::out) override
	{
		std::streampos invalid_result{std::streamoff{-1}};
		if (!pbase())
			return invalid_result;
		ASSERT_VALID_PPTRS(*this);
		sync_hwm();
		std::streamoff hwm_off{hwm() - pbase()};
		std::streamoff new_off{0};
		switch (way)
		{
			case std::ios_base::beg:
				new_off = 0;
				break;
			case std::ios_base::cur:
				new_off = pptr() - pbase();
				break;
			case std::ios_base::end:
				new_off = hwm_off;
				break;
			default:
				return invalid_result;
		}
		new_off += off;
		if (new_off < 0 || hwm_off < new_off)
			return invalid_result;
		setp(pbase(), epptr());
		pbump(new_off);
		ASSERT_VALID_PPTRS(*this);
		return std::streampos{new_off};
	}

private:
	logicmill::util::mutable_buffer m_buf;
	char*                           m_high_watermark;
};

class imembuf : public std::streambuf
{
public:
	using buffer_type = logicmill::util::const_buffer;

	imembuf(buffer_type&& buf) : m_buf(std::move(buf))
	{
		char_type* base = reinterpret_cast<char_type*>(const_cast<byte_type*>(m_buf.data()));
		setg(base, base, base + m_buf.size());
	}

	imembuf(imembuf&& rhs) : m_buf{std::move(rhs.m_buf)}
	{
		char* p = reinterpret_cast<char*>(const_cast<logicmill::byte_type*>(m_buf.data()));
		setp(p, p + m_buf.size());
		pbump(rhs.gptr() - rhs.eback());
		assert(eback() == rhs.eback());
		assert(gptr() == rhs.gptr());
		assert(egptr() == rhs.egptr());
		rhs.null_ptrs();
		pubimbue(std::locale::classic());
		ASSERT_VALID_GPTRS(rhs);
	}

	imembuf&
	operator=(imembuf&& rhs)
	{
		m_buf = std::move(rhs.m_buf);
		char* p = reinterpret_cast<char*>(const_cast<logicmill::byte_type*>(m_buf.data()));
		setp(p, p + m_buf.size());
		pbump(rhs.gptr() - rhs.eback());
		assert(eback() == rhs.eback());
		assert(gptr() == rhs.gptr());
		assert(egptr() == rhs.egptr());
		rhs.null_ptrs();
		ASSERT_VALID_GPTRS(rhs);
		return *this;
	}

	imembuf&
	rewind()
	{
		setg(eback(), eback(), egptr());
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

	// for debugging/test:

	void
	print_state() const
	{
		std::ptrdiff_t gptr_diff  = gptr() - eback();
		std::ptrdiff_t egptr_diff = egptr() - eback();
		std::cout << "eback: " << (void*)eback() << ", gptr offset: " << gptr_diff << ", egptr offset: " << egptr_diff
				  << std::endl;
		m_buf.dump(std::cout);
		std::cout.flush();
	}

	std::streamsize
	size()
	{
		return static_cast<std::streamsize>(egptr() - eback());
	}

	std::streamoff
	position()
	{
		return static_cast<std::streamsize>(gptr() - eback());
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
		std::streamsize copy_size = std::min(n, egptr() - gptr());
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
		return seekoff(pos, std::ios_base::beg, which);
	}

	virtual std::streampos
	seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in) override
	{
		std::streampos invalid_result{std::streamoff{-1}};
		if (!eback())
			return invalid_result;
		ASSERT_VALID_GPTRS(*this);
		std::streamoff new_off{0};
		switch (way)
		{
			case std::ios_base::beg:
				new_off = 0;
				break;
			case std::ios_base::cur:
				new_off = gptr() - eback();
				break;
			case std::ios_base::end:
				new_off = egptr() - eback();
				break;
			default:
				return invalid_result;
		}
		new_off += off;
		if (new_off < 0 || (egptr() - eback()) < new_off)
			return invalid_result;
		setg(eback(), eback(), egptr());
		gbump(new_off);
		return std::streampos{new_off};
	}

	util::const_buffer m_buf;
};

}    // namespace util
}    // namespace logicmill

#endif    // UTIL_MEMBUF_H
