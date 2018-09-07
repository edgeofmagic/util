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

#include <logicmill/bstream/error.h>

using namespace logicmill;
// using namespace bstream;


class bstream_category_impl : public std::error_category
{
public:
	virtual const char* 
	name() const noexcept override;

	virtual std::string 
	message( int ev ) const noexcept override;

	virtual std::error_condition
	default_error_condition(int ev) const noexcept override;
};

const char* 
bstream_category_impl::name() const noexcept
{
    return "bstream";
}

std::string 
bstream_category_impl::message(int ev ) const noexcept
{
    switch ( static_cast< bstream::errc > (ev ) )
    {
		case bstream::errc::ok:
			return "success";
		case bstream::errc::read_past_end_of_stream:
			return "read past end of stream";
		case bstream::errc::type_error:
			return "type error";
		case bstream::errc::member_count_error:
			return "member count error";
		case bstream::errc::context_mismatch:
			return "context mismatch";
		case bstream::errc::invalid_err_category:
			return "invalid error category";
		case bstream::errc::invalid_ptr_downcast:
			return "invalid pointer downcast";
		case bstream::errc::abstract_non_poly_class:
			return "abstract class not found in polymorphic context";
		case bstream::errc::invalid_operation:
			return "invalid operation";
		case bstream::errc::invalid_state:
			return "invalid state";
		case bstream::errc::ibstreambuf_not_shareable:
			return "bstream buffer is not shareable";


		case bstream::errc::expected_nil:
			return "expected nil";
		case bstream::errc::invalid_header_for_shared_ptr:
			return "invalid header for shared ptr";
		case bstream::errc::invalid_header_for_unique_ptr:
			return "invalid header for unique ptr";
		case bstream::errc::dynamic_type_mismatch_in_saved_ptr:
			return "dynamic type mismatch in saved ptr";
		case bstream::errc::static_type_mismatch_in_saved_ptr:
			return "static type mismatch in saved ptr";
		case bstream::errc::expected_bool:
			return "expected bool";
		case bstream::errc::expected_float:
			return "expected float";
		case bstream::errc::expected_double:
			return "expected double";
		case bstream::errc::expected_string:
			return "expected string";
		case bstream::errc::expected_array:
			return "expected array";
		case bstream::errc::expected_map:
			return "expected map";
		case bstream::errc::unexpected_map_key:
			return "unexpected map key";
		case bstream::errc::unexpected_array_size:
			return "unexpected array size";
		case bstream::errc::unexpected_map_size:
			return "unexpected map size";
		case bstream::errc::expected_blob:
			return "expected blob";
		case bstream::errc::expected_extern:
			return "expected extern";
		case bstream::errc::invalid_header_for_error_code:
			return "invalid header for error code";
		case bstream::errc::string_length_exceeds_limit:
			return "string length exceeds limit";

		case bstream::errc::num_deser_type_error_int8:
			return "unexpected typecode in deserializer for 8-bit signed integer";
		case bstream::errc::num_deser_type_error_uint8:
			return "unexpected typecode in deserializer for 8-bit unsigned integer";
		case bstream::errc::num_deser_range_error_int16:
			return "value out of range in deserializer for 16-bit signed integer";
		case bstream::errc::num_deser_type_error_int16:
			return "unexpected typecode in deserializer for 16-bit signed integer";
		case bstream::errc::num_deser_type_error_uint16:
			return "unexpected typecode in deserializer for 16-bit unsigned integer";
		case bstream::errc::num_deser_range_error_int32:
			return "value out of range in deserializer for 32-bit signed integer";
		case bstream::errc::num_deser_type_error_int32:
			return "unexpected typecode in deserializer for 32-bit signed integer";
		case bstream::errc::num_deser_type_error_uint32:
			return "unexpected typecode in deserializer for 32-bit unsigned integer";
		case bstream::errc::num_deser_range_error_int64:
			return "value out of range in deserializer for 64-bit signed integer";
		case bstream::errc::num_deser_type_error_int64:
			return "unexpected typecode in deserializer for 64-bit signed integer";
		case bstream::errc::num_deser_type_error_uint64:
			return "unexpected typecode in deserializer for 64-bit unsigned integer";
		case bstream::errc::val_deser_type_error_string:
			return "unexpected typecode in deserializer for string";
		case bstream::errc::val_deser_type_error_string_view:
			return "unexpected typecode in deserializer for string_view";


		default:
			return "unknown bstream error";
    }
}

std::error_condition
bstream_category_impl::default_error_condition(int ev) const noexcept
{
	switch ( static_cast< bstream::errc > ( ev ) )
	{
		case bstream::errc::expected_nil:
		case bstream::errc::invalid_header_for_shared_ptr:
		case bstream::errc::invalid_header_for_unique_ptr:
		case bstream::errc::dynamic_type_mismatch_in_saved_ptr:
		case bstream::errc::static_type_mismatch_in_saved_ptr:
		case bstream::errc::expected_bool:
		case bstream::errc::expected_float:
		case bstream::errc::expected_double:
		case bstream::errc::expected_string:
		case bstream::errc::expected_array:
		case bstream::errc::expected_map:
		case bstream::errc::unexpected_map_key:
		case bstream::errc::unexpected_array_size:
		case bstream::errc::unexpected_map_size:
		case bstream::errc::expected_blob:
		case bstream::errc::expected_extern:
		case bstream::errc::invalid_header_for_error_code:
		case bstream::errc::string_length_exceeds_limit:
		case bstream::errc::num_deser_type_error_int8:
		case bstream::errc::num_deser_type_error_uint8:
		case bstream::errc::num_deser_range_error_int16:
		case bstream::errc::num_deser_type_error_int16:
		case bstream::errc::num_deser_type_error_uint16:
		case bstream::errc::num_deser_range_error_int32:
		case bstream::errc::num_deser_type_error_int32:
		case bstream::errc::num_deser_type_error_uint32:
		case bstream::errc::num_deser_range_error_int64:
		case bstream::errc::num_deser_type_error_int64:
		case bstream::errc::num_deser_type_error_uint64:
		case bstream::errc::val_deser_type_error_string:
		case bstream::errc::val_deser_type_error_string_view:
			return std::error_condition( static_cast< int >( bstream::errc::type_error ), *this );

		default:
			return std::error_condition(ev, *this);
	}
}

std::error_category const&
bstream::error_category() noexcept
{
    static bstream_category_impl instance;
    return instance;
}

std::error_condition 
bstream::make_error_condition( errc e )
{
    return std::error_condition( static_cast< int >( e ), bstream::error_category() );
}

std::error_code 
bstream::make_error_code( errc e )
{
    return std::error_code( static_cast< int >( e ), bstream::error_category() );
}


