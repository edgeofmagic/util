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

#ifndef LOGICMILL_ASYNC_EVENT_H
#define LOGICMILL_ASYNC_EVENT_H

#include <logicmill/async/error.h>
#include <logicmill/traits.h>
#include <functional>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>

#if 1
#define LOGICMILL_ASYNC_USE_SENDER_BASE(_source_base_)                                                                 \
	using _source_base_::fit;                                                                                          \
	using _source_base_::get_fitting;                                                                                  \
	using _source_base_::send;                                                                                         \
	using _source_base_::is_paired;                                                                                    \
/**/

#define LOGICMILL_ASYNC_USE_RECEIVER_BASE(_sink_base_)                                                                 \
	using _sink_base_::get_fitting;                                                                                    \
/**/
# endif 

namespace logicmill {
namespace async {


template < class T, T Value, class... Args >
class event
{
public:
	using sink_f = std::function< void ( Args... ) >;
	using source_f = std::function< void ( sink_f ) >;
};

template< class T >
struct is_event_type : std::false_type {};

template< class T, T Value, class... Args >
struct is_event_type< event< T, Value, Args... > > : std::true_type {};

template< class T, class Enable = void >
class sink;

template< class T, class Enable = void >
class source;

template< class T >
class sink< T, std::enable_if_t< is_event_type< T >::value > >
{
public:
	using event_type = T;
	friend class source< event_type >;
	using type = typename T::sink_f;

	sink() : m_sink_f{ nullptr } {}

	template< class S, class = std::enable_if_t< std::is_convertible< S, type >::value > >
	sink( S&& snkf ) : m_sink_f{ std::forward< S >( snkf ) } {}

	sink( sink const& rhs ) : m_sink_f{ rhs.m_sink_f } {}
	sink( sink&& rhs ) : m_sink_f{ std::move( rhs.m_sink_f ) } {}

private:
	type		m_sink_f;
};

template< class T >
struct is_sink : std::false_type {};

template< class E >
struct is_sink< sink< E > > : std::true_type {};

template< class T >
class source< T, std::enable_if_t< is_event_type< T >::value > >
{
public:
	using event_type = T;
	using type = typename T::source_f;
	source() : m_src_f{ nullptr } {}
	template< class S, class = std::enable_if_t< std::is_convertible< S, typename event_type::source_f >::value > >
	source( S&& srcf ) : m_src_f{ std::forward< S >( srcf ) } {}

	source( source const& rhs ) : m_src_f{ rhs.m_src_f } {}
	source( source&& rhs ) : m_src_f{ std::move( rhs.m_src_f ) } {}

	source& operator=( source const& rhs )
	{
		m_src_f = rhs.m_src_f;
		return *this;
	}

	source& operator=( source&& rhs )
	{
		m_src_f = std::move( rhs.m_src_f );
		return *this;
	}

	void
	fit( sink< event_type >&& snk )
	{
		m_src_f( std::move( snk.m_sink_f ) );
	}

	void
	fit( sink< event_type > const& snk )
	{
		m_src_f( snk.m_sink_f );
	}

private:
	typename event_type::source_f		m_src_f;
};

template< class T >
struct is_source : std::false_type {};

template< class E >
struct is_source< source< E > > : std::true_type {};

using test_event = event< int, 0, int >;
using test_sink = sink< test_event >;

static_assert( is_sink< test_sink >::value );
static_assert( is_event_type< test_event >::value );
static_assert( std::is_same< test_event, sink< test_event >::event_type >::value );
static_assert( std::is_same< test_event::sink_f, sink< test_event >::type >::value );

template< class E  >
class source_base;

template< class T, T Value, class... Args >
class source_base< event< T, Value, Args... > > 
{
public:
	using event_type = event< T, Value, Args... >;
	using source_type = source< event_type >;
	using sink_type = sink< event_type >;

	template< class Evt >
	typename std::enable_if_t< std::is_same< Evt, event_type >::value, source< Evt > >
	get_source()
	{
		return source< Evt >{ [=] ( auto&& snk )
		{
			this->m_sink = std::forward< decltype( snk ) >( snk );
		} };
	}

	template< class Evt, class... Params >
	typename std::enable_if_t< std::is_same< Evt, event_type >::value >
	send( Params&&... params )
	{
		if ( m_sink )
		{
			m_sink( std::forward< Params >( params )... );
		}
	}

private:
	typename event_type::sink_f		m_sink = nullptr;
};

template< class E, class Derived >
class sink_base;

template< class Derived, class T, T Value, class... Args >
class sink_base< event< T, Value, Args... >, Derived >
{
public:
	using event_type = event< T, Value, Args... >;

