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

#ifndef LOGICMILL_BSTREAM_OBMEMBUF_H
#define LOGICMILL_BSTREAM_OBMEMBUF_H

#include <logicmill/bstream/obstreambuf.h>
#include <logicmill/bstream/buffer.h>

#ifndef LOGICMILL_BSTREAM_DEFAULT_OBMEMBUF_SIZE
#define LOGICMILL_BSTREAM_DEFAULT_OBMEMBUF_SIZE  16384UL
#endif

namespace logicmill 
{
namespace bstream 
{

class obmembuf : public obstreambuf
{
public:

    obmembuf( size_type size = LOGICMILL_BSTREAM_DEFAULT_OBMEMBUF_SIZE )
    :
    obstreambuf{},
    m_buf{ size }
    {
        reset_ptrs();
    }

    obmembuf( mutable_buffer&& buf )
    :
    obstreambuf{},
    m_buf{ std::move( buf ) }
    {
        reset_ptrs();
    }

    obmembuf( obmembuf&& ) = delete;
    obmembuf( obmembuf const& ) = delete;
    obmembuf& operator=( obmembuf&& ) = delete;
    obmembuf& operator=( obmembuf const& ) = delete;
    
	obmembuf& 
	clear() noexcept;

	const_buffer
	get_buffer();

	mutable_buffer&
	get_buffer_ref();

	const_buffer
	release_buffer();

protected:

    virtual void
    really_flush( std::error_code& err ) override;

	virtual bool
	is_valid_position( position_type pos ) const override;

	virtual void
	really_jump( std::error_code& err ) override;

    void
    resize( size_type size )
    {
        size_type cushioned_size = ( size * 3 ) / 2;
	    // force a hard lower bound to avoid non-resizing dilemma in resizing, when cushioned == size == 1
        m_buf.expand( std::max( 16UL, cushioned_size ) );
    }

    virtual void
    really_overflow( size_type n, std::error_code& err ) override;

    void
    reset_ptrs()
    {
        auto base = m_buf.data();
        set_ptrs( base, base, base + m_buf.capacity() );
    }

    mutable_buffer			m_buf;
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_OBMEMBUF_H