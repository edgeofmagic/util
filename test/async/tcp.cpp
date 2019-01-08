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
#include <logicmill/async/tcp.h>

using namespace logicmill;
using namespace async;

TEST_CASE("logicmill::async::tcp_acceptor [ smoke ] { basic functionality }")
{
	bool acceptor_handler_did_execute{false};
	bool channel_connect_handler_did_execute{false};

	async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};

	std::error_code err;
	auto            lp = loop::create();

	auto loop_exit_timer = lp->create_timer(err, [&](async::timer::ptr tp) {
		tp->loop()->stop(err);
		CHECK(!err);
	});

	loop_exit_timer->start(std::chrono::milliseconds{2000}, err);
	CHECK(!err);

	async::ip::endpoint listen_ep{async::ip::address::v4_any(), 7001};
	auto                lstnr = lp->create_acceptor(
            async::options{listen_ep},
            err,
            [&](acceptor::ptr const& ls, channel::ptr const& chan, std::error_code err) {
                CHECK(!err);
                chan->close();
                ls->close();
                acceptor_handler_did_execute = true;
            });
	if (err) 
	{
		std::cout << "err on create_acceptor is " << err.message() << std::endl;
	}
	CHECK(!err);

	auto connect_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		std::error_code err;
		std::cout << "connect_ep: " << connect_ep.to_string() << std::endl;
		lp->connect_channel(async::options{connect_ep}, err, [&](channel::ptr const& chan, std::error_code err) {
			CHECK(!err);
			auto peer_ep = chan->get_peer_endpoint(err);
			CHECK(!err);
			CHECK(peer_ep == connect_ep);
			std::cout << "endpoint from get_peer_endpoint: " << peer_ep.to_string() << std::endl;
			auto chan_ep = chan->get_endpoint(err);
			std::cout << "endpoint from get_endpoint: " << chan_ep.to_string() << std::endl;
			chan->close();
			channel_connect_handler_did_execute = true;
		});
		CHECK(!err);
	});
	CHECK(!err);

	connect_timer->start(std::chrono::milliseconds{1000}, err);
	CHECK(!err);
	CHECK(connect_timer->is_pending());

	connect_timer.reset();

	lp->run(err);
	CHECK(acceptor_handler_did_execute);
	CHECK(channel_connect_handler_did_execute);
	CHECK(!err);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::async::tcp_acceptor [ smoke ] { error on bad address }")
{
	bool acceptor_handler_did_execute{false};
	bool channel_connect_handler_did_execute{false};
	bool channel_close_handler_did_execute{false};

	std::error_code err;
	auto            lp = loop::create();


	auto loop_exit_timer = lp->create_timer(err, [&](async::timer::ptr tp) {
		tp->loop()->stop(err);
		CHECK(!err);
	});

	loop_exit_timer->start(std::chrono::milliseconds{2000}, err);
	CHECK(!err);

	async::ip::endpoint listen_ep{async::ip::address{"11.42.53.5"}, 7001};
	auto                lstnr = lp->create_acceptor(
            async::options{listen_ep},
            err,
            [&](acceptor::ptr const& ls, channel::ptr const& chan, std::error_code err) {
                CHECK(!err);
                chan->close();
                ls->close();
                acceptor_handler_did_execute = true;
            });

	REQUIRE(err);
	REQUIRE(err == std::errc::address_not_available);

	if (err)
	{
		lstnr->close();
	}

	auto connect_timer = lp->create_timer(err, [&](async::timer::ptr timer_ptr) {
		std::error_code     err;
		async::ip::endpoint connect_ep{async::ip::address::v4_loopback(), 7001};

		lp->connect_channel(async::options{connect_ep}, err, [&](channel::ptr const& chan, std::error_code err) {
			CHECK(err);
			CHECK(err == std::errc::connection_refused);
			CHECK(chan);
			chan->close([&](channel::ptr const& closed_chan) { channel_close_handler_did_execute = true; });
			channel_connect_handler_did_execute = true;
		});
		CHECK(!err);
	});
	CHECK(!err);

	connect_timer->start(std::chrono::milliseconds{1000}, err);
	CHECK(!err);
	CHECK(connect_timer->is_pending());

	connect_timer.reset();

	lp->run(err);
	CHECK(!acceptor_handler_did_execute);
	CHECK(channel_connect_handler_did_execute);
	CHECK(channel_close_handler_did_execute);
	CHECK(!err);

	lp->close(err);
	CHECK(!err);
}

TEST_CASE("logicmill::async::tcp_acceptor [ smoke ] { connect read write }")
{
	bool acceptor_connection_handler_did_execute{false};
	bool acceptor_read_handler_did_execute{false};
	bool acceptor_write_handler_did_execute{false};
	bool channel_connect_handler_did_execute{false};
	bool channel_read_handler_did_execute{false};
	bool channel_write_handler_did_execute{false};

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

			chan->start_read(err, [&](channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code err) {
				CHECK(!err);
				CHECK(buf.as_string() == "reply to first payload");
				channel_read_handler_did_execute = true;
			});

			CHECK(!err);

			bstream::mutable_buffer mbuf{"first test payload"};
			chan->write(
					std::move(mbuf),
					err,
					[&](channel::ptr const& chan, bstream::mutable_buffer&& buf, std::error_code err) {
						CHECK(!err);
						CHECK(buf.as_string() == "first test payload");
						channel_write_handler_did_execute = true;
					});

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
	CHECK(channel_read_handler_did_execute);
	CHECK(channel_write_handler_did_execute);
	CHECK(!err);
}

