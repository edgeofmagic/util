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

#if 0 
#define LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( _event_name_ )			\
	using _event_name_::emitter::add_listener;							\
	using _event_name_::emitter::remove_listener;						\
	using _event_name_::emitter::emit;									\
	using _event_name_::emitter::listener_count;						\
	using _event_name_::remove_all;										\
/**/

#define LOGICMILL_ASYNC_EVENT_USE_HANDLER_BASE( _event_name_ )			\
	using _event_name_::handler::handle;								\
/**/
# endif 

namespace logicmill {
namespace async {


template < class T, T Value, class... Args >
class event
{
public:
	using listener = std::function< void ( Args... ) >;

};

template< class E >
class handler;

template< class T, T Value, class... Args >
class handler< event< T, Value, Args... > >
{
public:
	using event_type =  event< T, Value, Args... >;
	using ptr = std::shared_ptr< handler >;

	virtual void handle( event_type defn, Args... args ) = 0;

};

template< class E, class Derived >
class handler_spec;

template< class Derived, class T, T Value, class... Args >
class handler_spec< event< T, Value, Args... >, Derived > 
	: public handler< event< T, Value, Args... > >
{
public:
	using event_type = event< T, Value, Args... >;
	using method_ptr_type = void ( Derived::* )( Args... );

	handler_spec( method_ptr_type mptr )
	:
	m_mptr{ mptr }
	{}

	virtual void handle( event_type , Args... args ) override
	{
		((static_cast< Derived& >(*this)).*m_mptr)( args... );
	}

protected:
	method_ptr_type m_mptr;
};

#if 0
template< class Derived >
class handler_spec : public handler
{
public:
	using method_ptr_type = void ( Derived::* )( Args... );

	handler_spec( method_ptr_type mptr )
	:
	m_mptr{ mptr }
	{}

	virtual void handle( event_definition , Args... args ) override
	{
		((static_cast< Derived& >(*this)).*m_mptr)( args... );
	}

protected:
	method_ptr_type m_mptr;
};
#endif

class emitter_base
{
public:
	virtual ~emitter_base() {}

	virtual void disconnect() {}
};

enum class cardinality
{
	simplex,
	multiplex
};

template< class E, cardinality K >
class emitter;

template< class T, T Value, class... Args >
class emitter< event< T, Value, Args... >, cardinality::simplex > : public emitter_base
{
public:
	using id_type = std::uint64_t;
	using event_type = event< T, Value, Args... >;
	using listener_type = typename event_type::listener;

	template< class U >
	std::enable_if_t< std::is_convertible< U, listener_type >::value , id_type >
	add_listener( event_type, U&& l )
	{
		id_type result = 0;
		if ( ! m_listener )
		{
			m_listener = std::forward< U >( l );
			m_listener_id = m_next_id++;
			result = m_listener_id;
		}
		return result;
	}

	id_type add_listener( event_type e, typename handler< event_type >::ptr hp )
	{
		return add_listener( e, [=] ( Args... args )
		{
			hp->handle( event_type{}, args... );
		} );
	}

	bool remove_listener( event_type, id_type id )
	{
		bool result = false;
		if ( id == m_listener_id )
		{
			m_listener = nullptr;
			m_listener_id = 0;
			result = true;
		}
		return result;
	}

	void remove_all( event_type )
	{
		m_listener = nullptr;
		m_listener_id = 0;
	}

	virtual void disconnect()
	{
		m_listener = nullptr;
		m_listener_id = 0;
	}

	std::size_t listener_count( event_type ) const
	{
		return ( m_listener ) ? 1 : 0;
	}

	void emit( event_type, Args... args )
	{
		if ( m_listener )
		{
			m_listener( args... );
		}
	}

private:
	id_type				m_next_id = 1;
	listener_type		m_listener = nullptr;
	id_type				m_listener_id = 0;
};

template< class T, T Value, class... Args >
class emitter< event< T, Value, Args... >, cardinality::multiplex > : public emitter_base
{
public:
	using id_type = std::uint64_t;
	using event_type = event< T, Value, Args... >;
	using listener_type = typename event_type::listener;
	using map_type = std::unordered_map< id_type, listener_type >;

	template< class U >
	id_type add_listener( event_type, U&& l )
	{
		id_type id = m_next_id++;
		m_listeners.emplace_back( id, std::forward< U >( l ) );
		return id;
	}

	id_type add_listener( event_type e, typename handler< event_type >::ptr hp )
	{
		return add_listener( e, [=] ( Args... args )
		{
			hp->handle( event_type{}, args... );
		} );
	}

	bool remove_listener( event_type, id_type id )
	{
		bool result = false;
		auto count = m_listeners.erase( id );
		return count == 1;
	}

	void remove_all( event_type )
	{
		clear();
	}

	virtual void disconnect()
	{
		clear();
	}

