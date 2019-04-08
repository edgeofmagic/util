#ifndef UTIL_MEMBUF_H
#define UTIL_MEMBUF_H

#include <functional>
#include <iosfwd>
#include <iostream>
#include <util/buffer.h>
#include <util/dumpster.h>
#include <sstream>
#include <streambuf>
#include <system_error>
#include <vector>

#ifndef UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE
#define UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE 16
#endif

#ifndef NDEBUG

#define ASSERT_VALID_PPTRS(target)                                                                                     \
	{                                                                                                                  \
		auto& b = (target);                                                                                            \
		if (b.m_buf.capacity() < 1)                                                                                    \
		{                                                                                                              \
			assert(b.pbase() == nullptr);                                                                              \
			assert(b.pptr() == nullptr);                                                                               \
			assert(b.epptr() == nullptr);                                                                              \
			assert(b.hwm() == nullptr);                                                                                \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			b.sync_hwm();                                                                                              \
			assert(b.pbase() != nullptr);                                                                              \
			assert(b.pptr() >= b.pbase());                                                                             \
			assert(b.pptr() <= b.epptr());                                                                             \
			assert(b.hwm() >= b.pbase());                                                                              \
			assert(b.hwm() <= b.epptr());                                                                              \
			assert(b.hwm() >= b.pptr());                                                                               \
			assert(reinterpret_cast<char*>(b.m_buf.data()) == b.pbase());                                              \
			assert(b.epptr() == (reinterpret_cast<char*>(b.m_buf.data()) + b.m_buf.capacity()));                       \
		}                                                                                                              \
	}
/**/

#define ASSERT_VALID_QPPTRS(target)                                                                                    \
	{                                                                                                                  \
		auto& b = (target);                                                                                            \
		assert(b.m_factory != nullptr);                                                                                \
		assert(b.m_alloc_size == b.m_factory->size());                                                                 \
		if (b.m_buf.size() < 1)                                                                                        \
		{                                                                                                              \
			assert(b.pbase() == nullptr);                                                                              \
			assert(b.pptr() == nullptr);                                                                               \
			assert(b.epptr() == nullptr);                                                                              \
			assert(b.hwm() == -1);                                                                                     \
			assert(b.m_current == -1);                                                                                 \
			assert(b.m_base_offset == -1);                                                                             \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			sync_hwm();                                                                                                \
			assert(b.m_current >= 0);                                                                                  \
			assert(b.m_buf.size() > 0);                                                                                \
			assert(b.m_base_offset == b.m_current * b.m_alloc_size);                                                   \
			assert(b.pbase());                                                                                         \
			assert(b.pptr() >= b.pbase());                                                                             \
			assert(b.epptr() >= b.pptr());                                                                             \
			assert(b.hwm() >= b.poff());                                                                               \
			assert(b.hwm() >= ((b.m_buf.size() - 1) * b.m_alloc_size));                                                \
			assert(b.hwm() <= (b.m_buf.size() * b.m_alloc_size)); /* TODO: is this correct? */                         \
			assert(b.epptr() == b.pbase() + b.m_alloc_size);                                                           \
			assert(reinterpret_cast<char*>(b.m_buf[b.m_current].data()) == b.pbase());                                 \
			assert(b.pbase() + b.m_alloc_size == b.epptr());                                                           \
			for (auto i = 0; i < b.m_buf.size() - 1; ++i)                                                              \
				assert(b.m_buf[i].size() == m_alloc_size);                                                             \
		}                                                                                                              \
	}
/**/

#define ASSERT_VALID_GPTRS(target)                                                                                     \
	{                                                                                                                  \
		auto& b = (target);                                                                                            \
		assert(b.eback() != nullptr);                                                                                  \
		assert(b.gptr() >= b.eback());                                                                                 \
		assert(b.egptr() >= b.gptr());                                                                                 \
		assert(reinterpret_cast<const char*>(b.m_buf.data()) == b.eback());                                            \
		assert(b.egptr() == (reinterpret_cast<const char*>(b.m_buf.data()) + b.m_buf.size()));                         \
	}
