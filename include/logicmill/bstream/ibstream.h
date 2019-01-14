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

#ifndef LOGICMILL_BSTREAM_IBSTREAM_H
#define LOGICMILL_BSTREAM_IBSTREAM_H

#include <boost/endian/conversion.hpp>
#include <deque>
#include <logicmill/bstream/context.h>
#include <logicmill/bstream/ibstream_traits.h>
#include <logicmill/bstream/source.h>
#include <logicmill/bstream/typecode.h>


namespace logicmill
{
namespace bstream
{

/*! \class ibstream
 *	\brief binary input stream
 *	
 *	An instance of ibstream is associated with a read-only buffer. The caller 
 *	can read from the stream in a variety of ways, depending on the calling 
 *	context and the type being read. At present, ibstream doesn't explicity
 *	support run-time polymorphism. The reading context is assumed to know
 *	\a a \a priori the contents of buffer as streamed by the sender ( that is,
 *	the types, number, and order of the items ).
 */
class ibstream
{
public:
	template<class U, class E>
	friend struct value_deserializer;

	using saved_ptr_info = std::pair<std::type_index, std::shared_ptr<void>>;

	class ptr_deduper
	{
	public:
		template<class T>
		void
		save_ptr(std::shared_ptr<T> ptr)
		{
			m_saved_ptrs.push_back(saved_ptr_info(typeid(*ptr), ptr));
		}

		saved_ptr_info const&
		get_saved_ptr(std::size_t index)
		{
			return m_saved_ptrs[index];
		}

		void
		clear()
		{
			m_saved_ptrs.clear();
		}

	private:
		std::deque<saved_ptr_info> m_saved_ptrs;
	};


	ibstream()                = delete;
	ibstream(ibstream const&) = delete;
	ibstream(ibstream&&)      = delete;

	ibstream(std::unique_ptr<bstream::source> strmbuf, context_base const& cntxt = get_default_context())
		: m_context{cntxt.get_context_impl()},
		  m_ptr_deduper{m_context->dedup_shared_ptrs() ? std::make_unique<ptr_deduper>() : nullptr},
		  m_strmbuf{std::move(strmbuf)}
		//   m_reverse_order{cntxt.get_context_impl()->byte_order() != bend::order::native}
	{}

	bstream::source&
	get_streambuf()
	{
		return *m_strmbuf.get();
	}

	bstream::source const&
	get_streambuf() const
	{
		return *m_strmbuf.get();
	}

	std::unique_ptr<bstream::source>
	release_streambuf()
	{
		return std::move(m_strmbuf);    // hope this is set to null
	}

	size_type
	size() const
	{
		return static_cast<size_type>(m_strmbuf->size());
	}

	position_type
	position() const
	{
		return static_cast<position_type>(m_strmbuf->position());
	}

	position_type
	position(position_type pos)
	{
		return static_cast<position_type>(m_strmbuf->position(pos));
	}

	position_type
	position(position_type pos, std::error_code& err)
	{
		return static_cast<position_type>(m_strmbuf->position(pos, err));
	}

	position_type
	position(offset_type offset, seek_anchor where)
	{
		return static_cast<position_type>(m_strmbuf->position(offset, where));
	}

	position_type
	position(offset_type offset, seek_anchor where, std::error_code& err)
	{
		return static_cast<position_type>(m_strmbuf->position(offset, where, err));
	}
	void
	rewind()
	{
		position(0);
	}

	void
	rewind(std::error_code& err)
	{
		position(0, err);
	}

	byte_type
	get()
	{
		return m_strmbuf->get();
	}

	byte_type
	get(std::error_code& err)
	{
		return m_strmbuf->get(err);
	}

	byte_type
	peek()
	{
		return m_strmbuf->peek();
	}

	byte_type
	peek(std::error_code& err)
	{
		return m_strmbuf->peek(err);
	}

	template<class U>
	typename std::enable_if<std::is_arithmetic<U>::value && sizeof(U) == 1, U>::type
	get_num()
	{
		return static_cast<U>(get());
	}

	template<class U>
	typename std::enable_if<std::is_arithmetic<U>::value && sizeof(U) == 1, U>::type
	get_num(std::error_code& err)
	{
		return static_cast<U>(get(err));
	}

	template<class U>
	typename std::enable_if<std::is_arithmetic<U>::value && (sizeof(U) > 1), U>::type
	get_num()
	{
		return m_strmbuf->get_num<U>();
	}

	template<class U>
	typename std::enable_if<std::is_arithmetic<U>::value && (sizeof(U) > 1), U>::type
	get_num(std::error_code& err)
	{
		return m_strmbuf->get_num<U>(err);
	}

	shared_buffer
	get_shared_slice(size_type nbytes)
	{
		return m_strmbuf->get_shared_slice(nbytes);
	}

