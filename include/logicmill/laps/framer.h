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

#ifndef LOGICMILL_LAPS_FRAMER_H
#define LOGICMILL_LAPS_FRAMER_H

#include <boost/endian/conversion.hpp>
#include <list>
#include <logicmill/async/event_flow.h>
#include <logicmill/bstream/compound_memory/source.h>
#include <logicmill/bstream/memory/sink.h>
#include <logicmill/laps/types.h>

namespace logicmill
{
namespace laps
{

	class framer
	{
	public:
		static constexpr std::size_t header_size   = sizeof(std::uint32_t) * 2;
		static constexpr bool        reverse_order = boost::endian::order::native != boost::endian::order::big;

	private:
		class bottom;

		class top : public flow::stackable<frame_duplex_top, top>
		{
		public:
			using base = flow::stackable<frame_duplex_top, top>;
			using flow::emitter<shared_frame_event>::send;
			using flow::emitter<control_event>::send;
			using flow::emitter<error_event>::send;
			using base::get_surface;

			top(bottom* bp) : m_bottom{bp} {}

			void
			on(mutable_frame_event, mutable_frame&& frm);

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

			bottom(top* tp) : m_top{tp}, m_header_is_valid{false} {}

			void
			on(const_data_event, std::deque<bstream::const_buffer>&& bufs)
			{
				// std::cout << "bufs size is : " << bufs.size() << "[" << bufs[0].size() << ", " << bufs[1].size() << "]" << std::endl;
				// bufs[0].dump(std::cout);
				// bufs[1].dump(std::cout);
				m_source.append(std::move(bufs));
				while (true)
				{
					if (m_header_is_valid)
					{
						if (m_source.remaining() >= m_frame_size)
						{
							// shared_frame frm{m_flags, m_source.get_segmented_slice(m_frame_size)};
							m_top->send<shared_frame_event>(shared_frame{m_flags, m_source.get_segmented_slice(m_frame_size)});
							m_header_is_valid = false;
						}
						else
						{
							break;
						}
					}
					else
					{
						if (m_source.remaining() >= header_size)
						{
							m_frame_size     = m_source.get_num<std::uint32_t>(reverse_order);
							m_flags    = m_source.get_num<std::uint32_t>(reverse_order);
							m_header_is_valid = true;
						}
						else
						{
							break;
						}
					}
				}
				m_source.trim();
			}

			void
			on(control_event, control_state s)
			{
				m_top->send<control_event>(s);
			}

			void
			on(error_event, std::error_code err)
			{
				m_top->send<error_event>(err);
			}


		private:
			top*                                                     m_top;
			bstream::compound_memory::source<bstream::shared_buffer> m_source;
			frame::frame_size_type                                   m_frame_size;
			frame::flags_type                                        m_flags;
			bool                                                     m_header_is_valid;
		};

	public:
		framer() : m_top{&m_bottom}, m_bottom{&m_top} {}

		stream_duplex_bottom&
		get_bottom()
		{
			return m_bottom.get_surface<stream_duplex_bottom>();
		}

		frame_duplex_top&
		get_top()
		{
			return m_top.get_surface<frame_duplex_top>();
		}

		framer(framer&& rhs) : m_top{&m_bottom}, m_bottom{&m_top} {}
		framer(framer const& rhs) : m_top{&m_bottom}, m_bottom{&m_top} {}

	private:
		top    m_top;
		bottom m_bottom;
};

}    // namespace laps
}    // namespace logicmill

void
logicmill::laps::framer::top::on(mutable_frame_event, mutable_frame&& frm)
// logicmill::laps::framer::top::on(mutable_frame_event, frame_header header, std::deque<bstream::mutable_buffer>&& bufs)
{
	bstream::memory::sink header_sink{header_size};
	header_sink.put_num(frm.size(), reverse_order);
	header_sink.put_num(frm.flags(), reverse_order);
	std::deque<bstream::mutable_buffer> bufs = frm.release_bufs();
	bufs.emplace_front(header_sink.release_buffer());
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

#endif    // LOGICMILL_LAPS_FRAMER_H