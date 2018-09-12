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
#include <functional>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>

#define LOGICMILL_ASYNC_EVENT_USE_EMITTER_BASE( _event_name_ )			\
	using _event_name_::emitter::add_handler;							\
	using _event_name_::emitter::remove_handler;						\
	using _event_name_::emitter::add_listener;							\
	using _event_name_::emitter::remove_listener;						\
	using _event_name_::emitter::emit;									\
	using _event_name_::emitter::handler_count;							\
	using _event_name_::emitter::listener_count;						\
	using _event_name_::emitter::receiver_count;						\
	using _event_name_::emitter::clear;									\
/**/

#define LOGICMILL_ASYNC_EVENT_USE_HANDLER_BASE( _event_name_ )			\
	using _event_name_::handler::handle;								\
/**/

namespace logicmill {
namespace async {

class emitter_base
{
public:
	virtual ~emitter_base() {}

	virtual void disconnect() {}
};

template< class T, T Value, class... Args >
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
	public:

		using id_type = std::uint64_t;

		void add_handler( event_definition, typename handler::ptr h )
		{
			m_handlers.insert( h );
		}

		bool remove_handler( event_definition, typename handler::ptr h )
		{
			return m_handlers.erase( h );
		}


		id_type add_listener( event_definition, listener l )
		{
			auto id = m_next_id;
			++m_next_id;
			m_listeners.emplace( id, std::move( l ) );
			return id;
		}

		bool remove_listener( event_definition, id_type id )
		{
			return m_listeners.erase( id );
		}

		void emit( event_definition, Args... args )
		{
			event_definition e;
			for ( auto it = m_handlers.begin(); it != m_handlers.end(); ++it )
			{
				(*it)->handle( e, args... );
			}

			for ( auto it = m_listeners.begin(); it != m_listeners.end(); ++it )
			{
				(it->second)( args... );
			}
		}

		std::size_t
		handler_count( event_definition ) const
		{
			return m_handlers.size();
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
		std::unordered_set< typename handler::ptr >					m_handlers;
		std::unordered_map< id_type, listener >						m_listeners;
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


} // namespace async
} // namespace logicmill

#endif // LOGICMILL_ASYNC_EVENT_H
