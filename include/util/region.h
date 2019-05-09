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

#ifndef UTIL_REGION_H
#define UTIL_REGION_H

#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <util/macros.h>
#include <util/shared_ptr.h>
#include <util/types.h>

namespace util
{

class region
{
protected:
	region(byte_type* data, size_type capacity) : m_data{data}, m_capacity{capacity} {}

public:
	using sptr = util::shared_ptr<region>;
	using uptr = std::unique_ptr<region>;

	virtual ~region() {}

	byte_type*
	data()
	{
		return m_data;
	}

	size_type
	capacity()
	{
		return m_capacity;
	}

protected:
	byte_type* m_data;
	size_type  m_capacity;
};

class dynamic_region : public region
{
protected:
	dynamic_region(byte_type* data, size_type capacity) : region{data, capacity} {}

	virtual byte_type*
	alloc(size_type capacity)
			= 0;

	virtual void
	dealloc(byte_type* p, size_type capacity)
			= 0;

public:
	byte_type*
	allocate(size_type capacity, std::error_code& err)
	{
		err.clear();
		byte_type* p{nullptr};
		p = alloc(capacity);
		if (!p)
		{
			err = make_error_code(std::errc::no_buffer_space);
			goto exit;
		}
		m_data     = p;
		m_capacity = capacity;

	exit:
		return m_data;
	}

	byte_type*
	allocate(size_type capacity)
	{
		std::error_code err;
		auto            result = allocate(capacity, err);
		if (err)
		{
			throw std::system_error{err};
		}
		return result;
	}

	void
	deallocate()
	{
		dealloc(m_data, m_capacity);
		m_data     = nullptr;
		m_capacity = 0;
	}

	byte_type*
	reallocate(size_type new_capacity)
	{
		return reallocate(m_capacity, new_capacity);
	}

	byte_type*
	reallocate(size_type new_capacity, std::error_code& err)
	{
		return reallocate(m_capacity, new_capacity, err);
	}


	byte_type*
	reallocate(size_type current_size, size_type new_capacity)
	{
		std::error_code err;
		auto            result = reallocate(current_size, new_capacity, err);
		if (err)
		{
			throw std::system_error{err};
		}
		return result;
	}


	byte_type*
	reallocate(size_type current_size, size_type new_capacity, std::error_code& err)
	{
		err.clear();

		if (!m_data && new_capacity > 0)
		{
			auto p = alloc(new_capacity);
			if (!p)
			{
				err = make_error_code(std::errc::no_buffer_space);
				goto exit;
			}
			m_data     = p;
			m_capacity = new_capacity;
		}
		else if (new_capacity > m_capacity)
		{
			current_size = (current_size > m_capacity) ? m_capacity : current_size;
			auto p       = alloc(new_capacity);
			if (!p)
			{
				err = make_error_code(std::errc::no_buffer_space);
				goto exit;
			}
			::memcpy(p, m_data, current_size);
			dealloc(m_data, m_capacity);
			m_data     = p;
			m_capacity = new_capacity;
		}
	exit:
		return m_data;
	}
};

template<class Del>
class del_region : protected Del, public region
{
public:
	using deleter_base = Del;
	using deleter_type = Del;

	del_region(byte_type* data, size_type capacity, deleter_type const& del) : deleter_base{del}, region{data, capacity}
	{}

	del_region(byte_type* data, size_type capacity, deleter_type&& del)
		: deleter_base{std::move(del)}, region{data, capacity}
	{}

	template<class = std::enable_if_t<std::is_default_constructible<deleter_type>::value>>
	del_region(byte_type* data, size_type capacity) : deleter_base{deleter_type{}}, region{data, capacity}
	{}

	virtual ~del_region()
	{
		destroy();
		m_data = nullptr;
	}

protected:
	void
	destroy()
	{
		deleter_base::operator()(m_data);
	}
};


template<class Alloc = std::allocator<byte_type>>
class alloc_region : protected Alloc, public dynamic_region
{
public:
	using sptr = util::shared_ptr<alloc_region>;
	using uptr = std::unique_ptr<alloc_region>;

	using allocator_type = Alloc;
	using allocator_base = Alloc;

	alloc_region() : allocator_base{allocator_type{}}, dynamic_region{nullptr, 0} {}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	alloc_region(_Alloc&& alloc) : allocator_base{std::forward<_Alloc>(alloc)}, dynamic_region{nullptr, 0}
	{}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	alloc_region(size_type capacity, _Alloc&& alloc)
		: allocator_base{std::forward<_Alloc>(alloc)},
		  dynamic_region{((capacity > 0) ? allocator_base::allocate(capacity) : nullptr), capacity}
	{}

	alloc_region(size_type capacity)
		: allocator_base{allocator_type{}},
		  dynamic_region{((capacity > 0) ? allocator_base::allocate(capacity) : nullptr), capacity}
	{}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	alloc_region(const byte_type* data, size_type size, _Alloc&& alloc)
		: allocator_base{std::forward<_Alloc>(alloc)}, dynamic_region{allocator_base::allocate(size), size}
	{
		if (m_data && data && size > 0)
		{
			::memcpy(m_data, data, size);
		}
	}

	alloc_region(const byte_type* data, size_type size)
		: allocator_base{allocator_type{}}, dynamic_region{allocator_base::allocate(size), size}
	{
		if (m_data && data && size > 0)
		{
			::memcpy(m_data, data, size);
		}
	}

