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
#include <logicmill/laps/channel_anchor.h>
#include <logicmill/laps/driver.h>
#include <logicmill/laps/framer.h>
#include <logicmill/util/buffer.h>

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
                chan->start_read(read_err, [&](channel::ptr const& cp, util::const_buffer&& buf, std::error_code err) {
                    CHECK(!err);
                    CHECK(buf.as_string() == "first test payload");

                    std::error_code write_err;
                    cp->write(
                            util::mutable_buffer{"reply to first payload"},
                            write_err,
                            [&](channel::ptr const& chan, util::mutable_buffer&& buf, std::error_code err) {
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

			stackp->top().start_read([&](std::deque<util::const_buffer>&& bufs) {
				CHECK(bufs.front().as_string() == "reply to first payload");
				driver_read_handler = true;
			});

			std::deque<util::mutable_buffer> bufs;
			bufs.emplace_back(util::mutable_buffer{"first test payload"});
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
	frame_top_catcher() : m_size{0}, m_flags{0}, m_event_count{0} {}

	void
	on(laps::shared_frame_event, laps::shared_frame&& frm)
	{

		m_size  = frm.size();
		m_flags = frm.flags();
		std::cout << "received event " << m_event_count << ": " << frm.size() << ", " << frm.bufs().size() << ": "
				  << frm.bufs()[0].to_string() << std::endl;
		m_bufs  = frm.release_bufs();
		m_size  = frm.size();
		m_flags = frm.flags();
		++m_event_count;
	}

	void
	on(laps::control_event, laps::control_state s)
	{}

	void
	on(laps::error_event, std::error_code err)
	{}

	laps::frame::frame_size_type
	size() const
	{
		return m_size;
	}

	laps::frame::flags_type
	flags() const
	{
		return m_flags;
	}

	laps::sbuf_sequence const&
	bufs() const
	{
		return m_bufs;
	}

	int
	event_count() const
	{
		return m_event_count;
	}

private:
	laps::frame::frame_size_type m_size;
	laps::frame::flags_type      m_flags;
	laps::sbuf_sequence          m_bufs;
	int                          m_event_count;
};
}    // namespace framer_test

TEST_CASE("logicmill::laps::framer [ smoke ] { framer existence }")
{
	laps::framer                   frmr;
	framer_test::frame_top_catcher ftop;
	frmr.get_top().stack(ftop.get_surface<laps::frame_duplex_bottom>());
	emitter<laps::const_data_event> driver;
	driver.get_source<laps::const_data_event>().bind(frmr.get_bottom()
															 .get_connector<laps::const_data_in_connector>()
															 .get_binding<sink<laps::const_data_event>>());

	std::string payload{"here is some buffer content, should be long enought to avoid short string optimization"};
	bstream::buffer::sink snk{8};
	util::mutable_buffer  b{payload};

	std::uint32_t blen{static_cast<std::uint32_t>(b.size())};
	std::uint32_t flags{0};
	snk.put_num(blen);
	snk.put_num(flags);
	util::mutable_buffer mbuf{snk.release_buffer()};
	std::cout << "mbuf size: " << mbuf.size() << std::endl;
	mbuf.dump(std::cout);
	std::deque<util::const_buffer> bufs;
	bufs.emplace_back(std::move(mbuf));
	bufs.emplace_back(util::const_buffer{std::move(b)});

	driver.emit<laps::const_data_event>(std::move(bufs));

	CHECK(ftop.size() == blen);
	CHECK(ftop.flags() == 0);
	CHECK(ftop.bufs().size() == 1);
	CHECK(ftop.bufs()[0].to_string() == payload);


	// driver.emit<laps::const_data_event>();
}


TEST_CASE("logicmill::laps::framer [ smoke ] { split header }")
{
	laps::framer                   frmr;
	framer_test::frame_top_catcher ftop;
	frmr.get_top().stack(ftop.get_surface<laps::frame_duplex_bottom>());
	emitter<laps::const_data_event> driver;
	driver.get_source<laps::const_data_event>().bind(frmr.get_bottom()
															 .get_connector<laps::const_data_in_connector>()
															 .get_binding<sink<laps::const_data_event>>());

	std::string payload{"here is some buffer content, should be long enought to avoid short string optimization"};
	bstream::buffer::sink hdr1{8};
	bstream::buffer::sink hdr2{8};
	util::mutable_buffer  b{payload};

	std::uint32_t blen{static_cast<std::uint32_t>(b.size())};
	std::uint32_t flags{0};
	hdr1.put_num(blen);
	hdr2.put_num(flags);
	util::mutable_buffer mbuf1{hdr1.release_buffer()};
	util::mutable_buffer mbuf2{hdr2.release_buffer()};

	std::deque<util::const_buffer> bufs;
	bufs.emplace_back(std::move(mbuf1));
	bufs.emplace_back(std::move(mbuf2));
	bufs.emplace_back(util::const_buffer{std::move(b)});

	driver.emit<laps::const_data_event>(std::move(bufs));

	CHECK(ftop.size() == blen);
	CHECK(ftop.flags() == 0);
	CHECK(ftop.bufs().size() == 1);
	CHECK(ftop.bufs()[0].to_string() == payload);
}

TEST_CASE("logicmill::laps::framer [ smoke ] { multiple frames in single buffer }")
{
	laps::framer                   frmr;
	framer_test::frame_top_catcher ftop;
	frmr.get_top().stack(ftop.get_surface<laps::frame_duplex_bottom>());
	emitter<laps::const_data_event> driver;
	driver.get_source<laps::const_data_event>().bind(frmr.get_bottom()
															 .get_connector<laps::const_data_in_connector>()
															 .get_binding<sink<laps::const_data_event>>());

	std::string payload1{"here is some buffer content, should be long enough to avoid short string optimization"};
	util::mutable_buffer b1{payload1};

	std::string          payload2{"here is some more buffer content, with a somewhat different length"};
	util::mutable_buffer b2{payload2};

	bstream::buffer::sink snk{1024};

	std::uint32_t blen{static_cast<std::uint32_t>(b1.size())};
	std::uint32_t flags{7};
	snk.put_num(blen);
	snk.put_num(flags);
	snk.putn(b1);

	blen  = static_cast<std::uint32_t>(b2.size());
	flags = 21;
	snk.put_num(blen);
	snk.put_num(flags);
	snk.putn(b2);

	std::deque<util::const_buffer> bufs;
	bufs.emplace_back(snk.release_buffer());

	driver.emit<laps::const_data_event>(std::move(bufs));

	CHECK(ftop.size() == payload2.size());
	CHECK(ftop.flags() == 21);
	CHECK(ftop.bufs()[0].to_string() == payload2);
	CHECK(ftop.event_count() == 2);
}
