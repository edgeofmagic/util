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

#ifndef LOGICMILL_ASYNC_ENDPOINT_H
#define LOGICMILL_ASYNC_ENDPOINT_H

#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <logicmill/async/address.h>
#include <logicmill/async/error.h>

namespace logicmill {
namespace async {
namespace ip {

class endpoint
{
public:

	endpoint()
	:
	m_addr{},
	m_port{ 0 }
	{}
		
	endpoint( const address &host, std::uint16_t port )
	:
	m_addr{ host },
	m_port{ port }
	{}
		
	endpoint( const endpoint &rhs )
	:
	m_addr( rhs.m_addr ),
	m_port( rhs.m_port )
	{}

	endpoint( sockaddr_storage const& sockaddr )
	:
	m_addr{},
	m_port{ 0 }
	{
		if ( sockaddr.ss_family == AF_INET )
		{
			auto addr4 = reinterpret_cast< const sockaddr_in* >( &sockaddr );
			
			addr( address{ addr4->sin_addr } );
			port( ntohs( addr4->sin_port ) );
		}
		else if ( sockaddr.ss_family == AF_INET6 )
		{
			auto addr6 = reinterpret_cast< const sockaddr_in6* >( &sockaddr );
			addr( address{ addr6->sin6_addr } );
			port( ntohs( addr6->sin6_port ) );
		}
	}

	~endpoint()
	{}
	
	endpoint&
	operator=( const endpoint &rhs )
	{
		m_addr = rhs.m_addr;
		m_port = rhs.m_port;
		return *this;
	}
	
	bool
	operator==( const endpoint &rhs ) const
	{
		return ( m_addr == rhs.m_addr ) && ( m_port == rhs.m_port );
	}
	
	bool
	operator!=( const endpoint &rhs ) const
	{
		return ! ( *this == rhs );
	}
	
	std::string
	to_string() const
	{
		std::ostringstream os;

		if ( m_addr.is_v4() )
		{
			os << m_addr.to_string() << ":" << std::to_string( m_port );
		}
		else if ( m_addr.is_v6() )
		{
			os << "[" << m_addr.to_string() << "]:" << m_port;
		}
		return os.str();
	}

	bool
	is_v4() const
	{
		return m_addr.is_v4();
	}
	
	bool
	is_v6() const
	{
		return m_addr.is_v6();
	}
	
	const address&
	addr() const
	{
		return m_addr;
	}
	
	void
	addr( const ip::address &addr )
	{
		m_addr = addr;
	}
	
	std::uint16_t
	port() const
	{
		return m_port;
	}
	
	void
	port( std::uint16_t val )
	{
		m_port = val;
	}

	explicit operator bool () const
	{
		return ( m_addr && m_port );
	}

	void
	to_sockaddr( sockaddr_storage &sockaddr ) const
	{
		memset( &sockaddr, 0, sizeof( sockaddr ) );

		if ( is_v4() )
		{
			auto addr4 = reinterpret_cast< struct sockaddr_in* >( &sockaddr );
			
			addr4->sin_family	= AF_INET;
			addr() >> addr4->sin_addr;
			addr4->sin_port = htons( port() );
#if defined( __APPLE__ )
			addr4->sin_len = sizeof( sockaddr_in );
#endif
		}
		else if ( is_v6() )
		{
			auto addr6 = reinterpret_cast< sockaddr_in6* >( &sockaddr );
			
			addr6->sin6_family	= AF_INET6;
			addr() >> addr6->sin6_addr;
			addr6->sin6_port = htons( port() );
#if defined( __APPLE__ )
			addr6->sin6_len = sizeof( sockaddr_in6 );
#endif
		}
	}


	
protected:

	address			m_addr;
	std::uint16_t	m_port;
};

inline std::ostream&
operator<<( std::ostream &os, const ip::endpoint &endpoint )
{
	return os << endpoint.to_string();
}

} // namespace ip
} // namespace async
} // namespace logicmill

namespace std
{

template <>
struct hash< logicmill::async::ip::endpoint >
{
	typedef logicmill::async::ip::endpoint		argument_type;
	typedef std::size_t 						result_type;
 
	result_type operator()( const argument_type &v ) const
	{
		result_type res	= std::hash< logicmill::async::ip::address >()( v.addr() );		
		res = ( res << 1 ) + res + std::hash< std::uint16_t >{}( v.port() );
		return res;
    }
};

}

#endif // LOGICMILL_ASYNC_ENDPOINT_H
