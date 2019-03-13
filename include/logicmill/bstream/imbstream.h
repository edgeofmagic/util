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

#ifndef LOGICMILL_BSTREAM_IMBSTREAM_H
#define LOGICMILL_BSTREAM_IMBSTREAM_H

#include <logicmill/bstream/buffer/source.h>
#include <logicmill/bstream/ibstream.h>

namespace logicmill
{
namespace bstream
{

class imbstream : public ibstream
{
public:
	imbstream()                 = delete;
	imbstream(imbstream const&) = delete;
	imbstream(imbstream&&)      = delete;

	using source_type = buffer::source<util::shared_buffer>;

	imbstream(std::unique_ptr<source_type> source, context_base const& context = get_default_context())
		: ibstream{std::move(source), context}
	{}

	imbstream(util::buffer const& buf, context_base const& context = get_default_context())
		: ibstream(std::make_unique<source_type>(buf), context)
	{}

	imbstream(util::buffer&& buf, context_base const& context = get_default_context())
		: ibstream{std::make_unique<source_type>(std::move(buf)), context}
	{}

	void
	use(std::unique_ptr<source_type> source)
	{
		ibstream::use(std::move(source));
		reset();
	}

	void
	use(std::unique_ptr<source_type> source, std::error_code& err)
	{
		ibstream::use(std::move(source));
		reset(err);
	}

	void
	use(util::mutable_buffer&& buf)
	{
		ibstream::use(std::make_unique<source_type>(std::move(buf)));
		reset();
	}

	void
	use(util::const_buffer&& buf)
	{
		ibstream::use(std::make_unique<source_type>(std::move(buf)));
		reset();
	}

	void
	use(util::const_buffer const& buf)
	{
		ibstream::use(std::make_unique<source_type>(buf));
		reset();
	}

	void
	use(util::mutable_buffer&& buf, std::error_code& err)
	{
		ibstream::use(std::make_unique<source_type>(std::move(buf)));
		reset(err);
	}

	void
	use(util::const_buffer&& buf, std::error_code& err)
	{
		ibstream::use(std::make_unique<source_type>(std::move(buf)));
		reset(err);
	}

	void
	use(util::const_buffer const& buf, std::error_code& err)
	{
		ibstream::use(std::make_unique<source_type>(buf));
		reset(err);
	}

	util::const_buffer
	get_buffer()
	{
		return get_source().get_buffer();
	}

	source_type&
	get_source()
	{
		return reinterpret_cast<source_type&>(*m_source);
	}
};

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_IMBSTREAM_H
