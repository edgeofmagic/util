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

#include <chrono>
#include <thread>
#include <logicmill/bstream/ibstreambuf.h>
#include <logicmill/bstream/obstreambuf.h>
#include <logicmill/bstream/ibmembuf.h>
#include <logicmill/bstream/obmembuf.h>
#include <logicmill/bstream/ibfilebuf.h>
#include <logicmill/bstream/obfilebuf.h>
#include <doctest.h>
#include <logicmill/bstream/error.h>

using namespace logicmill;
using namespace bstream;

#ifndef DOCTEST_CONFIG_DISABLE

class bstream::detail::obs_test_probe
{
public:
	obs_test_probe( bstream::obstreambuf& target )
	: 
	m_target{ target }
	{}

    position_type base_offset()
    {
        return m_target.m_pbase_offset;
    }

    position_type hwm()
    {
        return m_target.m_high_watermark;
    }

    bool dirty()
    {
        return m_target.m_dirty;
    }

	bool did_jump()
	{
		return m_target.m_did_jump;
	}

	size_type jump_to()
	{
		return m_target.m_jump_to;
	}

    // bool mutability_implied()
    // {
    //     return m_target.m_implies_mutability;
    // }

    void* base()
    {
        return m_target.m_pbase;
    }

    void* next()
    {
        return m_target.m_pnext;
    }

    void* end()
    {
        return m_target.m_pend;
    }

private:
	bstream::obstreambuf& m_target;

};

class bstream::detail::ibs_test_probe
{
public:
	ibs_test_probe( bstream::ibstreambuf& target )
	: 
	m_target{ target }
	{}

    position_type base_offset()
    {
        return m_target.m_gbase_offset;
    }    

    const void* base()
    {
        return m_target.m_gbase;
    }

    const void* next()
    {
        return m_target.m_gnext;
    }

    const void* end()
    {
        return m_target.m_gend;
    }

private:
	bstream::ibstreambuf& m_target;

};


#endif

TEST_CASE( "logicmill/smoke/obstreambuf/basic" )
{
    // std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

    // buffer tbuf{ 16, buffer::policy::no_copy_on_write };

    bstream::obmembuf obuf{ 16 };

    bstream::detail::obs_test_probe probe{ obuf };

    std::error_code err;

	std::string zooble{ "zooble" };
	std::string gorn{ "gorn" };
	std::string black{ "black" };

    mutable_buffer tbuf0{ zooble.size() };
	tbuf0.putn( 0, zooble.data(), zooble.size() );
	tbuf0.size( zooble.size() );
    mutable_buffer tbuf1{ gorn.size() };
	tbuf1.putn( 0, gorn.data(), gorn.size() );
	tbuf1.size( gorn.size() );
    mutable_buffer tbuf2{ black.size() };
	tbuf2.putn( 0, black.data(), black.size() );
	tbuf2.size( black.size() );

    obuf.putn( tbuf0.data(), tbuf0.size(), err );
    CHECK( ! err );
    CHECK( obuf.position() == 6 );
    CHECK( ! err );

    {
        auto& bref = obuf.get_buffer_ref();
        auto end_pos = probe.hwm();
        CHECK( end_pos == bref.size() );
//        view.dump( std::cout );
    }

    {
        obuf.position( 8, err );
        CHECK( ! err );
        obuf.putn( tbuf1.data(), tbuf1.size(), err );
        CHECK( !err );
        CHECK( obuf.position() == 12 );
        CHECK( ! err );
    }
    {
        auto& bref = obuf.get_buffer_ref();
        auto end_pos = probe.hwm();
        CHECK( end_pos == bref.size() );
//        view.dump( std::cout );
    }

    {
        obuf.putn( tbuf2.data(), tbuf2.size(), err );
        CHECK( ! err );
    }

    {
        auto& bref = obuf.get_buffer_ref();
        auto end_pos = probe.hwm();
        auto tell_end = obuf.size();
        CHECK( ! err );
        CHECK( tell_end == end_pos );
        CHECK( end_pos == bref.size() );
//        view.dump( std::cout );
    }

}