	template<class _Alloc, class = typename std::enable_if_t<std::is_same<typename _Alloc::pointer, byte_type*>::value>>
	alloc_region(byte_type* data, size_type capacity, _Alloc&& alloc)
		: allocator_base{std::forward<_Alloc>(alloc)}, dynamic_region{data, capacity}
	{}

	alloc_region(byte_type* data, size_type capacity) : allocator_base{allocator_type{}}, dynamic_region{data, capacity}
	{}

	virtual ~alloc_region()
	{
		dealloc(m_data, m_capacity);
		m_data     = nullptr;
		m_capacity = 0;
	}

protected:
	virtual byte_type*
	alloc(size_type capacity) override
	{
		return allocator_base::allocate(capacity);
	}

	virtual void
	dealloc(byte_type* p, size_type capacity) override
	{
		allocator_base::deallocate(p, capacity);
	}
};


template<size_type Size>
class fixed_region : public region
{
public:
	using sptr = util::shared_ptr<fixed_region>;
	using uptr = std::unique_ptr<fixed_region>;

	fixed_region() : region{m_bytes, Size} {}

private:
	byte_type m_bytes[Size];
};

template<class Alloc = std::allocator<byte_type>, class Enable = void>
class alloc_region_factory;

template<class Alloc>
class alloc_region_factory<
		Alloc,
		typename std::enable_if_t<
				traits::is_allocator<Alloc>::value && std::is_same<typename Alloc::pointer, byte_type*>::value>>
	: public Alloc
{
public:
	alloc_region_factory(Alloc&& alloc) : Alloc{std::move(alloc)} {}

	alloc_region_factory(Alloc const& alloc) : Alloc{alloc} {}

	alloc_region_factory() : Alloc{} {}

	std::unique_ptr<region>
	create(size_type capacity)
	{
		return std::make_unique<alloc_region<Alloc>>(capacity, static_cast<Alloc>(*this));
	}
};

class binned_fixed_region_factory
{
public:
	binned_fixed_region_factory() {}

	std::unique_ptr<region>
	create(size_type size) const
	{
		size = (size < 16) ? 16 : size;
		std::size_t   index{0};
		auto          blog = std::ilogb(size - 1);
		std::uint64_t mask = (size - 1) >> (blog - 1);
		if (mask == 3)
			index = blog * 2 - 6;
		else if (mask == 2)
			index = blog * 2 - 7;
		else
			throw std::invalid_argument{"unexpected mask result"};

		switch (index)
		{
			case 0:
				return std::make_unique<fixed_region<16>>();
			case 1:
				return std::make_unique<fixed_region<24>>();
			case 2:
				return std::make_unique<fixed_region<32>>();
			case 3:
				return std::make_unique<fixed_region<48>>();
			case 4:
				return std::make_unique<fixed_region<64>>();
			case 5:
				return std::make_unique<fixed_region<96>>();
			case 6:
				return std::make_unique<fixed_region<128>>();
			case 7:
				return std::make_unique<fixed_region<192>>();
			case 8:
				return std::make_unique<fixed_region<256>>();
			case 9:
				return std::make_unique<fixed_region<384>>();
			case 10:
				return std::make_unique<fixed_region<512>>();
			case 11:
				return std::make_unique<fixed_region<768>>();
			case 12:
				return std::make_unique<fixed_region<1024>>();
			case 13:
				return std::make_unique<fixed_region<1536>>();
			case 14:
				return std::make_unique<fixed_region<2048>>();
			case 15:
				return std::make_unique<fixed_region<3072>>();
			case 16:
				return std::make_unique<fixed_region<4096>>();
			case 17:
				return std::make_unique<fixed_region<6144>>();
			case 18:
				return std::make_unique<fixed_region<8192>>();
			case 19:
				return std::make_unique<fixed_region<12288>>();
			case 20:
				return std::make_unique<fixed_region<16384>>();
			case 21:
				return std::make_unique<fixed_region<24576>>();
			case 22:
				return std::make_unique<fixed_region<32768>>();
			case 23:
				return std::make_unique<fixed_region<49152>>();
			case 24:
				return std::make_unique<fixed_region<65536>>();
			case 25:
				return std::make_unique<fixed_region<98304>>();
			case 26:
				return std::make_unique<fixed_region<131072>>();
			case 27:
				return std::make_unique<fixed_region<196608>>();
			case 28:
				return std::make_unique<fixed_region<262144>>();
			case 29:
				return std::make_unique<fixed_region<393216>>();
			case 30:
				return std::make_unique<fixed_region<524288>>();
			case 31:
				return std::make_unique<fixed_region<786432>>();
			case 32:
				return std::make_unique<fixed_region<1048576>>();
			case 33:
				return std::make_unique<fixed_region<1572864>>();
			case 34:
				return std::make_unique<fixed_region<2097152>>();
			case 35:
				return std::make_unique<fixed_region<3145728>>();
			case 36:
				return std::make_unique<fixed_region<4194304>>();
			case 37:
				return std::make_unique<fixed_region<6291456>>();
			case 38:
				return std::make_unique<fixed_region<8388608>>();
			case 39:
				return std::make_unique<fixed_region<12582912>>();
			case 40:
				return std::make_unique<fixed_region<16777216>>();
			default:
				throw std::invalid_argument{"size exceeds maximum"};
		};
	}
};

template<size_type Size>
class fixed_region_factory
{
public:
	using region_type = fixed_region<Size>;

	fixed_region_factory() {}

	std::unique_ptr<region>
	create() const
	{
		return std::make_unique<region_type>();
	}
};

}    // namespace util

#endif    // UTIL_REGION_H