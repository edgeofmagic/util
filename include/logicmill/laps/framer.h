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
#include <logicmill/bstream/buffer/sink.h>
#include <logicmill/bstream/bufseq/source.h>
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
	friend class top;
	friend class bottom;

	class top : public flow::stackable<frame_duplex_top, top>, public face<framer, top>
	{
	public:
		using connector_base = flow::stackable<frame_duplex_top, top>;
		using base           = face<framer, top>;
		using flow::emitter<shared_frame_event>::emit;
		using flow::emitter<control_event>::emit;
		using flow::emitter<error_event>::emit;
		using connector_base::get_surface;

		top(framer* owner) : base{owner} {}

		void
		on(mutable_frame_event, mutable_frame&& frm);

		void
		on(control_event, control_state state);

		void
		on(error_event, std::error_code err);
	};

	class bottom : public flow::stackable<stream_duplex_bottom, bottom>, public face<framer, bottom>
	{
	public:
		using connector_base = flow::stackable<stream_duplex_bottom, bottom>;
		using base           = face<framer, bottom>;
		using emitter<mutable_data_event>::emit;
		using emitter<mutable_buffer_event>::emit;
		using emitter<control_event>::emit;
		using emitter<error_event>::emit;
		using connector_base::get_surface;

		bottom(framer* owner) : base{owner}, m_header_is_valid{false} {}

		void
		on(const_buffer_event, util::const_buffer&& buf)
		{
			m_source.append(std::move(buf));
			while (true)
			{
				if (m_header_is_valid)
				{
					if (m_source.remaining() >= m_frame_size)
					{
						owner()->m_top.emit<shared_frame_event>(
								shared_frame{m_flags, m_source.get_segmented_slice(m_frame_size)});
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
						m_frame_size      = m_source.get_num<std::uint32_t>();
						m_flags           = m_source.get_num<std::uint32_t>();
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
			if (s == control_state::start)
			{
				owner()->m_top.propagate_start();
			}
			else    // s == control_state::stop
			{
				owner()->m_top.propagate_stop();
			}
		}

		void
		on(error_event, std::error_code err)
		{
			owner()->m_top.emit<error_event>(err);
		}

	private:
		bstream::bufseq::source<util::shared_buffer> m_source;
		frame::frame_size_type                       m_frame_size;
		frame::flags_type                            m_flags;
		bool                                         m_header_is_valid;
	};

public:
	framer() : m_top{this}, m_bottom{this} {}

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

	framer(framer&& rhs) : m_top{this}, m_bottom{this} {}
	framer(framer const& rhs) : m_top{this}, m_bottom{this} {}

private:
	top    m_top;
	bottom m_bottom;
};

}    // namespace laps
}    // namespace logicmill

void
logicmill::laps::framer::top::on(mutable_frame_event, mutable_frame&& frm)
{
	bstream::buffer::sink header_sink{header_size};
	header_sink.put_num(frm.size());
	header_sink.put_num(frm.flags());
	std::deque<util::mutable_buffer> bufs = frm.release_bufs();
	bufs.emplace_front(header_sink.release_buffer());
	owner()->m_bottom.emit<mutable_data_event>(std::move(bufs));
}

void
logicmill::laps::framer::top::on(control_event, control_state state)
{
	owner()->m_bottom.emit<control_event>(state);
}

void
logicmill::laps::framer::top::on(error_event, std::error_code err)
{
	owner()->m_bottom.emit<error_event>(err);
}

#endif    // LOGICMILL_LAPS_FRAMER_H