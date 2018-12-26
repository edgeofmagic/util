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

#ifndef LOGICMILL_ASYNC_STREAM_H
#define LOGICMILL_ASYNC_STREAM_H

#include <chrono>
#include <functional>
#include <logicmill/async/error.h>
#include <logicmill/async/event.h>
#include <logicmill/bstream/buffer.h>
#include <memory>
#include <system_error>

namespace logicmill
{
namespace async
{
namespace stream
{

using id_type = std::uint64_t;

enum class stream_event
{
	data,
	cancel,
	control
};

enum class control_state
{
	pause,
	resume,
	shutdown
};

using receipt = std::function<void(id_type id, std::error_code err)>;

template<class Payload>
using data_event = event<stream_event, stream_event::data, Payload, id_type, receipt>;
using cancel_event = event<stream_event, stream_event::cancel, id_type>;
using control_event = event<stream_event, stream_event::control, control_state>;

template<class Payload>
using data_out_connector = connector<source<data_event<Payload>>, source<cancel_event>, sink<control_event>>;

template<class Payload>
using data_in_connector = typename complement<data_out_connector<Payload>>::type;

template<class Payload, class Derived>
using data_out = connectable<data_out_connector<Payload>, Derived>;

template<class Payload, class Derived>
using data_in = connectable<data_in_connector<Payload>, Derived>;

template<class Payload>
class duplex_connector
{
public:
	duplex_connector(data_out_connector<Payload> const& out, data_in_connector<Payload> const& in)
		: m_out_connector{out}, m_in_connector{in}
	{}

	template<class In, class Out>
	duplex_connector(Out&& out, In&& in) : m_out_connector{std::forward<Out>(out)}, m_in_connector{std::forward<In>(in)}
	{}

	void
	mate(duplex_connector& other)
	{
		m_out_connector.mate(other.m_in_connector);
		m_in_connector.mate(other.m_out_connector);
	}

private:
	data_out_connector<Payload> m_out_connector;
	data_in_connector<Payload> m_in_connector;
};

template<class Payload, class Derived>
class duplex : public data_in<Payload, Derived>, public data_out<Payload, Derived>
{
public:
	using data_emitter = typename data_out<Payload, Derived>::template emitter<data_event<Payload>>;
	using cancel_emitter = typename data_out<Payload, Derived>::template emitter<cancel_event>;
	using control_emitter = typename data_in<Payload, Derived>::template emitter<control_event>;

	using data_emitter::send;
	using cancel_emitter::send;
	using control_emitter::send;

	using data_out<Payload, Derived>::get_connector;
	using data_in<Payload, Derived>::get_connector;

	duplex()
		: m_connector{data_out<Payload, Derived>::template get_connector<data_out_connector<Payload>>(),
					  data_in<Payload, Derived>::template get_connector<data_in_connector<Payload>>()}
	{}

	template<class Connector>
	typename std::enable_if_t<std::is_same<Connector, duplex_connector<Payload>>::value, duplex_connector<Payload>&>
	get_connector()
	{
		return m_connector;
	}

	template<class T>
	void
	stack(T& other)
	{
		m_connector.mate(other.template get_connector<duplex_connector<Payload>>());
	}

private:
	duplex_connector<Payload> m_connector;
};

}    // namespace stream
}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ASYNC_STREAM_H