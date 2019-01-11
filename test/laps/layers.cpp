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
#include <logicmill/laps/framer.h>

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

namespace framer_test
{
	class frame_top_catcher : public stackable<laps::frame_duplex_bottom, frame_top_catcher>
	{
	public:

		void on(laps::shared_frame_event, laps::frame_header header, laps::sbuf_sequence&& bufs)
		{
			m_header = header;
			m_bufs = std::move(bufs);
			std::cout << "received event: " << header.size << ", " << m_bufs.size() << ": " << m_bufs[0].to_string() << std::endl;
		}

		void on(laps::control_event, laps::control_state s) {}

		void on(laps::error_event, std::error_code err) {}

		laps::frame_header const& header() const
		{
			return m_header;
		}

		laps::sbuf_sequence const& bufs() const
		{
			return m_bufs;
		}

	private:
		laps::frame_header m_header;
		laps::sbuf_sequence m_bufs;
	};
}

TEST_CASE("logicmill::laps::framer [ smoke ] { framer existence }")
{
	laps::framer frmr;
	framer_test::frame_top_catcher ftop;
	frmr.get_top().stack(ftop.get_surface<laps::frame_duplex_bottom>());
	emitter<laps::const_data_event> driver;
	driver.get_source<laps::const_data_event>().bind(frmr.get_bottom()
						.get_connector<laps::const_data_in_connector>()
						.get_binding<sink<laps::const_data_event>>());
	
	bstream::memory::sink snk{8};
	bstream::mutable_buffer b{"here is some buffer content, should be long enought to avoid short string optimization"};

	std::uint32_t blen{static_cast<std::uint32_t>(b.size())};
	std::uint32_t flags{0};
	snk.put_num(blen, true);
	snk.put_num(flags, true);
	bstream::mutable_buffer mbuf{snk.release_buffer()};
	std::cout << "mbuf size: " << mbuf.size() << std::endl;
	mbuf.dump(std::cout);
	std::deque<bstream::const_buffer> bufs;
	bufs.emplace_back(std::move(mbuf));
	bufs.emplace_back(bstream::const_buffer{std::move(b)});

	driver.send<laps::const_data_event>(std::move(bufs));

	// driver.send<laps::const_data_event>();
}