	std::size_t listener_count( event_type ) const
	{
		return m_listeners.size();
	}

	void emit( event_type, Args... args )
	{
		for ( auto it = m_listeners.begin(); it != m_listeners.end(); ++it )
		{
			(it->second)( args... );
		}
	}

private:

	void clear()
	{
		m_listeners.clear();
	}

	id_type				m_next_id = 1;
	map_type			m_listeners;
};



#if 0

enum class cardinality
{
	simplex,
	multiplex
};

class emitter_base
{
public:
	virtual ~emitter_base() {}

	virtual void disconnect() {}
};

template< class T, T Value, cardinality K, class... Args >
struct event_definition 
{
	using listener = std::function< void ( Args... ) >;

	class handler
	{
	public:
		using ptr = std::shared_ptr< handler >;

		virtual void handle( event_definition defn, Args... args ) = 0;
	};

	template< class Derived >
	class handler_spec : public handler
	{
	public:
		using method_ptr_type = void ( Derived::* )( Args... );

		handler_spec( method_ptr_type mptr )
		:
		m_mptr{ mptr }
		{}

		virtual void handle( event_definition , Args... args ) override
		{
        	((static_cast< Derived& >(*this)).*m_mptr)( args... );
		}

	protected:
		method_ptr_type m_mptr;
	};

	class emitter : public emitter_base
	{
		using id_type
	}

	class emitter : public emitter_base
	{
	public:

		using id_type = std::uint64_t;


		emitter( cardinality card = cardinality::multiplex )
		:
		m_is_simplex{ ( ( card == cardinality::simplex ) ? true : false ) },
		m_next_id{ 1 }
		{
			if ( m_is_simplex )
			{
				new ( & m_listeners.m_simplex ) simplex_listener();
			}
			else
			{
				new ( & m_listeners.m_multiplex ) listener_map();
			}
		}

		~emitter()
		{
			if ( m_is_simplex )
			{
				m_listeners.m_simplex.~simplex_listener();
			}
			else
			{
				m_listeners.m_multiplex.~listener_map();
			}
		}


		id_type add_listener( event_definition evt, typename handler::ptr h, std::error_code& err )
		{
			return add_listener( evt, err, [=] ( Args... args )
			{
				h->handle( event_definition{}, args... );
			} );
		}

		id_type add_listener( event_definition evt, typename handler::ptr h )
		{
			return add_listener( evt, [=] ( Args... args )
			{
				h->handle( event_definition{}, args... );
			} );
		}


		template< class T >
		id_type add_listener( event_definition, std::error_code& err, T&& l )
		{
			clear_error( err );
			id_type result = 0;
			if ( m_is_simplex )
			{
				if ( m_listeners.m_simplex.m_listener )
				{
					err = make_error_code( std::errc::already_connected );
					goto exit;
				}
				else
				{
					result = m_next_id++;
					m_listeners.m_simplex.m_id = result;
					m_listeners.m_simplex.m_listener = std::forward< T >( l );
				}
			}
			else
			{
				auto id = m_next_id++;
				m_listeners.m_multiplex.emplace( id, std::forward< T >( l ) );
				result = id;
			}

		exit:
			return result;
		}

		template< class T >
		id_type add_listener( event_definition, T&& l )
		{
			id_type result = 0;
			if ( m_is_simplex )
			{
				if ( m_listeners.m_simplex.m_listener )
				{
					throw std::system_error{ make_error_code( std::errc::already_connected ) };
				}
				else
				{
					result = m_next_id++;
					m_listeners.m_simplex.m_id = result;
					m_listeners.m_simplex.m_listener = std::forward< T >( l );
				}
			}
			else
			{
				auto id = m_next_id++;
				m_listeners.m_multiplex.emplace( id, std::forward< T >( l ) );
				result = id;
			}

			return result;
		}


		id_type remove_listener( event_definition, id_type id )
		{
			id_type result = 0;
			if ( m_is_simplex )
			{
				if ( m_Listeners.m_simplex.m_id == id )
				{
					m_listeners.m_simplex.m_id = 0;
					m_listeners.m_simplex.m_listener = nullptr;
					result = id;
				}
			}
			else
			{
				auto removed = m_listeners.m_multiplex.erase( id );
				if ( removed > 0 )
				{
					assert( removed == 1 );
					result = id;
				}
			}
			return result;
		}

		void emit( event_definition, Args... args )
		{
			for ( auto it = m_listeners.begin(); it != m_listeners.end(); ++it )
			{
				(it->second)( args... );
			}
		}

		std::size_t
		listener_count( event_definition ) const
		{
			return m_listeners.size();
		}

		std::size_t
		receiver_count( event_definition e ) const
		{
			return handler_count( e ) + listener_count( e );
		}

		void
		clear( event_definition )
		{
			m_handlers.clear();
			m_listeners.clear();
		}

