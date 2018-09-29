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

#ifndef LOGICMILL_TEST_BSTREAM_COMMON_H
#define LOGICMILL_TEST_BSTREAM_COMMON_H

#include <logicmill/bstream/source.h>
#include <logicmill/bstream/sink.h>
#include <logicmill/bstream/file/source.h>
#include <logicmill/bstream/file/sink.h>
#include <logicmill/bstream/memory/simple/source.h>
#include <logicmill/bstream/memory/simple/sink.h>

// #include <logicmill/bstream/sequential/sink.h>
// #include <logicmill/bstream/sequential/source.h>
// #include <logicmill/bstream/random/sink.h>
// #include <logicmill/bstream/random/source.h>
// #include <logicmill/bstream/file/sequential/sink.h>
// #include <logicmill/bstream/file/sequential/source.h>
// #include <logicmill/bstream/file/random/source.h>
// #include <logicmill/bstream/file/random/sink.h>
// #include <logicmill/bstream/memory/simple/sequential/sink.h>
// #include <logicmill/bstream/memory/simple/sequential/source.h>
// #include <logicmill/bstream/memory/simple/random/source.h>
// #include <logicmill/bstream/memory/simple/random/sink.h>


#define MATCH_MEMORY( _actual_, _expected_ )										\
	( ::memcmp( ( _actual_ ), ( _expected_ ), sizeof( _actual_ ) ) == 0 )			\
/**/

#define MATCH_MEMORY_N( _actual_, _expected_, _count_ )								\
	( ::memcmp( ( _actual_ ), ( _expected_ ), ( _count_ ) ) == 0 )					\
/**/

#define MATCH_BUFFER( _actual_, _expected_ )										\
	( ::memcmp( ( _actual_ ).data(), ( _expected_ ), ( _actual_ ).size() ) == 0	)	\
/**/

namespace logicmill { namespace bstream { 

namespace detail
{

class source_test_probe
{
public:
	source_test_probe( bstream::source& target )
	: 
	m_target{ target }
	{}

    position_type base_offset()
    {
        return m_target.m_base_offset;
    }

    position_type pos()
    {
        return m_target.gpos();
    }

    const byte_type* base()
    {
        return m_target.m_base;
    }

    const byte_type* next()
    {
        return m_target.m_next;
    }

    const byte_type* end()
    {
        return m_target.m_end;
    }

private:
	bstream::source& m_target;

};

class sink_test_probe
{
public:
	sink_test_probe( bstream::sink& target )
	: 
	m_target{ target }
	{}

    position_type base_offset()
    {
        return m_target.m_base_offset;
    }

    position_type hwm()
    {
        return m_target.m_high_watermark;
    }

    bool dirty()
    {
        return m_target.m_dirty;
    }

    byte_type* dirty_start()
    {
        return m_target.m_dirty_start;
    }

    size_type dirty_start_position()
    {
        return m_target.m_dirty_start - m_target.m_base;
    }

	bool did_jump()
	{
		return m_target.m_did_jump;
	}

	size_type jump_to()
	{
		return m_target.m_jump_to;
	}

    position_type pos()
    {
        return m_target.ppos();
    }

    byte_type* base()
    {
        return m_target.m_base;
    }

    byte_type* next()
    {
        return m_target.m_next;
    }

