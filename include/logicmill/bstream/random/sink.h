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
#ifndef LOGICMILL_BSTREAM_RANDOM_SINK_H
#define LOGICMILL_BSTREAM_RANDOM_SINK_H

#include <system_error>
#include <logicmill/bstream/types.h>

namespace logicmill 
{
namespace bstream 
{
namespace random
{

namespace detail
{
	class sink_test_probe;
}

/** Base buffer class for binary output streams
 * 
 * This class is designed to be the base class for all binary output stream buffer classes.
 * A binary output stream buffer is associated with a binary output stream object. The stream object
 * delegates write operations to the stream buffer. The stream buffer acts as an intermediary between
 * the stream object and the controlled output sequence.
 * 
 * The controlled output sequence is an abstract sequence of bytes of arbitrary length. The
 * stream buffer maintains an internal byte sequence in memory. The internal byte sequence
 * may hold the output sequence in its entirety, or it may hold a sub-sequence. The buffer
 * maintains four values that internal byte sequence and its relationship to 
 * the output sequence. There are two basic types of values to be considered: positions
 * and pointers. Positions are integral values that indicate an offset relative to the beginning
 * of the output sequence. Pointers are addresses of bytes in the internal byte sequence. 
 * 
 * pbase
 * pbase is a pointer the the first byte in the buffer's internal sequence.
 * 
 * pnext
 * pnext is a pointer the the next available (unwritten) byte in the internal sequence.
 * 
 * pend
 * pend is a pointer to the end of the internal sequence; specifically, it points the 
 * byte following the last useable byte in the internal sequence.
 * 
 * pbase_offset
 * pbase_offset is the position (that is, the offset from the beginning of the output sequence)
 * of the first byte in the internal sequence (at pbase).
 * 
 * The following invariants hold true:
 * pnext >= pbase
 * pend >= pnext
 * pbase_offset >= 0
 * 
 * Also, any pointer value p in the range (pbase <= p < pend) has a corresponding position, computed as 
 * pbase_offset + ( p - pbase ). Certain of these positions are explicitly represented in the stream
 * buffer interface:
 * 
 * ppos
 * ppos is the position corresponding to the pnext pointer, sometimes referred to as the current position
 * of the stream buffer (i.e., the position at which the next write will take place).
 * 
 * Other important operations:
 * 
 * flush
 * The flush operation synchronizes the internal sequence with the output sequence. Derived implementations
 * must provide an implementation of the virtual member function really_flush() that
 * effects synchronization for the specific output sequence type. It is important to note
 * that flush may (but does not necessarily) change the internal sequence pointers or positions.
 * 
 * dirty
 * dirty is a boolean value associated with the stream buffer. dirty is true if the buffer's internal
 * sequence contains values that have not been synchronized (flushed) with the output sequence.
 * After a flush operation, dirty is always false.
 * 
 * dirty_start
 * if dirty is true, dirty_start is a pointer indicating the first byte in the internal sequence
 * that is not synchronized with the output sequence. Unsynchronized byte are always contiguous, 
 * including bytes from dirty_start to pnext - 1. 
 * 
 * Stream buffers support the ability to set the current position explicitly, with a seek operation,
 * A derived implementation may choose not to support seeking, in which seek operations will result
 * in an error condition. The ability to seek forces additional complexity on the stream buffer's
 * interface and behavior.
 * 
 * high_water_mark
 * high_water_mark is a positional value, indicating the largest position in the output 
 * sequence that contains a valid value (essentially, the size of the output sequence).
 * If the current position (ppos) is equal to the high_water_mark, a write operation
 * will cause the high_water_mark value to increase.
 * 
 * touch
 * Some derived implementations (specifically, output file buffers) may permit the 
 * existence of "holes"--regions in the output sequence to which values have not been explicitly
 * written. When a seek operation moves the current position past the high_water_mark position, 
 * and a write occurs, the region of the output sequnce between the previous high_water_mark 
 * and the position of the seek/write operations potentially constitutes such a hole. The 
 * In general, the abstract behavior of any output sequence type should be to fill holes with
 * zero-valued bytes. However, a particular implementation may choose to either write zero-valued
 * bytes to the region, or mark the region as a hole, in which case any future attempt to
 * obtain values from the region of the output sequence will return zero-valued bytes.
 * 
 * To permit implementations freedom of choice in the matter, stream buffers support a touch
 * operation. When a seek operation sets the current position beyond the high_water_mark, the
 * state of the buffer (except for the current position) does not change until a write occurs
 * at the new position. Immediately prior to the actual write, the stream buffer invokes touch
 * (which, in turn, invokes the virtual member function really_touch), allowing the derived
 * implementation to create and/or fill the resulting hole. In essence, the touch operation 
 * announces to the derived implementation that a write operation at the current position
 * is imminent. touch forces a synchronization with the output sequence.
 * 
 * In order to prevent unnecessary invocations of touch, the stream buffer maintains a positional
 * value last_touched. Whenever a synchronization operation (touch or flush) occurs, last_touched 
 * is set to the current position after the synchronization. 
 * 
 * 
 */

class sink
{
public:
	friend class detail::sink_test_probe;