		virtual void disconnect() override
		{
			m_handlers.clear();
			m_listeners.clear();
		}

	protected:
		id_type														m_next_id = 1;
		const bool													m_is_simplex;

		using listener_map = std::unordered_map< id_type, listener >;

		struct simplex_listener
		{
			id_type		m_id;
			listener	m_listener;
		};

		union listeners
		{
			listener_map						m_multiplex;
			simplex_listener					m_simplex;
		};

		listeners								m_listeners;
	};

};

template< class... Events >
class multi_emitter : public Events::emitter...
{
public:

	template< class E >
	void add_handler( E e, typename E::handler::ptr p )
	{
		E::emitter::add_handler( e, p );
	}

	template< class E >
	bool remove_handler( E e, typename E::handler::ptr p )
	{
		return E::emitter::remove_handler( e, p );
	}

	template< class E >
	typename E::emitter::id_type add_listener( E e, typename E::listener l )
	{
		return E::emitter::add_listener( e, l );
	}

	template< class E >
	bool remove_listener( E e, typename E::emitter::id_type id )
	{
		return E::emitter::remove_listener( e, E::emitter::id_type );
	}

	template< class E, class... Args >
	void emit( E e, Args... args )
	{
		E::emitter::emit( e, args... );
	}

	template< class E >
	std::size_t handler_count( E e ) const
	{
		return E::emitter::handler_count( e );
	}

	template< class E >
	std::size_t listener_count( E e ) const
	{
		return E::emitter::listener_count( e );
	}

	template< class E >
	std::size_t receiver_count( E e ) const
	{
		return E::emitter::receiver_count( e );
	}

	template< class Handler >
	void add_all_handlers( std::shared_ptr< Handler > hp )
	{
		add_multi_handlers< Handler, Events... >( hp );
	}

	template< class Handler >
	void remove_all_handlers( std::shared_ptr< Handler > hp )
	{
		remove_multi_handlers< Handler, Events... >( hp );
	}

	template< class Handler, class... _Events >
	void add_handlers( std::shared_ptr< Handler > hp, _Events... _evts )
	{
		add_multi_handlers< Handler, _Events... >( hp );
	}

	void clear_emitters()
	{
		clear_multi_emitters< Events... >();
	}

	template< class E >
	void clear( E e )
	{
		E::emitter::clear( e );
	}

	virtual void disconnect() override
	{
		clear_emitters();
	}

	std::size_t
	total_handler_count() const
	{
		return multi_handler_count< Events... >();
	}

	std::size_t
	total_listener_count() const
	{
		return multi_listener_count< Events... >();
	}

	std::size_t
	total_receiver_count() const
	{
		return total_handler_count() + total_listener_count();
	}

protected:

	template< class First, class... Rest >
	typename std::enable_if_t< ( sizeof...( Rest ) > 0 ), std::size_t >
	multi_handler_count() const
	{
		return handler_count( First{} ) + multi_handler_count< Rest... >();
	}

	template< class E >
	std::size_t multi_handler_count() const
	{
		return handler_count( E{} );
	}

	template< class First, class... Rest >
	typename std::enable_if_t< ( sizeof...( Rest ) > 0 ), std::size_t >
	multi_listener_count() const
	{
		return listener_count( First{} ) + multi_handler_count< Rest... >();
	}

	template< class E >
	std::size_t multi_listener_count() const
	{
		return listener_count( E{} );
	}

	template< class Handler, class First, class... Rest >
	typename std::enable_if_t< ( sizeof...( Rest ) > 0 ) > 
	add_multi_handlers( std::shared_ptr< Handler > hp )
	{
		First::emitter::add_handler( First{}, std::static_pointer_cast< typename First::handler >( hp ) );
		add_multi_handlers< Handler, Rest... >( hp );
	}

	template< class Handler, class E >
	void add_multi_handlers( std::shared_ptr< Handler > hp )
	{
		E::emitter::add_handler( E{}, hp );
	}

	template< class Handler, class First, class... Rest >
	typename std::enable_if_t< ( sizeof...( Rest ) > 0 ) > 
	remove_multi_handlers( std::shared_ptr< Handler > hp )
	{
		First::emitter::remove_handler( First{}, std::static_pointer_cast< typename First::handler >( hp ) );
		remove_multi_handlers< Handler, Rest... >( hp );
	}

	template< class Handler, class E >
	void remove_multi_handlers( std::shared_ptr< Handler > hp )
	{
		E::emitter::remove_handler( E{}, hp );
	}

	template< class First, class... Rest >
	typename std::enable_if_t< ( sizeof...( Rest ) > 0 ) >
	clear_multi_emitters()
	{
		First::emitter::clear( First{} );
		clear_multi_emitters< Rest... >();
	}

	template< class E >
	void clear_multi_emitters()
	{
		E::emitter::clear( E{} );
	}
};

#endif 

} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_EVENT_H
