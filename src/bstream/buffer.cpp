#include <logicmill/bstream/buffer.h>
#include <boost/crc.hpp>
#include <logicmill/bstream/utils/dumpster.h>

using namespace logicmill;
using namespace bstream;

// const buffer::allocation_manager buffer::default_manager{ 
// 	buffer::default_allocator{},
// 	buffer::default_reallocator{},
// 	buffer::default_deallocator{} };

// const buffer::allocation_manager buffer::no_realloc_manager{ 
// 	buffer::default_allocator{},
// 	nullptr,
// 	buffer::default_deallocator{} };
	
// const buffer::allocation_manager buffer::null_manager{};
	
buffer::checksum_type
buffer::checksum( size_type offset, size_type length ) const
{
	
	boost::crc_32_type crc;
	if ( ( m_data != nullptr ) && ( offset < length ) && ( ( offset + length ) <= m_size ) )
	{
		crc.process_bytes( m_data + offset, length );
	}
	return crc.checksum();
}

void
buffer::dump( std::ostream& os ) const
{
	utils::dumpster{}.dump( os, m_data, m_size );
}
