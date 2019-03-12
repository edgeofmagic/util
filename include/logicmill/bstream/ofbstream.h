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

#ifndef LOGICMILL_BSTREAM_OFBSTREAM_H
#define LOGICMILL_BSTREAM_OFBSTREAM_H

#include <fstream>
#include <logicmill/bstream/file/sink.h>
#include <logicmill/bstream/memory.h>
#include <logicmill/bstream/obstream.h>
#include <system_error>

namespace logicmill
{
namespace bstream
{

// configure, don't open
class ofbstream : public obstream
{
public:
	ofbstream(open_mode mode = file::sink::default_mode, context_base const& cntxt = get_default_context())
		: obstream{std::make_unique<file::sink>(mode, cntxt.buffer_size(), cntxt.byte_order()), cntxt}
	{}

	ofbstream(ofbstream const&) = delete;
	ofbstream(ofbstream&&)      = delete;

	ofbstream(std::unique_ptr<file::sink> fbuf, context_base const& cntxt = get_default_context())
		: obstream{std::move(fbuf), cntxt}
	{}

	ofbstream(std::string const& filename)
		: obstream{std::make_unique<file::sink>(
						   filename,
						   file::sink::default_mode,
						   get_default_context().buffer_size(),
						   get_default_context().byte_order()),
				   get_default_context()}
	{}

	ofbstream(std::string const& filename, std::error_code& err)
		: obstream{std::make_unique<file::sink>(
						   filename,
						   file::sink::default_mode,
						   get_default_context().buffer_size(),
						   get_default_context().byte_order(),
						   err),
				   get_default_context()}
	{}

	ofbstream(std::string const& filename, open_mode mode)
		: obstream{std::make_unique<file::sink>(
						   filename,
						   mode,
						   get_default_context().buffer_size(),
						   get_default_context().byte_order()),
				   get_default_context()}
	{}

	ofbstream(std::string const& filename, open_mode mode, std::error_code& err)
		: obstream{std::make_unique<file::sink>(
						   filename,
						   mode,
						   get_default_context().buffer_size(),
						   get_default_context().byte_order(),
						   err),
				   get_default_context()}
	{}

	ofbstream(std::string const& filename, open_mode mode, context_base const& cntxt)
		: obstream{std::make_unique<file::sink>(filename, mode, cntxt.buffer_size(), cntxt.byte_order()), cntxt}
	{}

	ofbstream(std::string const& filename, open_mode mode, context_base const& cntxt, std::error_code& err)
		: obstream{std::make_unique<file::sink>(filename, mode, cntxt.buffer_size(), cntxt.byte_order(), err), cntxt}
	{}

	void
	open(std::string const& filename, open_mode mode)
	{
		get_filebuf().open(filename, mode);
	}

	void
	open(std::string const& filename, open_mode mode, std::error_code& err)
	{
		get_filebuf().open(filename, mode, err);
	}

	bool
	is_open() const
	{
		return get_filebuf().is_open();
	}

	void
	flush()
	{
		get_filebuf().flush();
	}

	void
	flush(std::error_code& err)
	{
		get_filebuf().flush(err);
	}

	void
	close()
	{
		get_filebuf().close();
	}

	void
	close(std::error_code& err)
	{
		get_filebuf().close(err);
	}

	file::sink&
	get_filebuf()
	{
		return reinterpret_cast<file::sink&>(get_streambuf());
	}

	file::sink const&
	get_filebuf() const
	{
		return reinterpret_cast<file::sink const&>(get_streambuf());
	}

	std::unique_ptr<file::sink>
	release_filebuf()
	{
		return bstream::static_unique_ptr_cast<file::sink>(release_streambuf());
	}

	position_type
	truncate(std::error_code& err)
	{
		return get_filebuf().truncate(err);
	}

	position_type
	truncate()
	{
		return get_filebuf().truncate();
	}

protected:
};

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_OFBSTREAM_H