    byte_type* end()
    {
        return m_target.m_end;
    }

private:
	bstream::sink& m_target;

};

}
	
// namespace sequential { namespace detail {

// class source_test_probe
// {
// public:
// 	source_test_probe( bstream::sequential::source& target )
// 	: 
// 	m_target{ target }
// 	{}

//     position_type base_offset()
//     {
//         return m_target.m_base_offset;
//     }

//     position_type pos()
//     {
//         return m_target.gpos();
//     }

//     const byte_type* base()
//     {
//         return m_target.m_base;
//     }

//     const byte_type* next()
//     {
//         return m_target.m_next;
//     }

//     const byte_type* end()
//     {
//         return m_target.m_end;
//     }

// private:
// 	bstream::sequential::source& m_target;

// };

// class sink_test_probe
// {
// public:
// 	sink_test_probe( bstream::sequential::sink& target )
// 	: 
// 	m_target{ target }
// 	{}

//     position_type base_offset()
//     {
//         return m_target.m_base_offset;
//     }

//     position_type pos()
//     {
//         return m_target.ppos();
//     }

//     byte_type* base()
//     {
//         return m_target.m_base;
//     }

//     byte_type* next()
//     {
//         return m_target.m_next;
//     }

//     byte_type* end()
//     {
//         return m_target.m_end;
//     }

// private:
// 	bstream::sequential::sink& m_target;

// };

// } /* namespace detail */ } /* namespace sequential */

// namespace random { namespace detail {

// class source_test_probe : public bstream::sequential::detail::source_test_probe
// {
// public:
// 	source_test_probe( bstream::random::source& target )
// 	:
// 	bstream::sequential::detail::source_test_probe{ target },
// 	m_target{ target }
// 	{}

// private:
// 	bstream::random::source& m_target;

// };

// class sink_test_probe
// {
// public:
// 	sink_test_probe( bstream::random::sink& target )
// 	: 
// 	m_target{ target }
// 	{}

//     position_type base_offset()
//     {
//         return m_target.m_base_offset;
//     }

//     position_type hwm()
//     {
//         return m_target.m_high_watermark;
//     }

//     bool dirty()
//     {
//         return m_target.m_dirty;
//     }

//     byte_type* dirty_start()
//     {
//         return m_target.m_dirty_start;
//     }

//     size_type dirty_start_position()
//     {
//         return m_target.m_dirty_start - m_target.m_base;
//     }

// 	bool did_jump()
// 	{
// 		return m_target.m_did_jump;
// 	}

// 	size_type jump_to()
// 	{
// 		return m_target.m_jump_to;
// 	}

//     position_type pos()
//     {
//         return m_target.ppos();
//     }

//     byte_type* base()
//     {
//         return m_target.m_base;
//     }

//     byte_type* next()
//     {
//         return m_target.m_next;
//     }

//     byte_type* end()
//     {
//         return m_target.m_end;
//     }

// private:
// 	bstream::random::sink& m_target;

// };

// } /* namespace detail */ } /* namespace random */

namespace file {

namespace detail
{

class source_test_probe : public bstream::detail::source_test_probe
{
public:

	source_test_probe( bstream::file::source& target )
	:
	bstream::detail::source_test_probe{ target },
	m_target{ target }
	{}

	mutable_buffer& buffer()
	{
		return m_target.m_buf;
	}

	bool is_open()
	{
		return m_target.m_is_open;
	}

	int flags()
	{
		return m_target.m_flags;
	}

private:

	bstream::file::source& m_target;
};

class sink_test_probe : public bstream::detail::sink_test_probe
{
public:

	sink_test_probe( bstream::file::sink& target )
	:
	bstream::detail::sink_test_probe{ target },
	m_target{ target }
	{}

	mutable_buffer& buffer()
	{
		return m_target.m_buf;
	}

	bool is_open()
	{
		return m_target.m_is_open;
	}

	open_mode mode()
	{
		return m_target.m_mode;
	}

	int flags()
	{
		return m_target.m_flags;
	}

private:
	bstream::file::sink& m_target;
};

}

// namespace sequential { namespace detail {

// class sink_test_probe : public bstream::sequential::detail::sink_test_probe
// {
// public:

// 	sink_test_probe( bstream::file::sequential::sink& target )
// 	:
// 	bstream::sequential::detail::sink_test_probe{ target },
// 	m_target{ target }
// 	{}

// 	mutable_buffer& buffer()
// 	{
// 		return m_target.m_buf;
// 	}

// 	bool is_open()
// 	{
// 		return m_target.m_is_open;
// 	}

// 	open_mode mode()
// 	{
// 		return m_target.m_mode;
// 	}

// 	int flags()
// 	{
// 		return m_target.m_flags;
// 	}

// private:
// 	bstream::file::sequential::sink& m_target;
// };

// class source_test_probe : public bstream::sequential::detail::source_test_probe
// {
// public:

// 	source_test_probe( bstream::file::sequential::source& target )
// 	:
// 	bstream::sequential::detail::source_test_probe{ target },
// 	m_target{ target }
// 	{}

// 	mutable_buffer& buffer()
// 	{
// 		return m_target.m_buf;
// 	}

// 	bool is_open()
// 	{
// 		return m_target.m_is_open;
// 	}

// 	int flags()
// 	{
// 		return m_target.m_flags;
// 	}

// private:
// 	bstream::file::sequential::source& m_target;
// };

// } /* namespace detail */ } /* namespace sequential */

// namespace random { namespace detail {

// class source_test_probe : public file::sequential::detail::source_test_probe
// {
// public:

// 	source_test_probe( bstream::file::random::source& target )
// 	:
// 	file::sequential::detail::source_test_probe{ target },
// 	m_target{ target }
// 	{}

// 	mutable_buffer& buffer()
// 	{
// 		return m_target.m_buf;
// 	}

// 	bool is_open()
// 	{
// 		return m_target.m_is_open;
// 	}

// 	int flags()
// 	{
// 		return m_target.m_flags;
// 	}

// private:

// 	bstream::file::random::source& m_target;
// };

// class sink_test_probe : public bstream::random::detail::sink_test_probe
// {
// public:
// 	sink_test_probe( bstream::file::random::sink& target )
// 	:
// 	bstream::random::detail::sink_test_probe{ target },
// 	m_target{ target }
// 	{}

// 	mutable_buffer& buffer()
// 	{
// 		return m_target.m_buf;
// 	}

// 	bool is_open()
// 	{
// 		return m_target.m_is_open;
// 	}

// 	open_mode mode()
// 	{
// 		return m_target.m_mode;
// 	}

// 	int flags()
// 	{
// 		return m_target.m_flags;
// 	}

// private:
// 	bstream::file::random::sink& m_target;
// };

// } /* namespace detail */ } /* namespace random */ 

} /* namespace file */

namespace memory { namespace simple { 

namespace detail
{

template< class Buffer >
class source_test_probe : public bstream::detail::source_test_probe
{
public:
	source_test_probe( bstream::memory::simple::source< Buffer >& target )
	:
	bstream::detail::source_test_probe{ target },
	m_target{ target }
	{}