	shared_buffer
	get_shared_slice(size_type nbytes, std::error_code& err)
	{
		return m_strmbuf->get_shared_slice(nbytes, err);
	}

	const_buffer
	get_slice(size_type nbytes)
	{
		return m_strmbuf->get_slice(nbytes);
	}

	const_buffer
	get_slice(size_type nbytes, std::error_code& err)
	{
		return m_strmbuf->get_slice(nbytes, err);
	}

	size_type
	getn(byte_type* dst, size_type nbytes)
	{
		return m_strmbuf->getn(dst, nbytes);
	}

	size_type
	getn(byte_type* dst, size_type nbytes, std::error_code& err)
	{
		return m_strmbuf->getn(dst, nbytes, err);
	}

	// bend::order
	// byte_order() const
	// {
	// 	return m_reverse_order ? ((bend::order::native == bend::order::little) ? bend::order::big : bend::order::little)
	// 						   : (bend::order::native);
	// }

	template<class T>
	typename std::enable_if_t<is_ibstream_constructible<T>::value, T>
	read_as()
	{
		return T(*this);
	}

	template<class T>
	typename std::enable_if_t<is_ibstream_constructible<T>::value && std::is_default_constructible<T>::value, T>
	read_as(std::error_code& ec)
	{
		ec.clear();
		try
		{
			return T(*this);
		}
		catch (std::system_error const& e)
		{
			ec = e.code();
			return T{};
		}
	}

	template<class T>
	typename std::enable_if_t<use_value_deserializer<T>::value, T>
	read_as()
	{
		return value_deserializer<T>::get(*this);
	}

	template<class T>
	typename std::enable_if_t<use_value_deserializer<T>::value && std::is_default_constructible<T>::value, T>
	read_as(std::error_code& ec)
	{
		ec.clear();
		try
		{
			return value_deserializer<T>::get(*this);
		}
		catch (std::system_error const& e)
		{
			ec = e.code();
			return T{};
		}
	}

	template<class T>
	typename std::enable_if_t<use_ref_deserializer<T>::value, T>
	read_as()
	{
		T obj;
		ref_deserializer<T>::get(*this, obj);
		return obj;
	}

	template<class T>
	typename std::enable_if_t<use_ref_deserializer<T>::value, T>
	read_as(std::error_code& ec)
	{
		ec.clear();
		T obj;
		try
		{
			ref_deserializer<T>::get(*this, obj);
		}
		catch (std::system_error const& e)
		{
			ec = e.code();
		}
		return obj;
	}

	template<class T>
	typename std::enable_if_t<has_ref_deserializer<T>::value, ibstream&>
	read_as(T& obj)
	{
		return ref_deserializer<T>::get(*this, obj);
	}

	template<class T>
	typename std::enable_if_t<has_ref_deserializer<T>::value, ibstream&>
	read_as(T& obj, std::error_code& ec)
	{
		ec.clear();
		try
		{
			ref_deserializer<T>::get(*this, obj);
		}
		catch (std::system_error const& e)
		{
			ec = e.code();
		}
		return *this;
	}

	template<class T>
	typename std::enable_if_t<
			!has_ref_deserializer<T>::value && is_ibstream_constructible<T>::value && std::is_assignable<T&, T>::value,
			ibstream&>
	read_as(T& obj)
	{
		obj = T(*this);
		return *this;
	}

	template<class T>
	typename std::enable_if_t<
			!has_ref_deserializer<T>::value && is_ibstream_constructible<T>::value && std::is_assignable<T&, T>::value,
			ibstream&>
	read_as(T& obj, std::error_code& ec)
	{
		ec.clear();
		try
		{
			obj = T(*this);
		}
		catch (std::system_error const& e)
		{
			ec = e.code();
		}
		return *this;
	}

	template<class T>
	typename std::enable_if_t<
			!has_ref_deserializer<T>::value && !is_ibstream_constructible<T>::value && has_value_deserializer<T>::value
					&& std::is_assignable<T&, T>::value,
			ibstream&>
	read_as(T& obj)
	{
		obj = value_deserializer<T>::get(*this);
		return *this;
	}

	template<class T>
	typename std::enable_if_t<
			!has_ref_deserializer<T>::value && !is_ibstream_constructible<T>::value && has_value_deserializer<T>::value
					&& std::is_assignable<T&, T>::value,
			ibstream&>
	read_as(T& obj, std::error_code& ec)
	{
		ec.clear();
		try
		{
			obj = value_deserializer<T>::get(*this);
		}
		catch (std::system_error const& e)
		{
			ec = e.code();
		}
		return *this;
	}

	std::size_t
	read_string_header();

	std::size_t
	read_string_header(std::error_code& ec);

	std::size_t
	read_array_header();

	std::size_t
	read_array_header(std::error_code& ec);

	std::size_t
	read_map_header();

	std::size_t
	read_map_header(std::error_code& ec);