/**/

#define ASSERT_VALID_QGPTRS(target)                                                                                    \
	{                                                                                                                  \
		auto& b = (target);                                                                                            \
		if (b.m_buf.size() < 1)                                                                                        \
		{                                                                                                              \
			assert(b.m_size == 0);                                                                                     \
			assert(b.m_current == -1);                                                                                 \
			assert(b.m_base_offset == -1);                                                                             \
			assert(b.m_offsets.empty());                                                                               \
			assert(b.eback() == nullptr);                                                                              \
			assert(b.gptr() == nullptr);                                                                               \
			assert(b.egptr() == nullptr);                                                                              \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			assert(b.m_current >= 0);                                                                                  \
			assert(b.m_current < b.m_buf.size());                                                                      \
			assert(b.m_offsets.size() == b.m_buf.size());                                                              \
			for (std::size_t i = 0, size = 0; i < b.m_buf.size(); ++i)                                                 \
			{                                                                                                          \
				auto seg_size = b.m_buf[i].size();                                                                     \
				assert(seg_size > 0);                                                                                  \
				assert(b.m_offsets[i] == size);                                                                        \
				size += seg_size;                                                                                      \
			}                                                                                                          \
			assert(b.m_size == b.m_offsets.back() + b.m_buf.back().size());                                            \
			assert(b.m_base_offset == b.m_offsets[b.m_current]);                                                       \
			assert(reinterpret_cast<const char*>(b.m_buf[b.m_current].data()) == b.eback());                           \
			assert(b.eback() != nullptr);                                                                              \
			assert(b.gptr() >= b.eback());                                                                             \
			assert(b.egptr() >= b.gptr());                                                                             \
			assert(b.egptr() == b.eback() + b.m_buf[b.m_current].size());                                              \
		}                                                                                                              \
	}
/**/

#else

#define ASSERT_VALID_PPTRS(target)
#define ASSERT_VALID_QPPTRS(target)
#define ASSERT_VALID_GPTRS(target)
#define ASSERT_VALID_QGPTRS(target)

#endif    // ifndef NDEBUG

namespace util
{

class omembuf : public std::streambuf
{
public:
	using buffer_type = util::mutable_buffer;
	using off_type    = std::streamoff;
	using pos_type    = std::streampos;

private:
	// force a hard lower bound to avoid non-resizing dilemma in make_room, when cushioned == required == 1
	static constexpr std::streamsize min_alloc_size
			= (UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE > 16) ? UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE : 16;

	util::mutable_buffer m_buf;
	char*                           m_high_watermark;

public:
	omembuf() : m_buf{}, m_high_watermark{nullptr}
	{
		auto base = reinterpret_cast<char_type*>(m_buf.data());
		setp(base, base + m_buf.capacity());
		hwm(base + m_buf.size());
		pubimbue(std::locale::classic());
		ASSERT_VALID_PPTRS(*this);
	}

	omembuf(buffer_type&& buf) : m_buf(std::move(buf)), m_high_watermark{nullptr}
	{
		auto base = reinterpret_cast<char_type*>(m_buf.data());
		setp(base, base + m_buf.capacity());
		hwm(base + m_buf.size());
		pubimbue(std::locale::classic());
		ASSERT_VALID_PPTRS(*this);
	}

	omembuf(std::size_t capacity) : m_buf(capacity), m_high_watermark{nullptr}
	{
		auto base = reinterpret_cast<char_type*>(m_buf.data());
		setp(base, base + m_buf.capacity());
		hwm(base);
		pubimbue(std::locale::classic());
		ASSERT_VALID_PPTRS(*this);
	}