	template< class Evt >
	typename std::enable_if_t< std::is_same< Evt, event_type >::value, sink< Evt > >
	get_sink()
	{
		return sink< event_type >{ [=] ( auto&&... params ) mutable
		{
			static_cast< Derived* >( this )->on( event_type{}, std::forward< decltype( params ) >( params )... );
		} };
	}
};

template< class T >
struct is_fitting : std::integral_constant< bool, is_source< T >::value || is_sink< T >::value > {};

template< class... Args >
struct args_are_fittings : std::false_type {};

template< class First, class... Rest >
struct args_are_fittings< First, Rest... > :
std::integral_constant< bool, is_fitting< First >::value && args_are_fittings< Rest... >::value > {};

template< class T >
struct args_are_fittings< T > : is_fitting< T > {};

static_assert( is_fitting< source< test_event > >::value );
static_assert( args_are_fittings< source< test_event >, test_sink >::value );

template< class T, class Enable = void >
struct fits_with;

template< class T >
struct fits_with< T, std::enable_if_t< is_source< T >::value > >
{
	using type = sink< typename T::event_type >;
};

template< class T >
struct fits_with< T, std::enable_if_t< is_sink< T >::value > >
{
	using type = source< typename T::event_type >;
};

template< class Source, class Sink >
inline typename std::enable_if_t< is_source< Source >::value && std::is_same< Sink, sink< typename Source::event_type > >::value >
fit( Source& src, Sink& snk )
{
	src.fit( snk );
}

template< class Sink, class Source >
inline typename std::enable_if_t< is_source< Source >::value && std::is_same< Sink, sink< typename Source::event_type > >::value >
fit( Sink& snk, Source& src )
{
	src.fit( snk );
}

template< std::size_t I = 0, class... Tp>
inline typename std::enable_if_t< I == sizeof...( Tp ) >
fit_each( std::tuple< Tp... >& a, std::tuple< typename fits_with< Tp >::type... >& b )
{
}

template< std::size_t I = 0, class... Tp>
inline typename std::enable_if_t< I < sizeof...( Tp ) >
fit_each( std::tuple< Tp... >& a, std::tuple< typename fits_with< Tp >::type... >& b )
{
	fit( std::get< I >( a ), std::get< I >( b ) );
	fit_each< I + 1, Tp... >( a, b );
}

template< class... Args >
class connector
{
public:
	friend class connector< typename fits_with< Args >::type... >;
	static_assert( args_are_fittings< Args... >::value );
	connector( Args... args )
	:
	m_tuple{ args... }
	{}

	void mate( connector< typename fits_with< Args >::type... >& that )
	{
		fit_each( m_tuple, that.m_tuple );
	}

private:

	std::tuple< Args... >		m_tuple;
};

template< class T >
struct complement;

template< class... Args >
struct complement< connector< Args... > >
{
	using type = connector< typename fits_with< Args >::type... >;
};

using test_event_0 = event< int, 1, bool >;
using test_event_1 = event< int, 2, std::string const& >;

using tcon0 = connector< source< test_event_0 >, sink< test_event_1 > >;

using tcon1 = complement< tcon0 >::type;

static_assert( std::is_same< fits_with< source< test_event > >::type, sink< test_event > >::value );


template< class T, class Derived, class Enable = void >
struct fitting_base;

template< class T, class Derived >
struct fitting_base< T, Derived, std::enable_if_t< is_source< T >::value > >
{
	using type = source_base< typename T::event_type >;
};

template< class T, class Derived >
struct fitting_base< T, Derived, std::enable_if_t< is_sink< T >::value > >
{
	using type = sink_base< typename T::event_type, Derived >;
};

template< class T, class Derived, class Enable = void >
struct fitting_initializer;

template< class T, class Derived >
struct fitting_initializer< T, Derived, std::enable_if_t< is_source< T >::value > >
{
	source< typename T::event_type >
	operator()( source_base< typename T::event_type >& s ) const
	{
		return s. template get_source< typename T::event_type >();
	}
};

template< class T, class Derived >
struct fitting_initializer< T, Derived, std::enable_if_t< is_sink< T >::value > >
{
	sink< typename T::event_type >
	operator()( sink_base< typename T::event_type, Derived >& s ) const
	{
		return s. template get_sink< typename T::event_type >();
	}
};

template< class Connector, class Derived >
class connectable;

template< class Derived, class... Args >
class connectable< connector< Args... >, Derived >
:
public fitting_base< Args, Derived >::type...
{
public:
	using connector_type = connector< Args... >;

	connectable()
	:
	m_connector{ fitting_initializer< Args, Derived >{}( *this )... }
	{}

	template< class Connector >
	typename std::enable_if_t< std::is_same< Connector, connector_type >::value, connector_type& >
	get_connector()
	{
		return m_connector;
	}

	template< class T >
	void
	mate( T& other )
	{
		m_connector.mate( other. template get_connector< typename complement< connector_type >::type >() );
	}

private:
	connector_type	m_connector;
};

} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_EVENT_H
