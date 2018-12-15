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

#include <doctest.h>
#include <iostream>
#include <logicmill/async/loop.h>
#include <logicmill/bstream/buffer.h>

#define END_LOOP(loop_ptr, delay_ms)                                                                                   \
	{                                                                                                                  \
		std::error_code _err;                                                                                          \
		auto            loop_exit_timer = loop_ptr->create_timer(_err, [](async::timer::ptr tp) {                      \
            std::error_code err;                                                                            \
            tp->loop()->stop(err);                                                                          \
            CHECK(!err);                                                                                    \
        });                                                                                                 \
		CHECK(!_err);                                                                                                  \
		loop_exit_timer->start(std::chrono::milliseconds{delay_ms}, _err);                                             \
		CHECK(!_err);                                                                                                  \
	}


#define DELAYED_ACTION_BEGIN(_loop_ptr)                                                                                \
{                                                                                                                      \
	std::error_code _err;                                                                                              \
	auto _action_timer = _loop_ptr->create_timer(_err, [&](async::timer::ptr tp)                                       \

#define DELAYED_ACTION_END(_delay_ms)                                                                                  \
	);                                                                                                                 \
	CHECK(!_err);                                                                                                      \
	_action_timer->start(std::chrono::milliseconds{_delay_ms}, _err);                                                  \
	CHECK(!_err);                                                                                                      \
}

using namespace logicmill;
using namespace async;

TEST_CASE("logicmill::async::udp [ smoke ] { basic functionality }")
{
	bool listener_handler_did_execute{false};
	bool send_timer_did_execute{false};

	std::error_code     err;
	auto                lp = loop::create();

	END_LOOP(lp, 2000);

	auto recvr = lp->create_transceiver(async::options{async::ip::endpoint{async::ip::address::v4_any(),7002}}, err);
	CHECK(!err);

	recvr->start_receive(err, [&](async::transceiver::ptr transp, bstream::const_buffer&& buf, async::ip::endpoint const& ep, std::error_code err)
	{
		std::cout << "in receiver handler, buffer: " << buf.to_string() << std::endl;
		std::cout << "received from " << ep.to_string() << std::endl;
	});

	async::transceiver::ptr trans = lp->create_transceiver(async::options{async::ip::endpoint{async::ip::address::v4_any(),0}}, err);
	CHECK(!err);

	bstream::mutable_buffer msg("hello there");
	CHECK(!err);

	DELAYED_ACTION_BEGIN(lp)
	{
		trans->send(std::move(msg), async::ip::endpoint{async::ip::address::v4_loopback(),7002}, err);
		CHECK(!err);
		std::cout << "sending buffer on UDP socket" << std::endl;
		send_timer_did_execute = true;
	}
	DELAYED_ACTION_END(1000);

	lp->run(err);
	CHECK(!err);

	CHECK(send_timer_did_execute);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::async::udp [ smoke ] { error on redundant receive }")
{
	bool first_receive_handler_did_execute{false};
	bool second_receive_handler_did_execute{false};
	bool send_timer_did_execute{false};

	std::error_code     err;
	auto                lp = loop::create();

	END_LOOP(lp, 2000);

	auto recvr = lp->create_transceiver(
			async::options{async::ip::endpoint{async::ip::address::v4_any(), 7002}},
			err,
			[&](async::transceiver::ptr    transp,
				bstream::const_buffer&&    buf,
				async::ip::endpoint const& ep,
				std::error_code            err) {
				first_receive_handler_did_execute = true;
				CHECK(buf.to_string() == "hello there");
				std::cout << "in first receiver handler, buffer: " << buf.to_string() << std::endl;
				std::cout << "received from " << ep.to_string() << std::endl;
			});
	CHECK(!err);

	recvr->start_receive(
			err,
			[&](async::transceiver::ptr    transp,
				bstream::const_buffer&&    buf,
				async::ip::endpoint const& ep,
				std::error_code            err) {
				second_receive_handler_did_execute = true;
			});
	CHECK(err);
	CHECK(err == std::errc::connection_already_in_progress);

	async::transceiver::ptr trans = lp->create_transceiver(async::options{async::ip::endpoint{async::ip::address::v4_any(),0}}, err);
	CHECK(!err);

	bstream::mutable_buffer msg("hello there");
	CHECK(!err);

	DELAYED_ACTION_BEGIN(lp)
	{
		trans->send(std::move(msg), async::ip::endpoint{async::ip::address::v4_loopback(),7002}, err);
		CHECK(!err);
		send_timer_did_execute = true;
	}
	DELAYED_ACTION_END(1000);

	lp->run(err);
	CHECK(!err);

	CHECK(send_timer_did_execute);
	CHECK(first_receive_handler_did_execute);
	CHECK(!second_receive_handler_did_execute);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::async::udp [ smoke ] { max datagram size }")
{
	bool receive_handler_did_execute{false};
	bool send_timer_did_execute{false};

	bstream::mutable_buffer big{async::transceiver::payload_size_limit};
	big.fill(static_cast<bstream::byte_type>('Z'));
	big.size(async::transceiver::payload_size_limit);

	std::error_code     err;
	auto                lp = loop::create();

	END_LOOP(lp, 2000);

	auto recvr = lp->create_transceiver(
			async::options{async::ip::endpoint{async::ip::address::v4_any(), 7002}},
			err,
			[&](async::transceiver::ptr    transp,
				bstream::const_buffer&&    buf,
				async::ip::endpoint const& ep,
				std::error_code            err) {
				CHECK(!err);
				receive_handler_did_execute = true;
				CHECK(buf.size() == async::transceiver::payload_size_limit);
				if (buf.size() == async::transceiver::payload_size_limit)
				{
					bool same = true;
					for (std::size_t i = 0; i < async::transceiver::payload_size_limit; ++i)
					{
						if (buf.data()[i] != static_cast<bstream::byte_type>('Z'))
						{
							same = false;
							break;
						}
					}
					CHECK(same);
				}
				std::cout << "in first receiver handler, buffer size: " << buf.size() << std::endl;
				std::cout << "received from " << ep.to_string() << std::endl;
			});
	CHECK(!err);

	async::transceiver::ptr trans = lp->create_transceiver(async::options{async::ip::endpoint{async::ip::address::v4_any(),0}}, err);
	CHECK(!err);

	DELAYED_ACTION_BEGIN(lp)
	{
		trans->send(std::move(big), async::ip::endpoint{async::ip::address::v4_loopback(),7002}, err,
		[&](transceiver::ptr const& trans, bstream::mutable_buffer&& buf, async::ip::endpoint const& ep, std::error_code err)
		{
			CHECK(!err);
			std::cout << "error in send handler, " << err.message() << std::endl;
			std::cout << "send handler called, buf size is " << buf.size() << std::endl;
		});
		CHECK(!err);
		send_timer_did_execute = true;
	}
	DELAYED_ACTION_END(1000);

	lp->run(err);
	CHECK(!err);

	CHECK(send_timer_did_execute);
	CHECK(receive_handler_did_execute);

	lp->close(err);
	CHECK(!err);
}