    // static constexpr buffer::size_type default_buffer_size = LOGICMILL_BSTREAM_DEFAULT_OBMEMBUF_SIZE;

    sink( byte_type* data, size_type size );

    sink( sink const& ) = delete;
    sink& operator=( sink&& ) = delete;
    sink& operator=( sink const& ) = delete;
    
    virtual ~sink() {}

protected:

    sink();

    sink( sink&& rhs );

public:

    void
    flush( std::error_code& err );

    void
    flush();

    void
    put( byte_type byte, std::error_code& err );

    void
    put( byte_type byte );

    void
    putn( const byte_type* src, size_type n, std::error_code& err );

    void
    putn( const byte_type* src, size_type n );

    void
    filln( const byte_type fill_byte, size_type n, std::error_code& err );

    void
    filln( const byte_type fill_byte, size_type n );

    position_type
    position( position_type pos, std::error_code& err )
    {
        return position( pos, bstream::seek_anchor::begin, err );
    }

    position_type
    position( position_type pos )
    {
        return position( pos, bstream::seek_anchor::begin );
    }

    position_type
    position( offset_type offset, seek_anchor where, std::error_code& err );

    position_type
    position( offset_type offset, seek_anchor where );

    position_type
    position() const;

	size_type
	size() const
	{
		return really_get_size();
	}

	size_type
	available() const
	{
		assert( m_end >= m_next );
		return static_cast< size_type >( m_end - m_next );
	}

protected:

    void
    set_ptrs( byte_type * base, byte_type * next, byte_type * end )
    {
        m_base = base;
        m_next = next;
        m_end = end;
    }

    // void
    // pbase_offset( position_type offset )
    // {
    //     m_base_offset = offset;
    // }

    // position_type
    // pbase_offset() const
    // {
    //     return m_base_offset;
    // }

    void
    overflow( size_type requested, std::error_code& err );

    void
    overflow( size_type requested );

    position_type
    get_high_watermark() const noexcept
    {
        return m_high_watermark;
    }

    position_type
    set_high_watermark() 
    {
        if ( m_dirty && ( ppos() > m_high_watermark ) )
        {
            m_high_watermark = ppos();
        }
        return m_high_watermark;
    }

    void
    force_high_watermark( position_type hwm )
    {
        m_high_watermark = hwm;
    }

    void
    reset_high_water_mark()
    {
        m_high_watermark = 0;
    }

    position_type
    ppos() const noexcept
    {
        return m_base_offset + ( m_next - m_base );
    }

	void
	really_fill( byte_type fill_byte, size_type n );

	position_type
	new_position( offset_type offset, seek_anchor where ) const;

protected: // to be overridden by derived classes

	virtual void
	really_jump( std::error_code& err );

	virtual bool
	is_valid_position( position_type pos ) const;

    virtual void
    really_overflow( size_type, std::error_code& err );
    
    virtual void
    really_flush( std::error_code& err );

	virtual size_type
	really_get_size() const
	{
		return ( m_dirty && ( ppos() > get_high_watermark() ) ) ? ppos() : get_high_watermark();
	}

protected:

    position_type       m_base_offset;
    position_type       m_high_watermark;
	position_type		m_jump_to;
    byte_type*          m_base;
    byte_type*          m_next;
    byte_type*          m_end;
    byte_type*          m_dirty_start;
    bool                m_dirty;
	bool				m_did_jump;
};

} // random
} // namespace bstream
} // namespace logicmill

#endif // LOGICMILL_BSTREAM_RANDOM_SINK_H