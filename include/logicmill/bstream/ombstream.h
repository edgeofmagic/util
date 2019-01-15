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

#ifndef LOGICMILL_BSTREAM_OMBSTREAM_H
#define LOGICMILL_BSTREAM_OMBSTREAM_H

#include <logicmill/bstream/buffer/sink.h>
#include <logicmill/bstream/obstream.h>

namespace logicmill
{
namespace bstream
{

class ombstream : public obstream
{
public:
	ombstream()                 = delete;
	ombstream(ombstream const&) = delete;
	ombstream(ombstream&&)      = delete;

	ombstream(std::unique_ptr<buffer::sink> strmbuf, context_base const& cntxt = get_default_context())
		: obstream{std::move(strmbuf), cntxt}
	{}

	ombstream(util::mutable_buffer&& buf, context_base const& cntxt = get_default_context())
		: obstream{std::make_unique<buffer::sink>(std::move(buf), cntxt.get_context_impl()->byte_order()), cntxt}
	{}

	ombstream(size_type size, context_base const& cntxt = get_default_context())
		: ombstream(std::make_unique<buffer::sink>(size, cntxt.get_context_impl()->byte_order()), cntxt)
	{}

	ombstream(context_base const& cntxt = get_default_context())
		: ombstream{std::make_unique<buffer::sink>(
							util::mutable_buffer{
									cntxt.get_context_impl()->buffer_size(),
							},
							cntxt.get_context_impl()->byte_order()),
					cntxt}
	{}

	util::const_buffer
	get_buffer()
	{
		return get_membuf().get_buffer();
	}

	util::mutable_buffer&
	get_buffer_ref()
	{
		return get_membuf().get_buffer_ref();
	}

	util::const_buffer
	release_buffer()
	{
		return get_membuf().release_buffer();
	}

	void
	clear()
	{
		get_membuf().clear();
	}

	util::mutable_buffer
	release_mutable_buffer()
	{
		return get_membuf().release_mutable_buffer();
	}

	buffer::sink&
	get_membuf()
	{
		return reinterpret_cast<buffer::sink&>(*m_strmbuf);
	}
};

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_OMBSTREAM_H