	omembuf(omembuf&& rhs) : m_buf{std::move(rhs.m_buf)}, m_high_watermark{rhs.m_high_watermark}
	{
		setp(reinterpret_cast<char*>(m_buf.data()), reinterpret_cast<char*>(m_buf.data()) + m_buf.capacity());
		pbump(rhs.pptr() - rhs.pbase());
		assert(pbase() == rhs.pbase());
		assert(pptr() == rhs.pptr());
		assert(epptr() == rhs.epptr());
		assert(hwm() == rhs.hwm());
		rhs.reset_ptrs_offsets();
		pubimbue(std::locale::classic());
		ASSERT_VALID_PPTRS(rhs);
		ASSERT_VALID_PPTRS(*this);
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
		rhs.reset_ptrs_offsets();
		ASSERT_VALID_PPTRS(rhs);
		ASSERT_VALID_PPTRS(*this);
		return *this;
	}

	omembuf&
	operator=(mutable_buffer&& buf)
	{
		m_buf = std::move(buf);
		char* p = reinterpret_cast<char*>(m_buf.data());
		if (p)
		{
			setp(p, p + m_buf.capacity());
			hwm(p + m_buf.size());

		}
		else
		{
			setp(nullptr, nullptr);
			hwm(nullptr);
		}
		ASSERT_VALID_PPTRS(*this);
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
		reset_ptrs_offsets();
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

	off_type
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
	reset_ptrs_offsets()
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
	showmanyc() override
	{
		return -1;
	}

	virtual omembuf* 
	setbuf(char_type* s, std::streamsize n) override
	{
		*this =  mutable_buffer{reinterpret_cast<byte_type*>(s), static_cast<size_type>(n), null_delete<byte_type>{}};
		return this;
	}

	virtual int
	sync() override
	{
		sync_hwm();
		return 0;
	}

	virtual std::streamsize
	xsputn(const char_type* bytes, std::streamsize n) override
	{
		std::streamsize result{0};
		std::streamsize remaining = epptr() - pptr();
		if (n <= remaining)
		{
			::memcpy(pptr(), bytes, n);
			pbump(n);
			result = n;
		}
		else
		{
			if (m_buf.is_expandable())
			{
				make_room(n);
				::memcpy(pptr(), bytes, n);
				pbump(n);
				result = n;
			}
			else
			{
				::memcpy(pptr(), bytes, remaining);
				pbump(remaining);
				result = remaining;
			}
		}
		return result;
	}

	virtual int_type
	overflow(int_type ch = traits_type::eof()) override
	{
		int_type result = traits_type::not_eof(ch);
		if (!traits_type::eq_int_type(ch, traits_type::eof()))
		{
			try
			{
				if (pptr() >= epptr())
				{
					assert(pptr() == epptr());
					make_room(1);
				}
				*pptr() = ch;
				pbump(1);
			}
			catch (...)
			{
				result = traits_type::eof();
			}
		}
		return result;
	}

	virtual pos_type
	seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::out) override
	{
		return seekoff(pos, std::ios_base::beg, which);
	}

	virtual pos_type
	seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::out) override
	{
		ASSERT_VALID_PPTRS(*this);
		pos_type result{off_type{-1}};
		if (pbase() != nullptr)
		{
			sync_hwm();
			off_type hwm_off{hwm() - pbase()};
			off_type new_off{0};
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
					new_off = -1;
			}
			if (new_off >= 0)
			{
				new_off += off;
				if (new_off >= 0 && new_off <= hwm_off)
				{
					setp(pbase(), epptr());
					pbump(new_off);
					result = pos_type{new_off};
				}
			}
		}
		ASSERT_VALID_PPTRS(*this);
		return result;
	}
};

class imembuf : public std::streambuf
{
public:
	using buffer_type = util::const_buffer;
	using off_type    = std::streamoff;
	using pos_type    = std::streampos;

private:
	buffer_type m_buf;

public:
	imembuf(buffer_type&& buf) : m_buf(std::move(buf))
	{
		char_type* p = reinterpret_cast<char_type*>(const_cast<byte_type*>(m_buf.data()));
		if (p)
		{
			char_type* base = reinterpret_cast<char_type*>(const_cast<byte_type*>(m_buf.data()));
			setg(base, base, base + m_buf.size());
		}
		else
		{
			setg(nullptr, nullptr, nullptr);
		}
		pubimbue(std::locale::classic());
		ASSERT_VALID_GPTRS(*this);
	}