TEST_CASE( "logicmill/smoke/ibstreambuf/basic" )
{
    // std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

	std::string contents{ "0123456789ABCDEF" };
    mutable_buffer buf{ contents.size() };
	buf.putn( 0, contents.data(), contents.size() );
	buf.size( contents.size() );
    bstream::ibstreambuf ibuf{ buf.data(), buf.size() };

    bstream::detail::ibs_test_probe probe{ ibuf };
    std::error_code err;

    int index = 0;
    while ( true )
    {
        auto b = ibuf.get( err );
        if ( index < 16 )
        {
            CHECK( b == *( buf.data() + index) );
            CHECK( ! err );
        }
        else
        {
            CHECK( err );
//            std::cout << err.message() << std::endl;
            break;
        }
        ++index;
    }

    CHECK( ibuf.position( 3, bstream::seek_anchor::begin, err ) == 3 );
    CHECK( ! err );
    auto b = ibuf.get( err );
    CHECK( ! err );
    CHECK( b == *( buf.data() + 3 ) );
    CHECK( ibuf.position( 5, bstream::seek_anchor::current, err  ) == 9 );
    b = ibuf.get( err );
    CHECK( ! err );
    CHECK( b == *( buf.data() + 9 ) );

    CHECK( ibuf.position( -1, bstream::seek_anchor::end, err ) == 15 );
    b = ibuf.get( err );
    CHECK( ! err );
    CHECK( b == *( buf.data() + 15 ) );

    b = ibuf.get( err );
    CHECK( err );
    CHECK( err == bstream::errc::read_past_end_of_stream );

    CHECK( ibuf.position( 0, bstream::seek_anchor::begin, err ) == 0 );
    CHECK( ! err );

    auto bf = ibuf.getn( as_const_buffer{}, 7, err );
    CHECK( !  err );

    CHECK( bf.size() == 7 );
    CHECK( ibuf.position() == 7 );
    CHECK( bf.to_string() == std::string{ "0123456" } ); 

    CHECK( ibuf.position( -8, bstream::seek_anchor::current, err ) == bstream::npos );
    CHECK( err );
    CHECK( err == std::errc::invalid_seek );

    CHECK( ibuf.position( -4, bstream::seek_anchor::end, err ) == 12 );
    CHECK( ! err );
    
    bf = ibuf.getn( as_const_buffer{}, 5, err );
    CHECK( bf.size() == 4 );
    CHECK( bf.to_string() == "CDEF" );
    CHECK( ! err );
}

TEST_CASE( "logicmill/smoke/obfilebuf/basic" )
{
    // std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

    mutable_buffer buf( 256 );
    for ( auto i = 0u; i < buf.size(); ++i )
    {
        buf.put( i, static_cast< bstream::byte_type >( i ) );
    }
    std::error_code err;
    bstream::obfilebuf obf{ "filebuftest", bstream::open_mode::truncate, err, 32 };
    CHECK( ! err );

    bstream::detail::obs_test_probe probe{ obf };
    CHECK( probe.base_offset() == 0 );
    CHECK( probe.next() == probe.base() );
    CHECK( probe.hwm() == 0 );
    CHECK( ! probe.dirty() );
    CHECK( probe.end() == reinterpret_cast< bstream::byte_type* >( probe.base() ) + 32 );
    
    obf.putn( buf.data(), 32, err );
    CHECK( ! err );
    CHECK( probe.base_offset() == 0 );
    CHECK( probe.next() == reinterpret_cast< bstream::byte_type* >( probe.base() ) + 32 );
    CHECK( probe.hwm() == 0 );
    CHECK( probe.dirty() );

    obf.flush( err );
    CHECK( ! err );
    CHECK( probe.base_offset() == 32 );
    CHECK( probe.next() == probe.base() );
    CHECK( probe.hwm() == 32 );
    CHECK( ! probe.dirty() );

    obf.close( err );
    CHECK( ! err );

    obf.open( "filebuftest", bstream::open_mode::at_end, err );
    CHECK( ! err );
    CHECK( probe.base_offset() == 32 );
    CHECK( probe.next() == probe.base() );
    CHECK( probe.hwm() == 32 );
    CHECK( ! probe.dirty() );

    auto pos = obf.position();
    CHECK( ! err );
    CHECK( pos == 32 );

    pos = obf.position( 32, bstream::seek_anchor::current, err );
    CHECK( ! err );
    CHECK( pos == 64 );
    CHECK( probe.base_offset() == 32 );
    CHECK( probe.next() == probe.base() );
    CHECK( probe.hwm() == 32 );
	CHECK( probe.did_jump() == true );
	CHECK( probe.jump_to() == 64 );
    CHECK( ! probe.dirty() );

    obf.putn( buf.data() + 32, 48, err );
    CHECK( ! err );
    CHECK( probe.base_offset() == 96 );
    CHECK( probe.next() == reinterpret_cast< bstream::byte_type* >( probe.base() ) + 16 );
    CHECK( probe.hwm() == 96 );
    CHECK( probe.dirty() );

    obf.close( err );
    CHECK( ! err );
}

TEST_CASE( "logicmill/smoke/ibfilebuf/basic" )
{
    // std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

    std::error_code err;
    bstream::ibfilebuf ibf{ "filebuftest", err, 0, 32 };
    CHECK( ! err );

    auto end_pos = ibf.size();
    CHECK( ! err );
    CHECK( end_pos == 112 );

    auto pos = ibf.position();
    CHECK( pos == 0 );

    const_buffer buf = ibf.getn( as_const_buffer{}, end_pos, err );
    CHECK( ! err );
    CHECK( buf.size() == 112 );

    ibf.close( err );
    CHECK( ! err );
//    buf.dump( std::cout );
}