	ibstream&
	check_map_key(std::string const& key);

	ibstream&
	check_map_key(std::string const& key, std::error_code& ec);

	ibstream&
	check_array_header(std::size_t expected);

	ibstream&
	check_array_header(std::size_t expected, std::error_code& ec);

	ibstream&
	check_map_header(std::size_t expected);

	ibstream&
	check_map_header(std::size_t expected, std::error_code& ec);

	std::size_t
	read_blob_header();

	std::size_t
	read_blob_header(std::error_code& ec);

	logicmill::shared_buffer
	read_blob_body_shared(std::size_t nbytes)
	{
		return get_shared_slice(nbytes);
	}

	logicmill::shared_buffer
	read_blob_body_shared(std::size_t nbytes, std::error_code& ec)
	{
		return get_shared_slice(nbytes, ec);
	}

	logicmill::const_buffer
	read_blob_body(std::size_t nbytes)
	{
		return get_slice(nbytes);
	}

	logicmill::const_buffer
	read_blob_body(std::size_t nbytes, std::error_code& ec)
	{
		return get_slice(nbytes, ec);
	}

	logicmill::shared_buffer
	read_blob_shared()
	{
		auto nbytes = read_blob_header();
		return read_blob_body_shared(nbytes);
	}

	logicmill::shared_buffer
	read_blob_shared(std::error_code& ec);

	logicmill::const_buffer
	read_blob(as_const_buffer tag)
	{
		auto nbytes = read_blob_header();
		return read_blob_body(nbytes);
	}

	logicmill::const_buffer
	read_blob(std::error_code& ec);

	std::size_t
	read_ext_header(std::uint8_t& ext_type);

	std::size_t
	read_ext_header(std::uint8_t& ext_type, std::error_code& ec);

	logicmill::shared_buffer
	read_ext_body_shared(std::size_t nbytes)
	{
		return get_shared_slice(nbytes);
	}

	logicmill::shared_buffer
	read_ext_body_shared(std::size_t nbytes, std::error_code& ec)
	{
		return get_shared_slice(nbytes, ec);
	}

	logicmill::const_buffer
	read_ext_body(std::size_t nbytes)
	{
		return get_slice(nbytes);
	}

	logicmill::const_buffer
	read_ext_body(std::size_t nbytes, std::error_code& ec)
	{
		return get_slice(nbytes, ec);
	}

	void
	read_nil()
	{
		auto tcode = get();
		if (tcode != typecode::nil)
		{
			throw std::system_error{make_error_code(bstream::errc::expected_nil)};
		}
	}

	void
	read_nil(std::error_code& ec)
	{
		ec.clear();
		auto tcode = get();
		if (tcode != typecode::nil)
		{
			ec = make_error_code(bstream::errc::expected_nil);
		}
	}

	void
	reset()
	{
		if (m_ptr_deduper)
			m_ptr_deduper->clear();
		position(0);
	}

	void
	reset(std::error_code& err)
	{
		if (m_ptr_deduper)
			m_ptr_deduper->clear();
		position(0, err);
	}

	const_buffer
	get_msgpack_obj_buf();

	const_buffer
	get_msgpack_obj_buf(std::error_code& ec)
	{
		ec.clear();
		try
		{
			return get_msgpack_obj_buf();
		}
		catch (std::system_error const& e)
		{
			ec = e.code();
			return const_buffer{};
		}
	}

protected:
	void
	use(std::unique_ptr<bstream::source> strmbuf)
	{
		m_strmbuf = std::move(strmbuf);
	}

	template<class T, class... Args>
	typename std::enable_if_t<std::is_base_of<bstream::source, T>::value>
	use(Args&&... args)
	{
		m_strmbuf = std::make_unique<T>(std::forward<Args>(args)...);
	}

	/*
     *  Pointers are streamed as a 2 - element object ( array or map ).
     *  The first element is the type tag ( int ). If the tag is -1,
     *  then there is no run-time type information in the stream;
     *  the object should be constructed based on the assumption that
     *  it is an instance of T. If it is not -1, then it is interpreted
     *  as a type tag, and the ibstream instance is expected to have
     *  a poly_context that can create an instance from this tag.
     * 
     *  The second element is either a nil, a positive integer value, 
     *  or a serialized object ( array or map ). If it is nil, a nullptr
     *  value is returned. 
     *  If it is an integer,
     *  it is interpreted as an id associated with a previously-
     *  stream object, which is expected to be stored in the stream 
     *  graph context.
     *  If it is an object, an instance of the appropriate type
     *  ( as indicated by the parameter T and/or the tag value as
     *  interpreted by the poly_contexts ) is constructed from the 
     *  streamed object, cast to the return type ( possibly mediated 
     *  by the poly_context ), and returned.
     * 
     */
	template<class T>
	std::shared_ptr<T>
	read_as_shared_ptr();