	Buffer& buffer()
	{
		return m_target.m_buf;
	}

private:
	bstream::memory::simple::source< Buffer >& m_target;

};

class sink_test_probe : public bstream::detail::sink_test_probe
{
public:
	sink_test_probe( memory::simple::sink& target )
	:
	bstream::detail::sink_test_probe{ target },
	m_target{ target }
	{}

	mutable_buffer& buffer()
	{
		return m_target.m_buf;
	}

private:
	memory::simple::sink& m_target;
};

}
	
// namespace sequential { namespace detail {

// template< class Buffer >
// class source_test_probe : public bstream::sequential::detail::source_test_probe
// {
// public:
// 	source_test_probe( bstream::memory::simple::sequential::source< Buffer >& target )
// 	:
// 	bstream::sequential::detail::source_test_probe{ target },
// 	m_target{ target }
// 	{}

// 	Buffer& buffer()
// 	{
// 		return m_target.m_buf;
// 	}

// private:
// 	bstream::memory::simple::sequential::source< Buffer >& m_target;

// };

// class sink_test_probe : public bstream::sequential::detail::sink_test_probe
// {
// public:
// 	sink_test_probe( bstream::memory::simple::sequential::sink& target )
// 	:
// 	bstream::sequential::detail::sink_test_probe{ target },
// 	m_target{ target }
// 	{}

// 	mutable_buffer& buffer()
// 	{
// 		return m_target.m_buf;
// 	}

// private:
// 	bstream::memory::simple::sequential::sink& m_target;

// };

// } /* namespace detail */ } /* namespace sequential */

// namespace random { namespace detail {

// template< class Buffer >
// class source_test_probe : public memory::simple::sequential::detail::source_test_probe< Buffer >
// {
// public:

// 	source_test_probe( bstream::memory::simple::random::source< Buffer >& target )
// 	:
// 	memory::simple::sequential::detail::source_test_probe< Buffer >{ target },
// 	m_target{ target }
// 	{}

// 	Buffer& buffer()
// 	{
// 		return m_target.m_buf;
// 	}

// protected:

// 	memory::simple::random::source< Buffer >& m_target;
// };

// class sink_test_probe : public bstream::random::detail::sink_test_probe
// {
// public:
// 	sink_test_probe( memory::simple::random::sink& target )
// 	:
// 	bstream::random::detail::sink_test_probe{ target },
// 	m_target{ target }
// 	{}

// 	mutable_buffer& buffer()
// 	{
// 		return m_target.m_buf;
// 	}

// private:
// 	memory::simple::random::sink& m_target;
// };

// } /* namespace detail */ } /* namespace random */ 

} /* namespace simple */ } /* namespace memory */

} /* namespace bstream */ } /* namespace logicmill */

#endif // LOGICMILL_TEST_BSTREAM_COMMON_H