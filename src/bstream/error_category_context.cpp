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

#include <logicmill/bstream/error_category_context.h>
#include <logicmill/bstream/error.h>

#include <iostream>

using namespace logicmill;
using namespace bstream;


error_category_context::category_init_list const&
error_category_context::default_categories()
{
	static const category_init_list catlist
			= {&std::system_category(), &std::generic_category(), &bstream::error_category()};
	return catlist;
};


error_category_context::error_category_context( category_init_list init_list )
{
	auto& def_categories = default_categories();
	m_category_vector.reserve( init_list.size() + def_categories.size() );
	m_category_vector.insert(m_category_vector.end(), def_categories.begin(), def_categories.end() );
	m_category_vector.insert(m_category_vector.end(), init_list.begin(), init_list.end() );

	for ( auto i = 0u; i < m_category_vector.size(); ++i )
	{
		m_category_map.emplace( m_category_vector[ i ], i );
	}
}

error_category_context::error_category_context()
{
	auto& def_categories = default_categories();
	m_category_vector.reserve( def_categories.size() );
	m_category_vector.insert(m_category_vector.end(), def_categories.begin(), def_categories.end() );

	for ( auto i = 0u; i < m_category_vector.size(); ++i )
	{
		m_category_map.emplace( m_category_vector[ i ], i );
	}
}

std::error_category const&
error_category_context::category_from_index( index_type index ) const
{
	if ( index < 0  || static_cast< std::size_t >( index ) >= m_category_vector.size() )
	{
		throw std::system_error( make_error_code( bstream::errc::invalid_err_category ) );
	}
	else
	{
		return *( m_category_vector[ index ] );
	}
}

error_category_context::index_type
error_category_context::index_of_category( std::error_category const& category ) const
{
	auto it = m_category_map.find( &category );
	if ( it != m_category_map.end() )
	{
		return it->second;
	}
	else
	{
		throw std::system_error( make_error_code( bstream::errc::invalid_err_category ) );
	}
}