	template<class T>
	std::unique_ptr<T>
	read_as_unique_ptr();

	template<class T>
	std::shared_ptr<T>
	deserialize_as_shared_ptr(std::enable_if_t<std::is_abstract<T>::value, poly_tag_type> tag)
	{
		std::shared_ptr<T> result{nullptr};

		if (tag == invalid_tag)
		{
			throw std::system_error{make_error_code(bstream::errc::abstract_non_poly_class)};
		}

		result = m_context->create_shared<T>(tag, *this);

		if (m_ptr_deduper)
		{
			m_ptr_deduper->save_ptr(result);
		}

		return result;
	}

	template<class T>
	std::shared_ptr<T>
	deserialize_as_shared_ptr(std::enable_if_t<!std::is_abstract<T>::value, poly_tag_type> tag)
	{
		std::shared_ptr<T> result{nullptr};

		if (tag == invalid_tag)    // read as T
		{
			result = shared_ptr_deserializer<T>::get(*this);
		}
		else
		{
			result = m_context->create_shared<T>(tag, *this);
		}

		if (m_ptr_deduper)
		{
			m_ptr_deduper->save_ptr(result);
		}

		return result;
	}

	template<class T>
	std::unique_ptr<T>
	deserialize_as_unique_ptr(std::enable_if_t<std::is_abstract<T>::value, poly_tag_type> tag)
	{
		if (tag == invalid_tag)    // read as T
		{
			throw std::system_error{make_error_code(bstream::errc::abstract_non_poly_class)};
		}
		else
		{
			return std::unique_ptr<T>(m_context->create_raw<T>(tag, *this));
		}
	}

	template<class T>
	std::unique_ptr<T>
	deserialize_as_unique_ptr(std::enable_if_t<!std::is_abstract<T>::value, poly_tag_type> tag)
	{
		if (tag == invalid_tag)    // read as T
		{
			return unique_ptr_deserializer<T>::get(*this);
		}
		else
		{
			return std::unique_ptr<T>(m_context->create_raw<T>(tag, *this));
		}
	}

	template<class T>
	std::shared_ptr<T>
	get_saved_ptr(int type_tag);

	std::error_code
	read_error_code();

	void
	ingest(bufwriter& os);

	std::unique_ptr<bufwriter>               m_bufwriter = nullptr;
	std::shared_ptr<const context_impl_base> m_context;
	std::unique_ptr<ptr_deduper>             m_ptr_deduper;
	std::unique_ptr<bstream::source>         m_strmbuf;
};

template<class T>
inline typename std::enable_if_t<has_ref_deserializer<T>::value, ibstream&>
operator>>(ibstream& is, T& obj)
{
	return ref_deserializer<T>::get(is, obj);
}

template<class T>
inline typename std::enable_if_t<
		!has_ref_deserializer<T>::value && is_ibstream_constructible<T>::value && std::is_assignable<T&, T>::value,
		ibstream&>
operator>>(ibstream& is, T& obj)
{
	obj = T(is);
	return is;
}

template<class T>
inline typename std::enable_if_t<
		!has_ref_deserializer<T>::value && !is_ibstream_constructible<T>::value && has_value_deserializer<T>::value
				&& std::is_assignable<T&, T>::value,
		ibstream&>
operator>>(ibstream& is, T& obj)
{
	obj = value_deserializer<T>::get(is);
	return is;
}


#if 1
template<class T>
struct value_deserializer<
		T,
		std::enable_if_t<std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed && sizeof(T) == 1>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode <= typecode::positive_fixint_max)
		{
			return static_cast<T>(tcode);
		}
		else if (tcode >= typecode::negative_fixint_min)
		{
			return static_cast<T>(static_cast<std::int8_t>(tcode));
		}
		else
		{
			switch (tcode)
			{
				case typecode::bool_true:
					return static_cast<T>(1);
				case typecode::bool_false:
					return static_cast<T>(0);
				case typecode::int_8:
					return static_cast<T>(static_cast<std::int8_t>(is.get()));
				default:
					throw std::system_error{make_error_code(bstream::errc::num_deser_type_error_int8)};
			}
		}
	}
};

template<class T>
struct value_deserializer<
		T,
		std::enable_if_t<std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed && sizeof(T) == 1>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode <= typecode::positive_fixint_max)
		{
			return static_cast<T>(tcode);
		}
		else
		{
			switch (tcode)
			{
				case typecode::bool_true:
					return static_cast<T>(1);
				case typecode::bool_false:
					return static_cast<T>(0);
				case typecode::uint_8:
					return static_cast<T>(is.get());
				default:
					throw std::system_error{make_error_code(bstream::errc::num_deser_type_error_uint8)};
			}
		}
	}
};

