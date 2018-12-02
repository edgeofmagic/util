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
#include <logicmill/async/framer.h>
#include <logicmill/bstream/buffer.h>

using namespace logicmill;
using namespace async;

TEST_CASE("logicmill::async::framer [ smoke ] { connect read write }")
{
	bool listener_connection_handler_did_execute{false};
	bool listener_read_handler_did_execute{false};
	bool listener_write_handler_did_execute{false};
	bool channel_connect_handler_did_execute{false};
	bool channel_read_handler_did_execute{false};
	bool channel_write_handler_did_execute{false};

	bstream::const_buffer message{"this is some frame content"};
	bstream::const_buffer reply{"this is some reply frame content"};

	static constexpr std::uint64_t test_info{0xDEADBEEFCAFEBABE};

	std::error_code     err;
	auto                lp = loop::create();
	async::ip::endpoint listen_ep{async::ip::address::v4_any(), 7001};
	auto                lstnr = lp->create_listener( async::options{listen_ep},
            err, [&](listener::ptr const& ls, channel::ptr const& chan, std::error_code const& err) {
                CHECK(!err);

				framer frm{chan};

				frm.start_read(read_err, [&](std::uint64_t info, std::uint64_t sequence, bstream::const_buffer&& buf, std::error_code const& err)
				{
					CHECK(!err);
					CHECK(info == test_info);
					CHECK(sequence == 27);
					CHECK(buf == message);

					std::error_code write_err;
					

				});

                std::error_code read_err;
                chan->start_read(
                        read_err, [&](channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code const& err) {
                            CHECK(!err);
                            CHECK(buf.as_string() == "first test payload");

                            std::error_code write_err;
                            cp->write(
                                    bstream::mutable_buffer{"reply to first payload"},
                                    write_err,
                                    [&](channel::ptr const&       chan,
                                        bstream::mutable_buffer&& buf,
                                        std::error_code const&    err) {
                                        CHECK(!err);
                                        CHECK(buf.as_string() == "reply to first payload");
                                        listener_write_handler_did_execute = true;
                                    });
                            CHECK(!write_err);
                            listener_read_handler_did_execute = true;
                        });
                CHECK(!read_err);
                listener_connection_handler_did_execute = true;
            });

	CHECK(!err);

	auto connect_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		std::error_code     err;
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};

		lp->connect_channel(async::options{connect_ep}, err, [&](channel::ptr const& chan, std::error_code const& err) {
			CHECK(!err);

			std::error_code rw_err;
			chan->start_read(
					rw_err, [&](channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code const& err) {
						CHECK(!err);
						CHECK(buf.as_string() == "reply to first payload");
						channel_read_handler_did_execute = true;
					});

			CHECK(!rw_err);

			bstream::mutable_buffer mbuf{"first test payload"};
			chan->write(
					std::move(mbuf),
					rw_err,
					[&](channel::ptr const& chan, bstream::mutable_buffer&& buf, std::error_code const& err) {
						CHECK(!err);
						CHECK(buf.as_string() == "first test payload");
						channel_write_handler_did_execute = true;
					});

			CHECK(!rw_err);
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
	CHECK(listener_connection_handler_did_execute);
	CHECK(listener_read_handler_did_execute);
	CHECK(listener_write_handler_did_execute);
	CHECK(channel_connect_handler_did_execute);
	CHECK(channel_read_handler_did_execute);
	CHECK(channel_write_handler_did_execute);
	CHECK(!err);
}
