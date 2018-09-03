#include <logicmill/buffer.h>
#include <boost/crc.hpp>

using namespace logicmill;

buffer::deallocator_func buffer::default_deallocator = []( elem_type *data )
{
	::free( data );
};

buffer::allocator_func buffer::default_allocator = []( elem_type *data, size_type, size_type new_cap )
{
	return ( data == nullptr ) ? reinterpret_cast< elem_type* >( ::malloc( new_cap ) ) : reinterpret_cast< elem_type* >( ::realloc( data, new_cap ) ); 
};

void
buffer::dump( std::ostream& /* os */ ) const
{
	// dumpster{}.dump( os, m_data, m_size );
}

buffer::checksum_type
buffer::checksum( /* size_type  offset, size_type length */ ) const
{
	// boost::crc_32_type crc;
	// if ( ( m_data != nullptr ) && ( offset < length ) && ( ( offset + length ) <= m_size ) )
	// {
	// 	crc.process_bytes( m_data + offset, length );
	// }
	// return crc.checksum();
	
	return 0;
}
