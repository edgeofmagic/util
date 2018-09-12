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

#ifndef LOGICMILL_BSTREAM_OBFILEBUF_H
#define LOGICMILL_BSTREAM_OBFILEBUF_H

#include <fcntl.h>
#include <vector>
#include <logicmill/bstream/obstreambuf.h>
#include <logicmill/buffer.h>

#ifndef LOGICMILL_BSTREAM_DEFAULT_OBFILEBUF_SIZE
#define LOGICMILL_BSTREAM_DEFAULT_OBFILEBUF_SIZE  16384UL
#endif

namespace logicmill 
{
namespace bstream 
{

class obfilebuf : public obstreambuf
{
public:

    static constexpr open_mode default_mode = open_mode::at_begin;

    obfilebuf( obfilebuf&& rhs );

    obfilebuf( std::string const& filename, open_mode mode, std::error_code& err, size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_OBFILEBUF_SIZE );

    obfilebuf( std::string const& filename, open_mode mode = default_mode, size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_OBFILEBUF_SIZE );

    obfilebuf( open_mode mode = default_mode, size_type buffer_size = LOGICMILL_BSTREAM_DEFAULT_OBFILEBUF_SIZE );

    void
    open( std::string const& filename, open_mode mode, std::error_code& err );

    void
    open( std::string const& filename, open_mode mode = default_mode );

    void
    open( std::string const& filename, open_mode mode, int flags_override, std::error_code& err );

    void
    open( std::string const& filename, open_mode mode, int flags_override );

    bool
    is_open() const noexcept
    {
        return m_is_open;
    }

    void
    close( std::error_code& err );

    void
    close();

    void
    open( std::error_code& err )
    {
        really_open( err );
    }

    void
    open();

    open_mode
    mode() const noexcept
    {
        return m_mode;
    }

    void
    mode( open_mode m )
    {
        m_mode = m;
        m_flags = to_flags( m );
    }

    int
    flags() const noexcept
    {
        return m_flags;
    }

    void
    flags( int flags )
    {
        m_flags = flags;
    }

    void
    filename( std::string const& filename )
    {
        m_filename = filename;
    }

    std::string const&
    filename() const noexcept
    {
        return m_filename;
    }

    position_type
    truncate( std::error_code& err );

    position_type
    truncate();

protected:

    // virtual bool
    // really_force_mutable() override;

    virtual void
    really_flush( std::error_code& err ) override;

	virtual bool
	is_valid_position( position_type pos ) const override;

	virtual void
	really_jump( std::error_code& err ) override;

    virtual void
    really_overflow( size_type, std::error_code& err ) override;

private:

    static bool
    is_truncate( int flags )
    {
        return ( flags & O_TRUNC ) != 0;
    }

    static bool
    is_append( int flags )
    {
        return ( flags & O_APPEND ) != 0;
    }

    void
    reset_ptrs()
    {
        byte_type* base = m_buf.data();
        set_ptrs( base, base, base + m_buf.size() );
    }

    void 
    really_open( std::error_code& err );

    constexpr int
    to_flags( open_mode mode )
    {
        switch ( mode )
        {
            case open_mode::append:
                return O_WRONLY | O_CREAT | O_APPEND;
            case open_mode::truncate:
                return O_WRONLY | O_CREAT | O_TRUNC;
            default:
                return O_WRONLY | O_CREAT;
        }
    }

    mutable_buffer					m_buf;
    std::string						m_filename;
    bool							m_is_open;
    open_mode						m_mode;
    int								m_flags;
    int								m_fd;
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_OBMEMBUF_H