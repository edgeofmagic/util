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
	using emitter<mutable_buffer_event>::emit;
	using emitter<control_event>::emit;
	using emitter<error_event>::emit;

	using base::get_surface;

	using read_handler    = std::function<void(util::const_buffer&&)>;
	using control_handler = std::function<void(control_state)>;
	using error_handler   = std::function<void(std::error_code)>;
	using void_handler = std::function<void()>;

	stream_duplex_bottom&
	get_bottom()
	{
		return get_surface<stream_duplex_bottom>();
	}

	driver() : base{}, m_is_writable{false}, m_read_active{false} {}

	driver(driver&& rhs)
		: base{},
		  m_read_handler{std::move(rhs.m_read_handler)},
		  m_error_handler{std::move(rhs.m_error_handler)},
		  m_start_handler{std::move(rhs.m_start_handler)},
		  m_stop_handler{std::move(rhs.m_stop_handler)}
	{}

	driver(driver const& rhs)
		: base{},
		  m_read_handler{rhs.m_read_handler},
		  m_error_handler{rhs.m_error_handler},
		  m_start_handler{rhs.m_start_handler},
		  m_stop_handler{rhs.m_stop_handler}
	{}

	~driver()
	{
		m_read_handler  = nullptr;
		m_error_handler = nullptr;
		m_stop_handler  = nullptr;
		m_start_handler = nullptr;
	}

	void
	write(std::deque<util::mutable_buffer>&& bufs, std::error_code& err)
	{
		err.clear();
		if (!m_is_writable)
		{
			err = make_error_code(laps::errc::not_writable);
			goto exit;
		}
		emit<mutable_data_event>(std::move(bufs));
	exit:
		return;
	}

	void
	write(std::deque<util::mutable_buffer>&& bufs)
	{
		if (!m_is_writable)
		{
			throw std::system_error{make_error_code(laps::errc::not_writable)};
		}
		emit<mutable_data_event>(std::move(bufs));
	}

	void
	write(util::mutable_buffer&& buf, std::error_code& err)
	{
		err.clear();
		if (!m_is_writable)
		{
			err = make_error_code(laps::errc::not_writable);
			goto exit;
		}
		emit<mutable_buffer_event>(std::move(buf));
	exit:
		return;
	}

	void
	write(util::mutable_buffer&& buf)
	{
		if (!m_is_writable)
		{
			throw std::system_error{make_error_code(laps::errc::not_writable)};
		}
		emit<mutable_buffer_event>(std::move(buf));
	}

	bool
	is_writable() const
	{
		return m_is_writable;
	}

	// template<class T>
	// std::enable_if_t<std::is_convertible<T, read_handler>::value>
	// on_read(T&& handler)
	// {
	// 	m_read_handler = std::forward<T>(handler);
	// }

	template<class T>
	std::enable_if_t<std::is_convertible<T, read_handler>::value>
	start_read(T&& handler, std::error_code& err)
	{
		err.clear();
		if (m_read_active)
		{
			err = make_error_code(laps::errc::already_reading);
			goto exit;
		}
		m_read_handler = std::forward<T>(handler);
		m_read_active = true;
		emit<control_event>(control_state::start);
	exit:
		return;
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, read_handler>::value>
	start_read(T&& handler)
	{
		if (m_read_active)
		{
			throw std::system_error{make_error_code(laps::errc::already_reading)};
		}
		m_read_handler = std::forward<T>(handler);
		m_read_active = true;
		emit<control_event>(control_state::start);
	}

	void
	resume_read(std::error_code& err)
	{
		err.clear();
		if (m_read_active)
		{
			err = make_error_code(laps::errc::already_reading);
			goto exit;
		}
		if (!m_read_handler)
		{
			err = make_error_code(laps::errc::cannot_resume_read);
			goto exit;
		}
		m_read_active = true;
		emit<control_event>(control_state::start);
	exit:
		return;
	}

	void
	resume_read()
	{
		if (m_read_active)
		{
			throw std::system_error{make_error_code(laps::errc::already_reading)};
			goto exit;
		}
		if (!m_read_handler)
		{
			throw std::system_error{make_error_code(laps::errc::cannot_resume_read)};
			goto exit;
		}
		m_read_active = true;
		emit<control_event>(control_state::start);
	exit:
		return;
	}

	void
	stop_read(std::error_code& err)
	{
		err.clear();
		if (!m_read_active)
		{
			err = make_error_code(laps::errc::not_reading);
			goto exit;
		}
		m_read_active = false;
		emit<control_event>(control_state::stop);
	exit:
		return;
	}

	void
	stop_read()
	{
		if (!m_read_active)
		{
			throw std::system_error{make_error_code(laps::errc::not_reading)};
		}
		m_read_active = false;
		emit<control_event>(control_state::stop);
	}

	template<class T>
	std::enable_if_t<std::is_convertible_v<T, void_handler>>
	on_writable(T&& handler)
	{
		m_start_handler = std::forward<T>(handler);
	}

	template<class T>
	std::enable_if_t<std::is_convertible_v<T, void_handler>>
	on_not_writable(T&& handler)
	{
		m_stop_handler = std::forward<T>(handler);
	}

	template<class T>
	std::enable_if_t<std::is_convertible<T, error_handler>::value>
	on_error(T&& handler)
	{
		m_error_handler = std::forward<T>(handler);
	}

	void
	on(const_buffer_event, util::const_buffer&& buf)
	{
		assert(m_read_active);
		assert(m_read_handler);
		m_read_handler(std::move(buf));
	}

	void
	on(control_event, control_state cstate)
	{
		if (cstate == control_state::start)
		{
			assert(!m_is_writable);
			m_is_writable = true;
			if (m_start_handler)
			{
				m_start_handler();
			}
		}
		else if (cstate == control_state::stop)
		{
			assert(m_is_writable);
			m_is_writable = false;
			if (m_stop_handler)
			{
				m_stop_handler();
			}
		}
		else
		{
			assert(false);
		}		
	}

	void
	on(error_event, std::error_code err)
	{
		if (m_error_handler)
		{
			m_error_handler(err);
		}
	}

private:
	read_handler    m_read_handler;
	error_handler   m_error_handler;
	void_handler	m_start_handler;
	void_handler	m_stop_handler;
	bool			m_is_writable;
	bool			m_read_active;
};

}    // namespace laps
}    // namespace logicmill

#endif    // LOGICMILL_LAPS_DRIVER_H