	imembuf(imembuf&& rhs) : m_buf{std::move(rhs.m_buf)}
	{
		char_type* p = reinterpret_cast<char_type*>(const_cast<byte_type*>(m_buf.data()));
		if (p)
		{
			setg(p, p, p + m_buf.size());
			gbump(rhs.gptr() - rhs.eback());
			assert(eback() == rhs.eback());
			assert(gptr() == rhs.gptr());
			assert(egptr() == rhs.egptr());
		}
		else
		{
			setg(nullptr, nullptr, nullptr);
		}
		rhs.reset_ptrs();
		pubimbue(std::locale::classic());
		ASSERT_VALID_GPTRS(rhs);
		ASSERT_VALID_GPTRS(*this);
	}

	imembuf&
	operator=(imembuf&& rhs)
	{
		m_buf        = std::move(rhs.m_buf);
		char_type* p = reinterpret_cast<char_type*>(const_cast<byte_type*>(m_buf.data()));
		if (p)
		{
			setg(p, p, p + m_buf.size());
			gbump(rhs.gptr() - rhs.eback());
			assert(eback() == rhs.eback());
			assert(gptr() == rhs.gptr());
			assert(egptr() == rhs.egptr());
		}
		else
		{
			setg(nullptr, nullptr, nullptr);
		}
		rhs.reset_ptrs();
		ASSERT_VALID_GPTRS(rhs);
		ASSERT_VALID_GPTRS(*this);
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
		reset_ptrs();
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

	off_type
	position()
	{
		return static_cast<std::streamsize>(gptr() - eback());
	}

protected:

	void
	reset_ptrs()
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

	virtual pos_type
	seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) override
	{
		return seekoff(pos, std::ios_base::beg, which);
	}

	virtual pos_type
	seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in) override
	{
		pos_type result{off_type{-1}};
		if (eback())
		{
			ASSERT_VALID_GPTRS(*this);
			off_type new_off{0};
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
					new_off = -1;
			}
			if (new_off >= 0)
			{
				new_off += off;
				if (new_off >= 0 && new_off <= (egptr() - eback()))
				{
					setg(eback(), eback(), egptr());
					gbump(new_off);
					result = pos_type{new_off};
				}
			}
		}
		return result;
	}
};

class omemqbuf : public std::streambuf
{
public:
	using buffer_type = std::deque<util::mutable_buffer>;
	using off_type    = std::streamoff;
	using pos_type    = std::streampos;
	using index_type  = std::int32_t;

private:
	buffer_type                                   m_buf;
	index_type                                    m_current;    // index of current buffer
	off_type                                      m_base_offset;
	off_type                                      m_high_watermark;
	size_type                                     m_alloc_size;    // capacity of individual buffers
	std::unique_ptr<util::mutable_buffer_factory> m_factory;

	omemqbuf(std::unique_ptr<mutable_buffer_factory>&& factory)
		: m_buf{},
		  m_current{-1},
		  m_base_offset{-1},
		  m_high_watermark{-1},
		  m_alloc_size{factory->size()},
		  m_factory{std::move(factory)}
	{
		setp(nullptr, nullptr);
		pubimbue(std::locale::classic());
	}

	bool
	move_from(omemqbuf&& other)
	{
		ASSERT_VALID_QPPTRS(other);
		bool result{false};
		if (other.m_buf.size() > 0)
		{
			other.sync_buffer_size();
			m_buf     = std::move(other.m_buf);
			m_current = other.m_current;
			hwm(other.hwm());
			sync_current_segment();
			pbump(other.pptr() - other.pbase());
			other.reset_ptrs_offsets();
			result = true;
		}
		return result;
	}

public:
	static constexpr std::size_t min_alloc_size
			= (UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE > 16) ? UTIL_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE : 16;

	omemqbuf(size_type alloc_size) : omemqbuf{std::make_unique<util::mutable_buffer_alloc_factory<>>(alloc_size)}
	{
		ASSERT_VALID_QPPTRS(*this);
	}