TEST_CASE("logicmill::async::tcp_acceptor [ smoke ] { connect read write multi-buffer }")
{
	bool acceptor_connection_handler_did_execute{false};
	bool acceptor_read_handler_did_execute{false};
	bool acceptor_write_handler_did_execute{false};
	bool channel_connect_handler_did_execute{false};
	bool channel_read_handler_did_execute{false};
	bool channel_write_handler_did_execute{false};

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
                            auto sv = buf.as_string();
                            CHECK(sv == "payload part 1, payload part 2");

                            std::deque<bstream::mutable_buffer> reply;
                            reply.emplace_back(bstream::mutable_buffer{"reply part 1, "});
                            reply.emplace_back(bstream::mutable_buffer{"reply part 2"});
                            std::error_code write_err;
                            cp->write(
                                    std::move(reply),
                                    write_err,
                                    [&](channel::ptr const&                   chan,
                                        std::deque<bstream::mutable_buffer>&& bufs,
                                        std::error_code                       err) {
                                        CHECK(!err);
                                        CHECK(bufs[0].as_string() == "reply part 1, ");
                                        CHECK(bufs[1].as_string() == "reply part 2");
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

			std::error_code rwerr;
			chan->start_read(rwerr, [&](channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code err) {
				CHECK(!err);
				CHECK(buf.as_string() == "reply part 1, reply part 2");
				channel_read_handler_did_execute = true;
			});
			CHECK(!rwerr);

			std::deque<bstream::mutable_buffer> mbufs;
			mbufs.emplace_back(bstream::mutable_buffer{"payload part 1, "});
			mbufs.emplace_back(bstream::mutable_buffer{"payload part 2"});
			chan->write(
					std::move(mbufs),
					rwerr,
					[&](channel::ptr const& chan, std::deque<bstream::mutable_buffer>&& bufs, std::error_code err) {
						CHECK(!err);
						CHECK(bufs[0].as_string() == "payload part 1, ");
						CHECK(bufs[1].as_string() == "payload part 2");
						channel_write_handler_did_execute = true;
					});
			CHECK(!rwerr);
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
	CHECK(!err);

	CHECK(acceptor_connection_handler_did_execute);
	CHECK(acceptor_read_handler_did_execute);
	CHECK(acceptor_write_handler_did_execute);
	CHECK(channel_connect_handler_did_execute);
	CHECK(channel_read_handler_did_execute);
	CHECK(channel_write_handler_did_execute);
}

TEST_CASE("logicmill::async::tcp_framing_acceptor [ smoke ] { framing connect read write }")
{
	bool acceptor_connection_handler_did_execute{false};
	bool acceptor_read_handler_did_execute{false};
	bool acceptor_write_handler_did_execute{false};
	bool channel_connect_handler_did_execute{false};
	bool channel_read_handler_did_execute{false};
	bool channel_write_handler_did_execute{false};

	std::error_code     err;
	auto                lp = loop::create();
	async::ip::endpoint listen_ep{async::ip::address::v4_any(), 7001};
	// async::options listen_opt{listen_ep}.framing(true);
	auto lstnr = lp->create_acceptor(
			async::options::create(listen_ep).framing(true),
			err,
			[&](acceptor::ptr const& ls, channel::ptr const& chan, std::error_code err) {
				CHECK(!err);

				std::error_code read_err;
				chan->start_read(
						read_err, [&](channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code err) {
							CHECK(!err);
							CHECK(buf.as_string() == "first test payload, padded to contain more than 32 characters");

							std::error_code write_err;
							cp->write(
									bstream::mutable_buffer{
											"reply to first payload, also padded to contain more than 32 characters"},
									write_err,
									[&](channel::ptr const& chan, bstream::mutable_buffer&& buf, std::error_code err) {
										CHECK(!err);
										CHECK(buf.as_string()
											  == "reply to first payload, also padded to contain more than 32 "
												 "characters");
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
		async::options      connect_options{connect_ep};

		lp->connect_channel(connect_options.framing(true), err, [&](channel::ptr const& chan, std::error_code err) {
			CHECK(!err);

			std::error_code rw_err;
			chan->start_read(rw_err, [&](channel::ptr const& cp, bstream::const_buffer&& buf, std::error_code err) {
				CHECK(!err);
				CHECK(buf.as_string() == "reply to first payload, also padded to contain more than 32 characters");
				channel_read_handler_did_execute = true;
			});

			CHECK(!rw_err);

			bstream::mutable_buffer mbuf{"first test payload, padded to contain more than 32 characters"};
			chan->write(
					std::move(mbuf),
					rw_err,
					[&](channel::ptr const& chan, bstream::mutable_buffer&& buf, std::error_code err) {
						CHECK(!err);
						CHECK(buf.as_string() == "first test payload, padded to contain more than 32 characters");
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
	CHECK(acceptor_connection_handler_did_execute);
	CHECK(acceptor_read_handler_did_execute);
	CHECK(acceptor_write_handler_did_execute);
	CHECK(channel_connect_handler_did_execute);
	CHECK(channel_read_handler_did_execute);
	CHECK(channel_write_handler_did_execute);
	CHECK(!err);
}
