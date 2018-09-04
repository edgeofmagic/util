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

#include <logicmill/bstream/ifbstream.h>
#include <logicmill/bstream/ofbstream.h>
#include <doctest.h>

using namespace logicmill;
using namespace bstream;
using bstream::ofbstream;

TEST_CASE( "logicmill/smoke/bstream/fbstream/write_read" )
{
    bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::truncate );
    mutable_buffer outbuf{ "abcdefghijklmnop" };
    os.putn( outbuf );
    os.close();

    bstream::ifbstream is( "fbstream_test_file" );
    auto fsize = is.tell( seek_anchor::end );
    const_buffer inbuf = is.getn( fsize );
    is.close();
    CHECK( outbuf == inbuf );
}

TEST_CASE( "logicmill/smoke/bstream/fbstream/write_read_ate" )
{
    {
        bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::truncate );
        mutable_buffer outbuf{ "abcdefghijklmnop" };
        os.putn( outbuf );
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.tell( seek_anchor::end );
        const_buffer inbuf = is.getn( fsize );
        is.close();
        mutable_buffer expected{ "abcdefghijklmnop" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::at_end );
        auto pos = os.tell( seek_anchor::current );
        CHECK( pos == 16 );
        auto fsize = os.tell( seek_anchor::end );
        CHECK( fsize == 16 );
        mutable_buffer outbuf{ "qrstuvwxyz" };
        os.putn( outbuf );
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.tell( seek_anchor::end );
        CHECK( fsize == 26 );
        const_buffer inbuf = is.getn( fsize );
        is.close();
        mutable_buffer expected{ "abcdefghijklmnopqrstuvwxyz" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.tell( seek_anchor::end );
        CHECK( fsize == 26 );
        is.seek( seek_anchor::begin, 16 );
        const_buffer inbuf = is.getn( 10 );
        bool caught_exception = false;
        try
        {
            is.get();
            CHECK( false ); // should never get here
        }
        catch ( std::system_error const& e )
        {
            caught_exception = true;
        }
        CHECK( caught_exception );
        is.close();
        mutable_buffer expected{ "qrstuvwxyz" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::append );
        auto pos = os.tell( seek_anchor::current );
        CHECK( pos == 26 );
        mutable_buffer outbuf{ "0123456789" };
        os.putn( outbuf );
        auto zpos = os.seek( seek_anchor::begin, 0 );
        os.putn( outbuf );
        zpos = os.tell( seek_anchor::current );
        os.close();
    }
    {
        bstream::ofbstream os( "fbstream_test_file" );
        auto pos = os.tell( bstream::seek_anchor::current );
        CHECK( pos == 0 );
        auto fsize = os.tell( bstream::seek_anchor::end );
        CHECK( fsize == 46 );
        mutable_buffer outbuf{ "0123456789" };
        os.putn( outbuf );
        auto zpos = os.tell( bstream::seek_anchor::current );
        CHECK( zpos == 10 );
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.tell( seek_anchor::end );
        CHECK( fsize == 46 );
        const_buffer inbuf = is.getn( fsize );
        is.close();
        mutable_buffer expected{ "0123456789klmnopqrstuvwxyz01234567890123456789" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::truncate );
        mutable_buffer outbuf{ "abcdefghijklmnop" };
        os.putn( outbuf );
        os.seek( seek_anchor::begin, 36 );
        mutable_buffer outbuf2{ "0123456789" };
        os.putn( outbuf2 );
        auto zpos = os.tell( seek_anchor::current );
        CHECK( zpos == 46 );
        os.close();
    }
    
}
