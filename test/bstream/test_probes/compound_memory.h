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

#ifndef LOGICMILL_TEST_BSTREAM_TEST_PROBES_COMPOUND_MEMORY_H
#define LOGICMILL_TEST_BSTREAM_TEST_PROBES_COMPOUND_MEMORY_H

#include "base.h"
#include <logicmill/bstream/compound_memory/sink.h>
#include <logicmill/bstream/compound_memory/source.h>


namespace logicmill
{
namespace bstream
{
namespace compound_memory
{
namespace detail
{

template<class Buffer>
class source_test_probe : public bstream::detail::source_test_probe
{
public:
	source_test_probe(bstream::compound_memory::source<Buffer>& target)
		: bstream::detail::source_test_probe{target}, m_target{target}
	{}

	bstream::size_type
	current_segment() const
	{
		return m_target.m_current;
	}

	std::vector<bstream::size_type> const&
	offsets() const
	{
		return m_target.m_offsets;
	}

private:
	bstream::compound_memory::source<Buffer>& m_target;
};

class sink_test_probe : public bstream::detail::sink_test_probe
{
public:
	sink_test_probe(compound_memory::sink& target) : bstream::detail::sink_test_probe{target}, m_target{target} {}

	bstream::size_type current_segnemt() const
	{
		return m_target.m_current;
	}

private:
	compound_memory::sink& m_target;
};

}    // namespace detail
}    // namespace compound_memory
}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_TEST_BSTREAM_TEST_PROBES_COMPOUND_MEMORY_H