	template<
			class _Alloc,
			class = std::enable_if_t<
					traits::is_allocator<_Alloc>::value && std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	omemqbuf(size_type alloc_size, _Alloc&& alloc)
		: omemqbuf{
				  std::make_unique<util::mutable_buffer_alloc_factory<_Alloc>>(alloc_size, std::forward<_Alloc>(alloc))}
	{
		ASSERT_VALID_QPPTRS(*this);
	}

	template<class IntType, IntType Size>
	omemqbuf(std::integral_constant<IntType, Size>)
		: omemqbuf{std::make_unique<util::mutable_buffer_fixed_factory<static_cast<std::size_t>(Size)>>()}
	{
		static_assert(Size >= min_alloc_size, "allocation size is less than minimum required");
		ASSERT_VALID_QPPTRS(*this);
	}

	omemqbuf(omemqbuf&& rhs) : omemqbuf{rhs.m_factory->dup()}
	{
		move_from(std::move(rhs));
		ASSERT_VALID_QPPTRS(*this);
		ASSERT_VALID_QPPTRS(rhs);
	}

	omemqbuf&
	operator=(omemqbuf&& rhs)
	{
		ASSERT_VALID_QPPTRS(rhs);
		m_factory    = rhs.m_factory->dup();
		m_alloc_size = m_factory->size();

		if (!move_from(std::move(rhs)))
		{
			m_buf.clear();
			reset_ptrs_offsets();
		}

		ASSERT_VALID_QPPTRS(*this);
		ASSERT_VALID_QPPTRS(rhs);
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
		reset_ptrs_offsets();
		return std::move(m_buf);
	}

	void
	print_state()
	{
		sync_hwm();
		sync_buffer_size();
		std::ptrdiff_t pptr_diff  = pptr() - pbase();
		std::ptrdiff_t epptr_diff = epptr() - pbase();
		std::cout << "segment count: " << m_buf.size() << ", alloc size: " << m_alloc_size
				  << ",  current segment: " << m_current << ", pbase: " << (void*)pbase()
				  << ", pptr offset: " << pptr_diff << ", hwm: " << hwm() << std::endl;
		m_buf[m_current].dump(std::cout);
		std::cout.flush();
	}

	std::streamsize
	size()
	{
		sync_hwm();
		return static_cast<std::streamsize>(hwm());
	}

	off_type
	position()
	{
		return poff();
	}

protected:
	off_type
	poff() const
	{
		return (m_current < 0) ? -1 : (m_base_offset + (pptr() - pbase()));
	}

	off_type
	hwm() const
	{
		return m_high_watermark;
	}

	void
	hwm(off_type off)
	{
		m_high_watermark = off;
	}

	off_type
	sync_hwm()
	{
		if (poff() > hwm())
		{
			hwm(poff());
		}
		return hwm();
	}

	void
	reset_ptrs_offsets()
	{
		setp(nullptr, nullptr);
		hwm(-1);
		m_current     = -1;
		m_base_offset = -1;
	}

	void
	sync_buffer_size()
	{
		if (m_buf.size() > 0)
		{
			sync_hwm();
			auto  last_segment_index = m_buf.size() - 1;
			auto& last_segment       = m_buf[last_segment_index];
			last_segment.size(hwm() - last_segment_index * m_alloc_size);
		}
		ASSERT_VALID_QPPTRS(*this);
	}

	// inherited virtual overrides:

	virtual std::streamsize
	xsputn(const char_type* src, std::streamsize n) override
	{
		auto remaining = n;
		if (remaining > 0)
		{
			if (pptr() >= epptr())
			{
				assert(pptr() == epptr());
				next_segment();
			}

			if (remaining <= epptr() - pptr())    // optimize for common case ( no overflow )
			{
				::memcpy(pptr(), src, remaining);
				pbump(remaining);
			}
			else
			{
				auto p = src;
				while (remaining > 0)
				{
					if (pptr() >= epptr())
					{
						assert(pptr() == epptr());    // just checking
						next_segment();
					}
					std::ptrdiff_t avail      = epptr() - pptr();
					size_type      chunk_size = std::min(avail, remaining);
					::memcpy(pptr(), p, chunk_size);
					p += chunk_size;
					remaining -= chunk_size;
					pbump(chunk_size);
				}
			}
		}
		return n;
	}