template<class T>
struct value_deserializer<
		T,
		std::enable_if_t<std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed && sizeof(T) == 2>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode <= typecode::positive_fixint_max)
		{
			return static_cast<T>(tcode);
		}
		else if (tcode >= typecode::negative_fixint_min)
		{
			return static_cast<T>(static_cast<std::int8_t>(tcode));
		}
		else
		{
			switch (tcode)
			{
				case typecode::bool_true:
					return static_cast<T>(1);
				case typecode::bool_false:
					return static_cast<T>(0);
				case typecode::int_8:
					return static_cast<T>(is.get_num<int8_t>());
				case typecode::uint_8:
					return static_cast<T>(is.get_num<uint8_t>());
				case typecode::int_16:
					return static_cast<T>(is.get_num<int16_t>());
				case typecode::uint_16:
				{
					std::uint16_t n = is.get_num<uint16_t>();
					if (n <= std::numeric_limits<T>::max())
					{
						return static_cast<T>(n);
					}
					else
					{
						throw std::system_error{make_error_code(bstream::errc::num_deser_range_error_int16)};
					}
				}
				default:
					throw std::system_error{make_error_code(bstream::errc::num_deser_type_error_int16)};
			}
		}
	}
};

template<class T>
struct value_deserializer<
		T,
		std::enable_if_t<std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed && sizeof(T) == 2>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode <= typecode::positive_fixint_max)
		{
			return static_cast<T>(tcode);
		}
		else
		{
			switch (tcode)
			{
				case typecode::bool_true:
					return static_cast<T>(1);
				case typecode::bool_false:
					return static_cast<T>(0);
				case typecode::uint_8:
					return static_cast<T>(is.get_num<uint8_t>());
				case typecode::uint_16:
					return static_cast<T>(is.get_num<uint16_t>());
				default:
					throw std::system_error{make_error_code(bstream::errc::num_deser_type_error_uint16)};
			}
		}
	}
};

template<class T>
struct value_deserializer<
		T,
		std::enable_if_t<std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed && sizeof(T) == 4>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode <= typecode::positive_fixint_max)
		{
			return static_cast<T>(tcode);
		}
		else if (tcode >= typecode::negative_fixint_min)
		{
			return static_cast<T>(static_cast<std::int8_t>(tcode));
		}
		else
		{
			switch (tcode)
			{
				case typecode::bool_true:
					return static_cast<T>(1);
				case typecode::bool_false:
					return static_cast<T>(0);
				case typecode::int_8:
					return static_cast<T>(is.get_num<int8_t>());
				case typecode::uint_8:
					return static_cast<T>(is.get_num<uint8_t>());
				case typecode::int_16:
					return static_cast<T>(is.get_num<int16_t>());
				case typecode::uint_16:
					return static_cast<T>(is.get_num<uint16_t>());
				case typecode::int_32:
					return static_cast<T>(is.get_num<int32_t>());
				case typecode::uint_32:
				{
					std::uint32_t n = is.get_num<uint32_t>();
					if (n <= std::numeric_limits<T>::max())
					{
						return static_cast<T>(n);
					}
					else
					{
						throw std::system_error{make_error_code(bstream::errc::num_deser_range_error_int32)};
					}
				}
				default:
					throw std::system_error{make_error_code(bstream::errc::num_deser_type_error_int32)};
			}
		}
	}
};

template<class T>
struct value_deserializer<
		T,
		std::enable_if_t<std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed && sizeof(T) == 4>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode <= typecode::positive_fixint_max)
		{
			return static_cast<T>(tcode);
		}
		else
		{
			switch (tcode)
			{
				case typecode::bool_true:
					return static_cast<T>(1);
				case typecode::bool_false:
					return static_cast<T>(0);
				case typecode::uint_8:
					return static_cast<T>(is.get_num<uint8_t>());
				case typecode::uint_16:
					return static_cast<T>(is.get_num<uint16_t>());
				case typecode::uint_32:
					return static_cast<T>(is.get_num<uint32_t>());
				default:
					throw std::system_error{make_error_code(bstream::errc::num_deser_type_error_uint32)};
			}
		}
	}
};

template<class T>
struct value_deserializer<
		T,
		std::enable_if_t<std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed && sizeof(T) == 8>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode <= typecode::positive_fixint_max)
		{
			return static_cast<T>(tcode);
		}
		else if (tcode >= typecode::negative_fixint_min)
		{
			return static_cast<T>(static_cast<std::int8_t>(tcode));
		}
		else
		{
			switch (tcode)
			{
				case typecode::bool_true:
					return static_cast<T>(1);
				case typecode::bool_false:
					return static_cast<T>(0);
				case typecode::int_8:
					return static_cast<T>(is.get_num<int8_t>());
				case typecode::uint_8:
					return static_cast<T>(is.get_num<uint8_t>());
				case typecode::int_16:
					return static_cast<T>(is.get_num<int16_t>());
				case typecode::uint_16:
					return static_cast<T>(is.get_num<uint16_t>());
				case typecode::int_32:
					return static_cast<T>(is.get_num<int32_t>());
				case typecode::uint_32:
					return static_cast<T>(is.get_num<uint32_t>());
				case typecode::int_64:
					return static_cast<T>(is.get_num<int64_t>());
				case typecode::uint_64:
				{
					std::uint64_t n = is.get_num<uint64_t>();
					if (n <= std::numeric_limits<T>::max())
					{
						return static_cast<T>(n);
					}
					else
					{
						throw std::system_error{make_error_code(bstream::errc::num_deser_range_error_int64)};
					}
				}
				default:
					throw std::system_error{make_error_code(bstream::errc::num_deser_type_error_int64)};
			}
		}
	}
};

