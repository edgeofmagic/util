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

#include <list>
#include <logicmill/async/event_flow.h>
#include <logicmill/laps/types.h>

namespace logicmill
{
namespace laps
{

class driver : public flow::stackable<stream_duplex_bottom, driver>
{
public:
	using base = flow::stackable<stream_duplex_bottom, driver>;
	using emitter<mutable_data_event>::emit;
	using emitter<control_event>::emit;
	using emitter<error_event>::emit;

	using base::get_surface;

	using read_handler    = std::function<void(std::deque<util::const_buffer>&&)>;
	using control_handler = std::function<void(control_state)>;
	using error_handler   = std::function<void(std::error_code)>;

	stream_duplex_bottom&
	get_bottom()
	{
		return get_surface<stream_duplex_bottom>();
	}

	driver() : base{} {}

	driver(driver&& rhs)
		: base{},
		  m_read_handler{std::move(rhs.m_read_handler)},
		  m_control_handler{std::move(rhs.m_control_handler)},
		  m_error_handler{std::move(rhs.m_error_handler)}
	{}

	driver(driver const& rhs)
		: base{},
		  m_read_handler{rhs.m_read_handler},
		  m_control_handler{rhs.m_control_handler},
		  m_error_handler{rhs.m_error_handler}
	{}

	~driver()
	{
		m_read_handler    = nullptr;
		m_control_handler = nullptr;
		m_error_handler   = nullptr;
	}

	void
	write(std::deque<util::mutable_buffer>&& bufs)
	{
		emit<mutable_data_event>(std::move(bufs));
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, read_handler>::value>
	on_read(T&& handler)
	{
		m_read_handler = std::forward<T>(handler);
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, read_handler>::value>
	start_read(T&& handler)
	{
		m_read_handler = std::forward<T>(handler);
		emit<control_event>(control_state::start);
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, control_handler>::value>
	on_control(T&& handler)
	{
		m_control_handler = std::forward<T>(handler);
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, error_handler>::value>
	on_error(T&& handler)
	{
		m_error_handler = std::forward<T>(handler);
	}

	void
	stop_read()
	{
		emit<control_event>(control_state::stop);
	}

	void
	on(const_data_event, std::deque<util::const_buffer>&& bufs)
	{
		m_read_handler(std::move(bufs));
	}

	void
	on(control_event, control_state cstate)
	{
		m_control_handler(cstate);
	}

	void
	on(error_event, std::error_code err)
	{
		m_error_handler(err);
	}

private:
	read_handler    m_read_handler;
	control_handler m_control_handler;
	error_handler   m_error_handler;
};

}    // namespace laps
}    // namespace logicmill

#endif    // LOGICMILL_LAPS_DRIVER_H