	void
	next_segment()
	{
		if (m_current < 0)
		{
			assert(pbase() == nullptr);
			assert(pptr() == nullptr);
			assert(epptr() == nullptr);
			assert(m_buf.size() == 0);
			assert(m_base_offset == -1);
			assert(hwm() == -1);
			m_buf.emplace_back(m_factory->create());
			m_current = 0;
		}
		else
		{
			assert(m_current < m_buf.size());
			assert(poff() == (m_current + 1) * m_alloc_size);
			if (m_current == m_buf.size() - 1)    // last buffer in deque, extend deque
			{
				m_buf[m_current].size(m_alloc_size);
				m_buf.emplace_back(m_factory->create());
			}
			else
			{
				assert(hwm() > poff());
			}
			++m_current;
		}

		sync_current_segment();
	}


	virtual int_type
	overflow(int_type ch = traits_type::eof()) override
	{
		int_type result = traits_type::not_eof(ch);
		if (!traits_type::eq_int_type(ch, traits_type::eof()))
		{
			try
			{
				if (pptr() >= epptr())
				{
					assert(pptr() >= epptr());
					next_segment();
				}
				*pptr() = ch;
				pbump(1);
			}
			catch (...)
			{
				result = traits_type::eof();
			}
		}
		return result;
	}

	virtual pos_type
	seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::out) override
	{
		return seekoff(pos, std::ios_base::beg, which);
	}

	virtual pos_type
	seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::out) override
	{
		pos_type result{off_type{-1}};
		if (m_buf.size() > 0)
		{
			ASSERT_VALID_QPPTRS(*this);
			sync_hwm();
			off_type new_off{0};
			switch (way)
			{
				case std::ios_base::beg:
					new_off = 0;
					break;
				case std::ios_base::cur:
					new_off = poff();
					break;
				case std::ios_base::end:
					new_off = hwm();
					break;
				default:
					new_off = -1;
			}
			if (new_off >= 0)
			{
				new_off += off;
				if (new_off >= 0 && new_off <= hwm())
				{
					result = pos_type{locate(new_off)};
				}
			}
		}
		ASSERT_VALID_QPPTRS(*this);
		return result;
	}

	void
	sync_current_segment()
	{
		char_type* base = reinterpret_cast<char_type*>(m_buf[m_current].data());
		assert(m_buf[m_current].capacity() == m_alloc_size);
		setp(base, base + m_alloc_size);
		m_base_offset = m_current * m_alloc_size;
	}


	off_type
	locate(off_type loc)
	{
		assert(m_buf.size() > 0);
		assert(loc <= hwm());
		assert(loc >= 0);

		auto new_current = loc / m_alloc_size;
		auto seg_off     = loc % m_alloc_size;
		if (seg_off == 0)
		{
			if (new_current > 0)
			{
				--new_current;
				seg_off = m_alloc_size;
			}
		}
		if (new_current < m_buf.size())
		{
			m_current = new_current;
			sync_current_segment();
			pbump(seg_off);
		}
		return loc;
	}
};

class imemqbuf : public std::streambuf
{
public:
	using segment_type = util::const_buffer;
	using buffer_type  = std::deque<segment_type>;
	using off_type     = std::streamoff;
	using pos_type     = std::streampos;
	using index_type   = std::int32_t;

private:
	buffer_type           m_buf;
	std::streamsize       m_size;
	index_type            m_current;
	off_type              m_base_offset;
	std::vector<off_type> m_offsets;

