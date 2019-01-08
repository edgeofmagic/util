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

mutable_buffer::mutable_buffer(const_buffer&& cbuf)
	:

	  buffer{cbuf.m_data, cbuf.m_size},
	  m_region{std::move(cbuf.m_region)},
	  m_capacity{m_region->capacity()}
{
	cbuf.m_data = nullptr;
	cbuf.m_size = 0;
	assert(m_data == m_region->data());
	ASSERT_MUTABLE_BUFFER_INVARIANTS(*this);
}
