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

#ifndef LOGICMILL_BSTREAM_COMPOUND_MEMORY_SINK_H
#define LOGICMILL_BSTREAM_COMPOUND_MEMORY_SINK_H

#include <deque>
#include <logicmill/bstream/buffer.h>
#include <logicmill/bstream/sink.h>

#ifndef LOGICMILL_BSTREAM_MEMORY_DEFAULT_BUFFER_SIZE
#define LOGICMILL_BSTREAM_MEMORY_DEFAULT_BUFFER_SIZE 16384UL
#endif

namespace logicmill
{
namespace bstream
{
namespace compound_memory
{

namespace detail
{
class sink_test_probe;
}

class sink : public bstream::sink
{
public:
	using base = bstream::sink;
	using default_alloc = std::allocator<byte_type>;

	using buffers = std::deque<mutable_buffer>;

	friend class detail::sink_test_probe;

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	sink(size_type size, _Alloc&& alloc, byte_order order=byte_order::big_endian)
		: base{order},
		  m_segment_capacity{size},
		  m_current{0},
		  m_bufs{},
		  m_factory{std::make_unique<mutable_buffer_alloc_factory<_Alloc>>(std::forward<_Alloc>(alloc))}
	{
		m_bufs.emplace_back(m_factory->create(size));
		reset_ptrs();
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	sink(_Alloc&& alloc, byte_order order=byte_order::big_endian)
	: base{order},
	m_segment_capacity{LOGICMILL_BSTREAM_MEMORY_DEFAULT_BUFFER_SIZE},
	m_current{0},
	m_bufs{},
	m_factory{std::make_unique<mutable_buffer_alloc_factory<_Alloc>>(std::forward<_Alloc>(alloc))}
	{
		m_bufs.emplace_back(m_factory->create(m_segment_capacity));
		reset_ptrs();
	}

	sink(size_type size, byte_order order=byte_order::big_endian)
		: base{order},
		  m_segment_capacity{size},
		  m_current{0},
		  m_bufs{},
		  m_factory{std::make_unique<mutable_buffer_alloc_factory<default_alloc>>()}
	{
		m_bufs.emplace_back(m_factory->create(size));
		reset_ptrs();
	}

	sink(byte_order order=byte_order::big_endian)
		: base{order},
		  m_segment_capacity{LOGICMILL_BSTREAM_MEMORY_DEFAULT_BUFFER_SIZE},
		  m_current{0},
		  m_bufs{},
		  m_factory{std::make_unique<mutable_buffer_alloc_factory<default_alloc>>()}
	{
		m_bufs.emplace_back(m_factory->create(m_segment_capacity));
		reset_ptrs();
	}



	sink(sink&&)      = delete;
	sink(sink const&) = delete;
	sink&
	operator=(sink&&)
			= delete;
	sink&
	operator=(sink const&)
			= delete;

	sink&
	clear() noexcept;

	buffers&
	get_buffers();

	buffers
	release_buffers();

protected:
	/** Sets size of tail buffer based on current high watermark.
	*
	*/
	void
	set_size();

	void
	locate(position_type pos, std::error_code& err);

	// virtual void
	// really_flush( std::error_code& err ) override;

	virtual bool
	is_valid_position(position_type pos) const override;

	virtual void
	really_jump(std::error_code& err) override;

	virtual void
	really_overflow(size_type, std::error_code& err) override;

	void
	reset_ptrs()
	{
		m_base_offset   = m_current * m_segment_capacity;
		byte_type* base = m_bufs[m_current].data();
		set_ptrs(base, base, base + m_segment_capacity);
	}

	size_type                               m_segment_capacity;    // capacity of individual buffers
	size_type                               m_current;             // index of current buffer
	buffers                                 m_bufs;
	std::unique_ptr<mutable_buffer_factory> m_factory;
};

}    // namespace compound_memory
}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_COMPOUND_MEMORY_SINK_H