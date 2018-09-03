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

#ifndef LOGICMILL_BSTREAM_IFBSTREAM_H
#define LOGICMILL_BSTREAM_IFBSTREAM_H

#include <logicmill/bstream/ibstream.h>
#include <logicmill/bstream/ibfilebuf.h>
#include <logicmill/bstream/utils/memory.h>
#include <fstream>
#include <system_error>

namespace logicmill
{
namespace bstream
{

class ifbstream : public ibstream
{
public:

    ifbstream( context_base const& cntxt = get_default_context() )
    :
    ibstream{ std::make_unique< ibfilebuf >(), cntxt }
    {}

    ifbstream( ifbstream const& ) = delete;
    ifbstream( ifbstream&& ) = delete;

    ifbstream( std::unique_ptr< ibfilebuf > fbuf, context_base const& cntxt = get_default_context() )
    : ibstream{ std::move( fbuf ), cntxt }
    {}

    ifbstream( ibfilebuf&& fbuf, context_base const& cntxt = get_default_context() )
    :
    ibstream{ std::make_unique< ibfilebuf >( std::move( fbuf ) ), cntxt }
    {}

    ifbstream( std::string const& filename, context_base const& cntxt = get_default_context() )
    :
    ibstream{ std::make_unique< ibfilebuf >( filename ), cntxt }
    {}

    ifbstream( std::string const& filename, std::error_code& err, context_base const& cntxt = get_default_context() )
    :
    ibstream{ std::make_unique< ibfilebuf >( filename, err ), cntxt }
    {}

    void
    open( std::string const& filename )
    {
        get_filebuf().open( filename );
    }

    void
    open( std::string const& filename, std::error_code& err )
    {
        get_filebuf().open( filename, err );
    }

    bool
    is_open() const
    {
        return get_filebuf().is_open();
    }

    void
    close()
    {
        get_filebuf().close();
    }

    void
    close( std::error_code& err )
    {
        get_filebuf().close( err );
    }

    ibfilebuf&
    get_filebuf()
    {
        return reinterpret_cast< ibfilebuf& >( get_streambuf() );
    }

    ibfilebuf const&
    get_filebuf() const
    {
        return reinterpret_cast< ibfilebuf const& >( get_streambuf() );
    }

    std::unique_ptr< ibfilebuf >
    release_filebuf()
    {
        return bstream::utils::static_unique_ptr_cast< ibfilebuf >( release_streambuf() );
    }

};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_IFBSTREAM_H