template<class T>
struct value_deserializer<
		T,
		std::enable_if_t<std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed && sizeof(T) == 8>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode <= typecode::positive_fixint_max)
		{
			return static_cast<T>(tcode);
		}
		else
		{
			switch (tcode)
			{
				case typecode::bool_true:
					return static_cast<T>(1);
				case typecode::bool_false:
					return static_cast<T>(0);
				case typecode::uint_8:
					return static_cast<T>(is.get_num<uint8_t>());
				case typecode::uint_16:
					return static_cast<T>(is.get_num<uint16_t>());
				case typecode::uint_32:
					return static_cast<T>(is.get_num<uint32_t>());
				case typecode::uint_64:
					return static_cast<T>(is.get_num<uint64_t>());
				default:
					throw std::system_error{make_error_code(bstream::errc::num_deser_type_error_uint64)};
			}
		}
	}
};

template<>
struct value_deserializer<std::string>
{
	std::string
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::string
	get(ibstream& is)
	{
		auto        tcode  = is.get();
		std::size_t length = 0;
		if (tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max)
		{
			std::uint8_t mask = 0x1f;
			length            = tcode & mask;
		}
		else
		{
			switch (tcode)
			{
				case typecode::str_8:
				{
					length = is.get_num<std::uint8_t>();
				}
				case typecode::str_16:
				{
					length = is.get_num<std::uint16_t>();
				}
				case typecode::str_32:
				{
					length = is.get_num<std::uint32_t>();
				}
				default:
					throw std::system_error{make_error_code(bstream::errc::val_deser_type_error_string)};
			}
		}
		byte_type strchars[length];
		is.getn(strchars, length);
		return std::string{reinterpret_cast<char*>(&strchars), length};
	}
};

template<>
struct value_deserializer<logicmill::string_alias>
{
	logicmill::string_alias
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static logicmill::string_alias
	get(ibstream& is, std::size_t length)
	{
		return logicmill::string_alias{is.get_shared_slice(length)};
	}

	static logicmill::string_alias
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max)
		{
			std::uint8_t mask   = 0x1f;
			std::size_t  length = tcode & mask;
			return get(is, length);
		}
		else
		{
			switch (tcode)
			{
				case typecode::str_8:
				{
					std::size_t length = is.get_num<std::uint8_t>();
					return get(is, length);
				}
				case typecode::str_16:
				{
					std::size_t length = is.get_num<std::uint16_t>();
					return get(is, length);
				}
				case typecode::str_32:
				{
					std::size_t length = is.get_num<std::uint32_t>();
					return get(is, length);
				}
				default:
					throw std::system_error{make_error_code(bstream::errc::val_deser_type_error_string_view)};
			}
		}
	}
};

#endif

template<class T>
struct value_deserializer<T, std::enable_if_t<std::is_enum<T>::value>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		auto ut = is.read_as<typename std::underlying_type<T>::type>();
		return static_cast<T>(ut);
	}
};

template<class T>
struct ref_deserializer<T, std::enable_if_t<has_deserialize_method<T>::value>>
{

	ibstream&
	operator()(ibstream& is, T& obj) const
	{
		return get(is, obj);
	}

	static ibstream&
	get(ibstream& is, T& obj)
	{
		return obj.deserialize(is);
	}
};

template<class T>
struct value_deserializer<T, std::enable_if_t<is_ibstream_constructible<T>::value>>
{
	T
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T
	get(ibstream& is)
	{
		return T{is};
	}
};

template<>
struct value_deserializer<bool>
{
	bool
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static bool
	get(ibstream& is)
	{
		auto tcode = is.get();
		switch (tcode)
		{
			case typecode::bool_true:
				return true;
			case typecode::bool_false:
				return false;
			default:
				throw std::system_error{make_error_code(bstream::errc::expected_bool)};
		}
	}
};

template<>
struct value_deserializer<float>
{
	float
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static float
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode == typecode::float_32)
		{
			std::uint32_t unpacked = is.get_num<std::uint32_t>();
			return reinterpret_cast<float&>(unpacked);
		}
		else
		{
			throw std::system_error{make_error_code(bstream::errc::expected_float)};
		}
	}
};

