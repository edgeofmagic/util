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

#ifndef LOGICMILL_LAPS_DRIVER_H
#define LOGICMILL_LAPS_DRIVER_H

#include <boost/endian/conversion.hpp>
#include <list>
#include <logicmill/async/event_flow.h>
#include <logicmill/laps/types.h>

namespace logicmill
{
namespace laps
{

class framer
{
public:
	struct frame_header
	{
		std::uint32_t size;
		std::uint32_t flags;
	};

	static constexpr std::size_t header_size = sizeof(std::uint32_t) * 2;
	
	using frame_size_type               = std::int64_t;
	static constexpr bool reverse_order = boost::endian::order::native != boost::endian::order::big;

private:
	static logicmill::bstream::mutable_buffer
	pack_frame_header(std::uint64_t frame_size)
	{
		std::uint64_t packed_frame_size = boost::endian::native_to_big(frame_size);
		return logicmill::bstream::mutable_buffer{&packed_frame_size, sizeof(frame_size)};
	}

	static std::uint64_t
	unpack_frame_header(const void* buf_ptr)
	{
		std::uint64_t packed_frame_size{0};
		::memcpy(&packed_frame_size, buf_ptr, sizeof(packed_frame_size));
		return boost::endian::big_to_native(packed_frame_size);
	}



	class bottom;

	class top : public flow::stackable<stream_duplex_top, top>
	{
	public:
		using base = flow::stackable<stream_duplex_top, top>;
		using emitter<const_data_event>::send;
		using emitter<control_event>::send;
		using emitter<error_event>::send;
		using base::get_surface;

		top(bottom* bp) : m_bottom{bp} {}

		void
		on(mutable_data_event, std::deque<bstream::mutable_buffer>&& bufs);

		void
		on(control_event, control_state state);

		void
		on(error_event, std::error_code err);

	private:
		bottom* m_bottom;
	};

	class bottom : public flow::stackable<stream_duplex_bottom, bottom>
	{
	public:
		using base = flow::stackable<stream_duplex_bottom, bottom>;
		using emitter<mutable_data_event>::send;
		using emitter<control_event>::send;
		using emitter<error_event>::send;
		using base::get_surface;

		bottom(top* tp) : m_top{tp}, m_header_byte_count{0}, m_frame_size{-1} {}

		void
		on(const_data_event, std::deque<bstream::const_buffer>&& bufs)
		{
			if (is_valid_header())
			{
				assert(m_partial_frame_size < m_current_header.size())
			}

			laps::sbuf_sequence incoming{laps::make_sbuf_sequence(std::move(bufs))};

			assert((is_frame_size_valid() && (m_payload_buffer.size() < m_frame_size)) || (!is_frame_size_valid()));

			std::size_t current_buffer_position{0};
			std::size_t remaining_in_buffer{buf.size()};

			while (remaining_in_buffer > 0)
			{
				if (!is_frame_size_valid())
				{
					assert(!is_header_complete());
					std::size_t nbytes_to_move{0};
					auto        needed_to_complete = sizeof(m_header_buf) - m_header_byte_count;
					if (buf.size() >= needed_to_complete)
					{
						nbytes_to_move = needed_to_complete;
					}
					else
					{
						nbytes_to_move = buf.size();
					}
					bstream::mutable_buffer hbuf{&m_header_buf, 8};
					::memcpy(&m_header_buf[m_header_byte_count], buf.data() + current_buffer_position, nbytes_to_move);
					m_header_byte_count += nbytes_to_move;
					current_buffer_position += nbytes_to_move;
					remaining_in_buffer -= nbytes_to_move;
					if (is_header_complete())
					{
						m_frame_size = unpack_frame_header(&m_header_buf);
						assert(m_frame_size >= 0);

						assert(m_payload_buffer.size() == 0);
						if (m_frame_size > 0)
						{
							m_payload_buffer.expand(m_frame_size);
						}
					}
				}

				assert((is_frame_size_valid() || remaining_in_buffer < 1));

				if (is_frame_size_valid())
				{
					assert(m_payload_buffer.size() <= m_frame_size);
					std::size_t needed_to_complete{m_frame_size - m_payload_buffer.size()};
					if (needed_to_complete > 0 && remaining_in_buffer > 0)
					{
						assert(m_payload_buffer.capacity() == m_frame_size);
						std::size_t nbytes_to_move = std::min(remaining_in_buffer, needed_to_complete);
						assert(nbytes_to_move > 0);
						m_payload_buffer.putn(m_payload_buffer.size(), buf.data() + current_buffer_position, nbytes_to_move);
						needed_to_complete -= nbytes_to_move;
						current_buffer_position += nbytes_to_move;
						remaining_in_buffer -= nbytes_to_move;
						m_payload_buffer.size(m_payload_buffer.size() + nbytes_to_move);
					}
					if (needed_to_complete < 1)
					{
						std::error_code err;
						m_read_handler(channel_ptr, std::move(m_payload_buffer), err);
						m_header_byte_count = 0;
						assert(m_payload_buffer.size() == 0);
						m_frame_size = -1;
					}
				}
			}
		}


	private:

		bool
		is_frame_size_valid() const
		{
			return m_frame_size >= 0;
		}

		bool
		is_header_complete() const
		{
			assert(m_header_byte_count <= sizeof(framer::frame_size_type));
			return m_header_byte_count == sizeof(framer::frame_size_type);
		}

		top*                                           m_top;
		std::size_t                                    m_header_byte_count;
		framer::frame_size_type                        m_frame_size;
		logicmill::bstream::byte_type                  m_header_buf[sizeof(framer::frame_size_type)];
		std::deque<logicmill::bstream::mutable_buffer> m_payload;
		std::size_t                                    m_payload_size;
	};

public:

	framer() : m_top{&m_bottom}, m_bottom{&m_top} {}

	stream_duplex_bottom&
	get_bottom()
	{
		return m_bottom.get_surface<stream_duplex_bottom>();
	}

	stream_duplex_top&
	get_top()
	{
		return m_top.get_surface<stream_duplex_top>();
	}

	framer(framer&& rhs) : m_top{&m_bottom}, m_bottom{&m_top} {}
	framer(framer const& rhs) : m_top{&m_bottom}, m_bottom{&m_top} {}

private:
	top m_top;
	bottom m_bottom;
};

}    // namespace laps
}    // namespace logicmill

void
logicmill::laps::framer::top::on(mutable_data_event, std::deque<bstream::mutable_buffer>&& bufs)
{
	std::uint64_t frame_size{0};
	for (auto& buf : bufs)
	{
		frame_size += buf.size();
	}
	bufs.emplace_front(framer::pack_frame_header(frame_size));
	m_bottom->send<mutable_data_event>(std::move(bufs));
}

void
logicmill::laps::framer::top::on(control_event, control_state state)
{
	m_bottom->send<control_event>(state);
}

void
logicmill::laps::framer::top::on(error_event, std::error_code err)
{
	m_bottom->send<error_event>(err);
}

#endif    // LOGICMILL_LAPS_DRIVER_H