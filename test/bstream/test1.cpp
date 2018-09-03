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

#include "../doctest.h"
#include <logicmill/bstream.h>
#include <logicmill/bstream/ombstream.h>
#include <logicmill/bstream/imbstream.h>
#include <thread>
#include <chrono>


using namespace logicmill;

namespace test_types_1
{

class far : BSTRM_BASE( far )
{
public:
	far() {}
	far( const std::string& name, int num ) : name_{ name }, number_{ num } {}
	virtual ~far() {}

	BSTRM_CLASS( far, , ( name_, number_ ) )
	
    const std::string& name() const
    {
        return name_;
    }

   	int number() const
	{
		return number_;
	}
	
private:
	std::string name_;
    int number_;
};


class foo : BSTRM_BASE( foo )
{
public:
	BSTRM_FRIEND_BASE( foo )
	BSTRM_CTOR( foo, , ( name_, number_ ) )
	BSTRM_ITEM_COUNT( , ( name_, number_ ) )
	BSTRM_SERIALIZE( foo, , ( name_, number_ ) )
	
	foo( std::string const& name, int number ) : name_{ name }, number_{ number } {}
	virtual ~foo() {}

	bstream::ibstream& deserialize( bstream::ibstream& is )
	{
		is.check_array_header( 2 );
		name_ = is.read_as< std::string >();
		number_ = is.read_as< int >();
		return is;
	}

	std::string const& name() const
	{
		return name_;
	}
	
	int number() const
	{
		return number_;
	}
	
private:
	std::string name_;
	int number_;
};

class foz : BSTRM_BASE( foz )
{
public:
	BSTRM_FRIEND_BASE( foz )
	BSTRM_CTOR( foz, , ( name_, number_ ) )
	BSTRM_ITEM_COUNT( , ( name_, number_ ) )
	BSTRM_SERIALIZE_DECL()
	
	foz( std::string const& name, int number ) : name_{ name }, number_{ number } {}
	
	virtual ~foz() {}
	
	bstream::ibstream& deserialize( bstream::ibstream& is )
	{
		is.check_array_header( 2 );
		name_ = is.read_as< std::string >();
		number_ = is.read_as< int >();
		return is;
	}

	std::string const& name() const
	{
		return name_;
	}
	
	int number() const
	{
		return number_;
	}
	
private:
	std::string name_;
	int number_;
};

} // namespace test_types_1

using namespace test_types_1;

BSTRM_SERIALIZE_DEF( , foz, , ( name_, number_ ) )

TEST_CASE( "logicmill/smoke/bstream/array_base_macro" )
{
	bstream::ombstream os{ 1024 };
	foo f0( "france is bacon", 27 );
	os << f0;
	
	bstream::imbstream is{ os.get_buffer() };
	
	foo f1 = is.read_as< foo >();
	CHECK( f1.name() == f0.name() );
	CHECK( f1.number() == f0.number() );
}

TEST_CASE( "logicmill/smoke/bstream/map_base_macro" )
{
	CHECK( ! bstream::has_deserialize_method< far >::value );

	bstream::ombstream os{ 1024 };
	far f0( "france is bacon", 27 );
	os << f0;
	
	bstream::imbstream is{ os.get_buffer() };
	
	far f1 = is.read_as< far >();
	CHECK( f1.name() == f0.name() );
	CHECK( f1.number() == f0.number() );

}

TEST_CASE( "logicmill/smoke/bstream/external_serialize_def" )
{
//	std::this_thread::sleep_for ( std::chrono::seconds( 10 ) );
	bstream::ombstream os{ 1024 };
	foz f0( "france is bacon", 27 );
	os << f0;
	
	bstream::imbstream is{ os.get_buffer() };
	
	foz f1 = is.read_as< foz >();
	CHECK( f1.name() == f0.name() );
	CHECK( f1.number() == f0.number() );
}