	template<class Buffer>
	inline void
	move_segments(std::deque<Buffer>&& buf)
	{
		m_offsets.reserve(buf.size());
		m_size          = 0;
		using iter_type = typename std::deque<Buffer>::iterator;
		for (auto it = buf.begin(); it != buf.end(); ++it)
		{
			if (it->size() > 0)
			{
				std::move_iterator<iter_type> move_it{it};
				m_offsets.push_back(m_size);
				m_size += it->size();
				m_buf.emplace_back(*move_it);
			}
		}
		buf.clear();
	}

	imemqbuf() : m_buf{}, m_size{0}, m_current{-1}, m_base_offset{-1}, m_offsets{}
	{
		pubimbue(std::locale::classic());
	}

public:
	imemqbuf(buffer_type&& buf) : imemqbuf{}
	{
		move_segments(std::move(buf));

		if (m_buf.size() == 0)
		{
			setg(nullptr, nullptr, nullptr);
		}
		else
		{
			m_current = 0;
			sync_current_segment();
		}
		ASSERT_VALID_QGPTRS(*this);
	}

	imemqbuf(std::deque<mutable_buffer>&& buf) : imemqbuf{}
	{
		move_segments(std::move(buf));

		if (m_buf.size() == 0)
		{
			setg(nullptr, nullptr, nullptr);
		}
		else
		{
			m_current      = 0;
			sync_current_segment();
		}
		ASSERT_VALID_QGPTRS(*this);
	}

	imemqbuf(imemqbuf&& rhs) : imemqbuf{}
	{
		ASSERT_VALID_QGPTRS(rhs);
		move_segments(std::move(rhs.m_buf));
		if (m_buf.size() == 0)
		{
			setg(nullptr, nullptr, nullptr);
		}
		else
		{
			m_current = rhs.m_current;
			sync_current_segment();
			gbump(rhs.gptr() - rhs.eback());
		}
		rhs.reset_ptrs_offsets();
		ASSERT_VALID_QGPTRS(*this);
		ASSERT_VALID_QGPTRS(rhs);
	}

	imemqbuf&
	operator=(imemqbuf&& rhs)
	{
		ASSERT_VALID_QGPTRS(rhs);
		move_segments(std::move(rhs.m_buf));
		if (m_buf.size() == 0)
		{
			setg(nullptr, nullptr, nullptr);
		}
		else
		{
			m_current = rhs.m_current;
			sync_current_segment();
			gbump(rhs.gptr() - rhs.eback());
		}
		rhs.reset_ptrs_offsets();
		ASSERT_VALID_QGPTRS(rhs);
		ASSERT_VALID_QGPTRS(rhs);
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
		reset_ptrs_offsets();
		return std::move(m_buf);
	}

	// for debugging/test:

	void
	print_state()
	{
		std::ptrdiff_t gptr_diff  = gptr() - eback();
		std::ptrdiff_t egptr_diff = egptr() - eback();
		std::cout << "segment count: " << m_buf.size() << ", size: " << m_size << ",  current segment: " << m_current
				  << ", current segment offset: " << m_base_offset << ", current segment size: " << egptr_diff
				  << ", current position offset: " << goff() << ", eback: " << (void*)eback()
				  << ", gptr offset: " << gptr_diff << std::endl;
		m_buf[m_current].dump(std::cout);
		std::cout.flush();
	}

	std::streamsize
	size()
	{
		return m_size;
	}

	off_type
	position()
	{
		return goff();
	}

protected:
	off_type
	goff() const
	{
		off_type result{-1};
		if (m_buf.size() > 0)
		{
			result = m_base_offset + (gptr() - eback());
		}
		return result;
	}

	void
	reset_ptrs_offsets()
	{
		setg(nullptr, nullptr, nullptr);
		m_size        = 0;
		m_current     = -1;
		m_base_offset = -1;
		m_offsets.clear();
	}

	void
	sync_current_segment()
	{
		auto&      segment = m_buf[m_current];
		char_type* base    = reinterpret_cast<char_type*>(const_cast<byte_type*>(segment.data()));
		assert(segment.size() > 0);
		setg(base, base, base + segment.size());
		m_base_offset = m_offsets[m_current];
	}