template<>
struct value_deserializer<double>
{
	double
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static double
	get(ibstream& is)
	{
		auto tcode = is.get();
		if (tcode == typecode::float_32)
		{
			std::uint32_t unpacked = is.get_num<std::uint32_t>();
			return static_cast<double>(reinterpret_cast<float&>(unpacked));
		}
		else if (tcode == typecode::float_64)
		{
			std::uint64_t unpacked = is.get_num<std::uint64_t>();
			return reinterpret_cast<double&>(unpacked);
		}
		else
		{
			throw std::system_error{make_error_code(bstream::errc::expected_double)};
		}
	}
};


/*
 *	Value deserializers for shared pointer types
 */

/*
 *	Prefer stream-constructed when available
 */

template<class T>
struct value_deserializer<std::shared_ptr<T>>
{
	std::shared_ptr<T>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::shared_ptr<T>
	get(ibstream& is)
	{
		return is.read_as_shared_ptr<T>();
	}
};

template<class T>
struct ptr_deserializer<T, typename std::enable_if_t<is_ibstream_constructible<T>::value>>
{
	T*
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T*
	get(ibstream& is)
	{
		return new T{is};
	}
};

template<class T>
struct shared_ptr_deserializer<T, typename std::enable_if_t<is_ibstream_constructible<T>::value>>
{
	std::shared_ptr<T>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::shared_ptr<T>
	get(ibstream& is)
	{
		return std::make_shared<T>(is);
	}
};

template<class T>
struct unique_ptr_deserializer<T, typename std::enable_if_t<is_ibstream_constructible<T>::value>>
{
	std::unique_ptr<T>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::unique_ptr<T>
	get(ibstream& is)
	{
		return std::make_unique<T>(is);
	}
};

/*
 *	If no stream constructor is available, prefer a value_deserializer< T >
 * 
 *	The value_deserializer< shared_ptr< T > > based on a value_deserializer< T > 
 *	requires a move contructor for T
 */

template<class T>
struct ptr_deserializer<
		T,
		typename std::enable_if_t<
				!is_ibstream_constructible<T>::value && has_value_deserializer<T>::value
				&& std::is_move_constructible<T>::value>>
{
	T*
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T*
	get(ibstream& is)
	{
		return new T{value_deserializer<T>::get(is)};
	}
};

template<class T>
struct shared_ptr_deserializer<
		T,
		typename std::enable_if_t<
				!is_ibstream_constructible<T>::value && has_value_deserializer<T>::value
				&& std::is_move_constructible<T>::value>>
{
	std::shared_ptr<T>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::shared_ptr<T>
	get(ibstream& is)
	{
		return std::make_shared<T>(value_deserializer<T>::get(is));
	}
};

template<class T>
struct unique_ptr_deserializer<
		T,
		typename std::enable_if_t<
				!is_ibstream_constructible<T>::value && has_value_deserializer<T>::value
				&& std::is_move_constructible<T>::value>>
{
	std::unique_ptr<T>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::unique_ptr<T>
	get(ibstream& is)
	{
		return std::make_unique<T>(value_deserializer<T>::get(is));
	}
};

/*
 *	The value_deserializer< shared_ptr< T > > based on a ref_deserializer< T > 
 *	requires eiher a move contructor or copy constructor for T;
 *	prefer the move contructor
 */

template<class T>
struct ptr_deserializer<
		T,
		typename std::enable_if_t<
				!is_ibstream_constructible<T>::value && !has_value_deserializer<T>::value
				&& has_ref_deserializer<T>::value && std::is_default_constructible<T>::value>>
{
	T*
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static T*
	get(ibstream& is)
	{
		T* ptr = new T;
		ref_deserializer<T>::get(is, *ptr);
		return ptr;
	}
};

template<class T>
struct shared_ptr_deserializer<
		T,
		typename std::enable_if_t<
				!is_ibstream_constructible<T>::value && !has_value_deserializer<T>::value
				&& has_ref_deserializer<T>::value && std::is_default_constructible<T>::value>>
{
	std::shared_ptr<T>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::shared_ptr<T>
	get(ibstream& is)
	{
		std::shared_ptr<T> ptr = std::make_shared<T>();
		ref_deserializer<T>::get(is, *ptr);
		return ptr;
	}
};

template<class T>
struct unique_ptr_deserializer<
		T,
		typename std::enable_if_t<
				!is_ibstream_constructible<T>::value && !has_value_deserializer<T>::value
				&& has_ref_deserializer<T>::value && std::is_default_constructible<T>::value>>
{
	std::unique_ptr<T>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::unique_ptr<T>
	get(ibstream& is)
	{
		std::unique_ptr<T> ptr = std::make_unique<T>();
		ref_deserializer<T>::get(is, *ptr);
		return ptr;
	}
};

