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

#ifndef LOGICMILL_BSTREAM_IBMEMBUF_H
#define LOGICMILL_BSTREAM_IBMEMBUF_H

#include <logicmill/bstream/ibstreambuf.h>

namespace logicmill 
{
namespace bstream 
{

class ibmembuf : public ibstreambuf
{
public:

    using ibstreambuf::getn;

    ibmembuf( const_buffer const& buf )
    :
    ibstreambuf{},
    m_buf{ buf }
    {
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
		m_end_position = m_buf.size();
    }

    ibmembuf( const_buffer&& buf )
    :
    ibstreambuf{},
    m_buf{ std::move( buf ) }
    {
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
		m_end_position = m_buf.size();
    }

	ibmembuf( mutable_buffer&& buf )
	:
	ibstreambuf{},
	m_buf{ std::move( buf ) }
	{
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
		m_end_position = m_buf.size();
	}

    const_buffer
    get_buffer()
    {
        return m_buf;
    }

	const_buffer&
	get_buffer_ref()
	{
		return m_buf;
	}

    const_buffer
    get_slice( size_type n );

    virtual const_buffer
    getn( size_type n, std::error_code& err ) override;

    virtual const_buffer
    getn( size_type n ) override;

protected:

    const_buffer      m_buf;
};

} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_IBMEMBUF_H