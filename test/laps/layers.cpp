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
#include <logicmill/laps/channel_anchor.h>
#include <logicmill/laps/driver.h>

using namespace logicmill;
using namespace async;
using namespace event_flow;

TEST_CASE("logicmill::laps::channel_anchor [ smoke ] { stackable connect read write }")
{
	bool acceptor_connection_handler_did_execute{false};
	bool acceptor_read_handler_did_execute{false};
	bool acceptor_write_handler_did_execute{false};
	bool channel_connect_handler_did_execute{false};
	bool driver_read_handler{false};
	bool channel_write_handler_did_execute{false};

	using stack_type = assembly<laps::channel_anchor, laps::driver>;
	std::shared_ptr<stack_type> stackp;

	std::error_code     err;
	auto                lp = loop::create();
	async::ip::endpoint listen_ep{async::ip::address::v4_any(), 7001};
	auto                lstnr = lp->create_acceptor(
            async::options{listen_ep},
            err,
            [&](acceptor::ptr const& ls, channel::ptr const& chan, std::error_code err) {
                CHECK(!err);

                std::error_code read_err;
                chan->start_read(
                        read_err, [&](channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code err) {
                            CHECK(!err);
                            CHECK(buf.as_string() == "first test payload");

                            std::error_code write_err;
                            cp->write(
                                    bstream::mutable_buffer{"reply to first payload"},
                                    write_err,
                                    [&](channel::ptr const& chan, bstream::mutable_buffer&& buf, std::error_code err) {
                                        CHECK(!err);
                                        CHECK(buf.as_string() == "reply to first payload");
                                        acceptor_write_handler_did_execute = true;
                                    });
                            CHECK(!write_err);
                            acceptor_read_handler_did_execute = true;
                        });
                CHECK(!read_err);
                acceptor_connection_handler_did_execute = true;
            });

	CHECK(!err);

	auto connect_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		std::error_code     err;
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};

		lp->connect_channel(async::options{connect_ep}, err, [&](channel::ptr const& chan, std::error_code err) {
			CHECK(!err);

			stackp = std::make_shared<stack_type>(laps::channel_anchor{chan}, laps::driver{});

			stackp->top().on_control([=](laps::control_state cstate) {
				std::cout << "control event in driver: " << ((cstate == laps::control_state::start) ? "start" : "stop")
						  << std::endl;
			});

			stackp->top().on_error(
					[=](std::error_code err) { std::cout << "error event in driver: " << err.message() << std::endl; });

			stackp->top().start_read([&](std::deque<bstream::const_buffer>&& bufs) {
				CHECK(bufs.front().as_string() == "reply to first payload");
				driver_read_handler = true;
			});

			std::deque<bstream::mutable_buffer> bufs;
			bufs.emplace_back(bstream::mutable_buffer{"first test payload"});
			stackp->top().write(std::move(bufs));

			CHECK(!err);
			channel_connect_handler_did_execute = true;
		});
		CHECK(!err);
	});
	CHECK(!err);

	auto shutdown_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		std::error_code err;
		lp->stop(err);
		CHECK(!err);
	});
	CHECK(!err);

	connect_timer->start(std::chrono::milliseconds{1000}, err);
	CHECK(!err);

	shutdown_timer->start(std::chrono::milliseconds{3000}, err);
	CHECK(!err);
	CHECK(connect_timer->is_pending());

	connect_timer.reset();

	lp->run(err);
	CHECK(!err);

	lp->close(err);
	CHECK(acceptor_connection_handler_did_execute);
	CHECK(acceptor_read_handler_did_execute);
	CHECK(acceptor_write_handler_did_execute);
	CHECK(channel_connect_handler_did_execute);
	CHECK(driver_read_handler);
	CHECK(!err);
}
