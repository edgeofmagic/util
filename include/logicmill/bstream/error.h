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

#ifndef LOGICMILL_BSTREAM_ERROR_H
#define LOGICMILL_BSTREAM_ERROR_H

#include <system_error>

namespace logicmill
{
namespace bstream
{

enum class errc
{
	ok = 0,
	read_past_end_of_stream,
	type_error,
	member_count_error,
	context_mismatch,
	invalid_err_category,
	invalid_ptr_downcast,
	abstract_non_poly_class,
	invalid_operation,
	invalid_state,
	ibstreambuf_not_shareable,

	// specific cases of type_error:

	expected_nil,
	invalid_header_for_shared_ptr,
	invalid_header_for_unique_ptr,
	dynamic_type_mismatch_in_saved_ptr,
	static_type_mismatch_in_saved_ptr,
	expected_bool,
	expected_float,
	expected_double,
	expected_string,
	expected_array,
	expected_map,
	unexpected_map_key,
	unexpected_array_size,
	unexpected_map_size,
	expected_blob,
	expected_extern,
	invalid_header_for_error_code,
	string_length_exceeds_limit,

	num_deser_type_error_int8,
	num_deser_type_error_uint8,
	num_deser_range_error_int16,
	num_deser_type_error_int16,
	num_deser_type_error_uint16,
	num_deser_range_error_int32,
	num_deser_type_error_int32,
	num_deser_type_error_uint32,
	num_deser_range_error_int64,
	num_deser_type_error_int64,
	num_deser_type_error_uint64,
	val_deser_type_error_string,
	val_deser_type_error_string_view,

};

std::error_category const&
error_category() noexcept;

std::error_condition
make_error_condition(errc e);

std::error_code
make_error_code(errc e);

}    // namespace bstream
}    // namespace logicmill

namespace std
{
template<>
struct is_error_condition_enum<logicmill::bstream::errc> : public true_type
{};
}    // namespace std

#endif /* LOGICMILL_BSTREAM_ERROR_H */
