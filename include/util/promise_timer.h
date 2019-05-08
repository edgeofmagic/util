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

#ifndef ARMI_PROMISE_TIMER_H
#define ARMI_PROMISE_TIMER_H

#include <chrono>
#include <functional>
#include <util/promise.h>

template<class AsyncAdapter>
class util::promise_timer
{
public:
	using async_io         = AsyncAdapter;
	using timer_type       = typename async_io::timer_type;
	using timer_param_type = typename async_io::timer_param_type;
	using loop_type        = typename async_io::loop_type;
	using loop_param_type  = typename async_io::loop_param_type;

	promise_timer(std::chrono::milliseconds t, loop_param_type lp) : m_timeout{t}, m_loop{lp} {}

	template<class T>
	void
	operator()(promise<T>& p)
	{
		p.cancel_timer();
		if (p.m_shared)
		{
			std::error_code err;
			auto            tp       = async_io::create_timer(m_loop);
			p.m_shared->cancel_timer = [tp]() { async_io::cancel_timer(tp); };
			async_io::start_timer(tp, m_timeout, [p](std::error_code const& err) mutable {
				p.cancel_timer();
				if (!p.is_finished())
				{
					if (p.m_shared && p.m_shared->on_timeout)
					{
						auto timeout = std::move(p.m_shared->on_timeout);
						timeout(err);
					}
					else
					{
						p.reject(err);    // if timer worked properly, errc == timed_out
					}
				}
			});
		}
	}

private:
	std::chrono::milliseconds m_timeout;
	loop_type                 m_loop;
};

#endif    // ARMI_PROMISE_TIMER_H