	virtual std::streamsize
	xsgetn(char_type* dst, std::streamsize n) override
	{
		std::streamsize result = 0;
		if (n < 1)
		{
			goto exit;
		}
		// optimize for the available case
		if (n < egptr() - gptr())
		{
			::memcpy(dst, gptr(), n);
			gbump(n);
			result = n;
		}
		else
		{
			char_type* p    = dst;
			char_type* endp = dst + n;
			while (p < endp)
			{
				if (gptr() >= egptr())
				{
					assert(gptr() == egptr());
					if (traits_type::eq_int_type(underflow(), traits_type::eof()))    // TODO: HERE!!!
					{
						goto exit;
					}
				}
				size_type chunk_size = std::min((egptr() - gptr()), (endp - p));
				if (chunk_size < 1)
					break;
				::memcpy(p, gptr(), chunk_size);
				p += chunk_size;
				result += chunk_size;
				gbump(chunk_size);
			}
			result = static_cast<size_type>(p - dst);
		}
	exit:
		return result;
	}

	off_type
	locate(off_type loc)
	{
		assert(m_buf.size() > 0);
		assert(loc <= m_size);
		assert(loc >= 0);

		off_type left{0};
		off_type right{static_cast<off_type>(m_buf.size())};
		bool     found{false};
		while (left <= right)
		{
			auto middle{(left + right) / 2};
			if (loc < m_offsets[middle])
			{
				right = middle - 1;
			}
			else if (loc > m_offsets[middle] + m_buf[middle].size())
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

		sync_current_segment();
		off_type seg_off = (loc - m_base_offset);
		gbump(seg_off);
		return loc;
	}

	virtual pos_type
	seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) override
	{
		return seekoff(pos, std::ios_base::beg, which);
	}

	virtual pos_type
	seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in) override
	{
		pos_type result{off_type{-1}};
		ASSERT_VALID_QGPTRS(*this);
		if (m_buf.size() > 0)
		{
			off_type new_off{0};
			switch (way)
			{
				case std::ios_base::beg:
					new_off = 0;
					break;
				case std::ios_base::cur:
					new_off = goff();
					break;
				case std::ios_base::end:
					new_off = m_size;
					break;
				default:
					return new_off = -1;
			}
			if (new_off >= 0)
			{
				new_off += off;
				if (new_off >= 0 && new_off <= m_size)
				{
					result = pos_type{locate(new_off)};
				}
			}
		}
		return result;
	}

	virtual int_type
	underflow() override
	{
		int_type result = traits_type::eof();
		if (gptr() >= egptr())
		{
			assert(gptr() == egptr());
			if (m_current < m_buf.size() - 1)
			{
				++m_current;

				sync_current_segment();
				result = traits_type::to_int_type(*gptr());
			}
		}
		else
		{
			result = traits_type::to_int_type(*gptr());
		}
		return result;
	}

	char_type
	previous_char()
	{
		auto& prev_seg = m_buf[m_current - 1];
		return *reinterpret_cast<const char_type*>(prev_seg.data() + (prev_seg.size() - 1));
	}

	virtual int_type
	pbackfail(int_type ch = traits_type::eof()) override
	{
		int_type result = traits_type::eof();

		if (gptr() > eback())
		{
			if (traits_type::eq_int_type(ch, traits_type::eof())
				|| traits_type::eq(traits_type::to_char_type(ch), gptr()[-1]))
			{
				setg(eback(), gptr() - 1, egptr());
				result = traits_type::not_eof(ch);
			}
		}
		else if (m_current > 0)
		{
			auto prev_ch = previous_char();
			if (traits_type::eq_int_type(ch, traits_type::eof())
				|| traits_type::eq(traits_type::to_char_type(ch), prev_ch))
			{
				--m_current;

				sync_current_segment();
				gbump((egptr() - eback()) - 1);
				assert(prev_ch == *gptr());
				result = traits_type::not_eof(traits_type::to_int_type(prev_ch));
			}
		}
		return result;
	}
};

}    // namespace util

#endif    // UTIL_MEMBUF_H
