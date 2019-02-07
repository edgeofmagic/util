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

#include <logicmill/laps/tls.h>

using namespace logicmill;
using namespace laps;

class tls_category_impl : public std::error_category
{
public:
	virtual const char*
	name() const noexcept override;

	virtual std::string
	message(int ev) const noexcept override;
};

const char*
tls_category_impl::name() const noexcept
{
	return "tls";
}

std::string
tls_category_impl::message(int ev) const noexcept
{
	switch (static_cast<tls::errc>(ev))
	{

		case tls::errc::ok:
			return "ok";
		case tls::errc::invalid_argument:
			return "invalid argument";
		case tls::errc::unsupported_argument:
			return "unsupported argument";
		case tls::errc::invalid_state:
			return "invalid_state";
		case tls::errc::lookup_error:
			return "lookup_error";
		case tls::errc::internal_error:
			return "internal_error";
		case tls::errc::invalid_key_length:
			return "invalid_key_length";
		case tls::errc::invalid_iv_length:
			return "invalid_iv_length";
		case tls::errc::prng_unseeded:
			return "prng_unseeded";
		case tls::errc::policy_violation:
			return "policy_violation";
		case tls::errc::algorithm_not_found:
			return "algorithm_not_found";
		case tls::errc::no_provider_found:
			return "no_provider_found";
		case tls::errc::provider_not_found:
			return "provider_not_found";
		case tls::errc::invalid_algorithm_name:
			return "invalid_algorithm_name";
		case tls::errc::encoding_error:
			return "encoding_error";
		case tls::errc::decoding_error:
			return "decoding_error";
		case tls::errc::integrity_failure:
			return "integrity_failure";
		case tls::errc::invalid_oid:
			return "invalid_oid";
		case tls::errc::stream_io_error:
			return "stream_io_error";
		case tls::errc::self_test_failure:
			return "self_test_failure";
		case tls::errc::not_implemented:
			return "not_implemented";
		case tls::errc::unknown:
			return "unknown";
		default:
			return "unknown tls error";
	}
}

std::error_category const&
tls::error_category() noexcept
{
	static tls_category_impl instance;
	return instance;
}

std::error_condition
tls::make_error_condition(tls::errc e)
{
	return std::error_condition(static_cast<int>(e), tls::error_category());
}

std::error_code
tls::make_error_code(tls::errc e)
{
	return std::error_code(static_cast<int>(e), tls::error_category());
}
