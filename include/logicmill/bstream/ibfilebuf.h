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

#ifndef LOGICMILL_BSTREAM_IBFILEBUF_H
#define LOGICMILL_BSTREAM_IBFILEBUF_H

#include <logicmill/bstream/ibstreambuf.h>

#ifndef LOGICMILL_BSTREAM_DEFAULT_IBFILEBUF_SIZE
#define LOGICMILL_BSTREAM_DEFAULT_IBFILEBUF_SIZE  16384UL
#endif

namespace logicmill 
{
namespace bstream 
{

class ibfilebuf : public ibstreambuf
{
public:

    ibfilebuf( size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_IBFILEBUF_SIZE );

    ibfilebuf( std::string const& filename, std::error_code& err, int flag_overrides = 0, size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_IBFILEBUF_SIZE );

    ibfilebuf( std::string const& filename, int flag_overrides = 0, size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_IBFILEBUF_SIZE );

    void
    open( std::string const& filename, std::error_code& err, int flag_overrides = 0 );

    void
    open( std::string const& filename, int flag_overrides = 0 );

    bool
    is_open() const noexcept
    {
        return m_is_open;
    }
    
    void
    close( std::error_code& err );

    void
    close();

protected:

    virtual position_type
    really_seek( seek_anchor where, offset_type offset, std::error_code& err ) override;

    virtual position_type
    really_tell( seek_anchor where, std::error_code& err ) override;

    virtual size_type
    really_underflow( std::error_code& err ) override;

private:

    size_type
    load_buffer( std::error_code& err );

    void
    reset_ptrs()
    {
        const byte_type* base = m_buf.data();
        set_ptrs( base, base, base );
    }

    void 
    really_open( std::error_code& err );

    mutable_buffer		m_buf;
    std::string			m_filename;
    bool				m_is_open;
    int					m_flags;
    int					m_fd;
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_IBFILEBUF_H