/*
 *	Value deserializers for unique pointer types
 */

template<class T>
struct value_deserializer<std::unique_ptr<T>>
{
	std::unique_ptr<T>
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::unique_ptr<T>
	get(ibstream& is)
	{
		return is.read_as_unique_ptr<T>();
	}
};

template<>
struct value_deserializer<logicmill::const_buffer>
{
	logicmill::const_buffer
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static logicmill::const_buffer
	get(ibstream& is)
	{
		return is.read_blob(as_const_buffer{});
	}
};

template<>
struct value_deserializer<logicmill::shared_buffer>
{
	logicmill::shared_buffer
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static logicmill::shared_buffer
	get(ibstream& is)
	{
		return is.read_blob_shared();
	}
};

template<>
struct value_deserializer<std::error_code>
{
	std::error_code
	operator()(ibstream& is) const
	{
		return get(is);
	}

	static std::error_code
	get(ibstream& is)
	{
		return is.read_error_code();
	}
};

template<class T>
struct ibstream_initializer<T, typename std::enable_if_t<is_ibstream_constructible<T>::value>>
{
	using param_type  = ibstream&;
	using return_type = ibstream&;
	static return_type
	get(param_type is)
	{
		return is;
	}
};

template<class T>
struct ibstream_initializer<
		T,
		typename std::enable_if_t<!is_ibstream_constructible<T>::value && has_value_deserializer<T>::value>>
{
	using param_type  = ibstream&;
	using return_type = T;
	static return_type
	get(param_type is)
	{
		return value_deserializer<T>::get(is);
	}
};

template<class T>
struct ibstream_initializer<
		T,
		typename std::enable_if_t<
				!is_ibstream_constructible<T>::value && !has_value_deserializer<T>::value
				&& std::is_default_constructible<T>::value
				&& (std::is_copy_constructible<T>::value || std::is_move_constructible<T>::value)
				&& has_ref_deserializer<T>::value>>
{
	using param_type  = ibstream&;
	using return_type = T;
	static return_type
	get(param_type is)
	{
		T obj;
		ref_deserializer<T>::get(is, obj);
		return obj;
	}
};

template<class T>
std::shared_ptr<T>
ibstream::read_as_shared_ptr()
{
	std::shared_ptr<T> result{nullptr};
	auto               n = read_array_header();
	if (n != 2)
	{
		throw std::system_error{make_error_code(bstream::errc::invalid_header_for_shared_ptr)};
	}

	auto type_tag = read_as<poly_tag_type>();
	auto code     = peek();
	if (code == typecode::nil)    // return result ( nullptr ) as is
	{
		code = get();
	}
	else if (typecode::is_positive_int(code))    // saved ptr
	{
		result = get_saved_ptr<T>(type_tag);
	}
	else    // not saved ptr
	{
		result = deserialize_as_shared_ptr<T>(type_tag);
	}
	return result;
}

template<class T>
std::unique_ptr<T>
ibstream::read_as_unique_ptr()
{
	auto n = read_array_header();
	if (n != 2)
	{
		throw std::system_error{make_error_code(bstream::errc::invalid_header_for_unique_ptr)};
	}

	auto tag  = read_as<poly_tag_type>();
	auto code = peek();
	if (code == typecode::nil)    // nullptr
	{
		code = get();
		return nullptr;
	}
	else    // streamed object
	{
		return deserialize_as_unique_ptr<T>(tag);
	}
}

template<class T>
std::shared_ptr<T>
ibstream::get_saved_ptr(int type_tag)
{
	std::shared_ptr<T> result{nullptr};

	if (m_ptr_deduper)
	{
		auto index = read_as<std::size_t>();
		auto info  = m_ptr_deduper->get_saved_ptr(index);
		if (type_tag > -1)
		{
			auto saved_tag = m_context->get_type_tag(info.first);
			if (saved_tag != type_tag)
			{
				throw std::system_error{make_error_code(bstream::errc::dynamic_type_mismatch_in_saved_ptr)};
			}
			if (m_context->can_downcast_ptr(type_tag, m_context->get_type_tag(typeid(T))))
			{
				result = std::static_pointer_cast<T>(info.second);
			}
			else
			{
				throw std::system_error{make_error_code(bstream::errc::invalid_ptr_downcast)};
			}
		}
		else    // no type info in stream
		{
			if (info.first == typeid(T))
			{
				result = std::static_pointer_cast<T>(info.second);
			}
			else
			{
				throw std::system_error{make_error_code(bstream::errc::static_type_mismatch_in_saved_ptr)};
			}
		}
	}
	else
	{
		throw std::system_error{make_error_code(bstream::errc::context_mismatch)};
	}
	return result;
}

}    // namespace bstream
}    // namespace logicmill

#endif    // LOGICMILL_BSTREAM_IBSTREAM_H
