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

#ifndef LOGICMILL_ARMI_ADAPTER_ASYNC_CLIENT_H
#define LOGICMILL_ARMI_ADAPTER_ASYNC_CLIENT_H

#include <functional>
#include <logicmill/armi/adapters/async/channel_manager.h>
#include <logicmill/armi/adapters/async/traits.h>

#include <logicmill/armi/adapters/async/bstream_bridge.h>
#include <logicmill/armi/adapters/bstream/traits.h>

#include <logicmill/armi/client_context.h>
#include <logicmill/async/channel.h>
#include <logicmill/async/loop.h>
#include <logicmill/util/promise.h>

namespace logicmill
{
namespace async
{

template<class T>
class adapter;

template<template<class...> class RemoteContext, class SerializationTraits, class TransportTraits>
class adapter<RemoteContext<SerializationTraits, TransportTraits>>
{
public:
	using serialization_traits = SerializationTraits;
	using transport_traits     = TransportTraits;

	using remote_type = RemoteContext<serialization_traits, transport_traits>;

	using bridge_type = armi::adapters::bridge<serialization_traits, transport_traits>;

	using client_context_type      = typename remote_type::client_context_type;
	using client_context_base_type = typename remote_type::client_context_base_type;
	using target_type              = typename remote_type::target_type;

	class client_adapter_base : public remote_type::client_context_type, public channel_manager
	{
	public:
		using channel_error_handler = std::function<void(channel_id_type channel, std::error_code err)>;
		using channel_id_type       = armi::channel_id_type;

		/* public interface for client_adapter */

		client_adapter_base(loop::ptr lp) : m_loop{lp}, m_is_closing{false} {}

		virtual ~client_adapter_base() {}

		void
		close(channel_id_type channel_id, std::error_code err)
		{
			if (is_valid_channel(channel_id))
				m_loop->dispatch([=]() { really_close(channel_id, err); });
		}

		void
		close(std::error_code err)
		{
			if (!m_is_closing)
			{
				m_is_closing = true;
				really_close(make_error_code(armi::errc::context_closed));
			}
		}

		virtual bool
		is_valid_channel(channel_id_type channel_id) override
		{
			bool result{false};
			auto chan = get_channel(channel_id);
			if (chan && !chan->is_closing())
				result = true;
			return result;
		}

		virtual void
		close(channel_id_type channel_id) override
		{
			close(channel_id, make_error_code(armi::errc::channel_closed));
		}

		virtual void
		send_request(
				channel_id_type                             channel_id,
				armi::request_id_type                       request_id,
				std::chrono::milliseconds                   timeout,
				typename bridge_type::serializer_param_type req) override
		{
			std::error_code err;

			auto chan = get_channel(channel_id);
			if (!chan)
			{
				err = make_error_code(armi::errc::invalid_channel_id);
				goto exit;
			}

			if (timeout.count() > 0)
			{
				m_loop->schedule(timeout, err, [=]() {
					client_context_base_type::cancel_request(request_id, make_error_code(std::errc::timed_out));
				});
				if (err)
					goto exit;
			}

			bridge_type::mutable_buffer_from_serializer(
					req, [&](util::mutable_buffer&& buf) { chan->write(std::move(buf), err); });
			if (err)
				goto exit;

			// chan->write(std::move(req), err, [=](channel::ptr const& chan, util::mutable_buffer&& buf, std::error_code err)
			// {
			// 	std::cout << "write complete";
			// 	if (err)
			// 	{
			// 		std::cout << ", err: " << err.message();
			// 	}
			// 	std::cout << std::endl;
			// });

		exit:
			if (err)
			{
				if (m_loop->is_alive())
					m_loop->dispatch([=]() { client_context_base_type::cancel_request(request_id, err); });
				else
					client_context_base_type::cancel_request(request_id, make_error_code(armi::errc::no_event_loop));
			}
			return;
		}

		void
		on_channel_read_error(channel_error_handler handler)
		{
			m_on_channel_read_error = std::move(handler);
		}

		void
		close()
		{
			close(make_error_code(armi::errc::context_closed));
		}

	protected:
		void
		really_close(channel_id_type channel_id, std::error_code err)
		{
			auto chan = get_channel(channel_id);
			if (chan)
			{
				remove(channel_id);
				if (m_loop->is_alive())
				{
					chan->close();
					m_loop->dispatch([=]() { client_context_base_type::cancel_channel_requests(channel_id, err); });
				}
				else
					client_context_base_type::cancel_channel_requests(
							channel_id, make_error_code(armi::errc::no_event_loop));
			}
		}

		void
		really_close(std::error_code err)
		{
			if (m_loop->is_alive())
			{
				visit_and_remove_all([](channel_id_type channel_id, async::channel::ptr const& chan) {
					if (chan)
						chan->close();
				});
				m_loop->dispatch([=]() { client_context_base_type::cancel_all_requests(err); });
			}
			else
			{
				clear_map();
				client_context_base_type::cancel_all_requests(make_error_code(armi::errc::no_event_loop));
			}
		}

		async::loop::ptr      m_loop;
		channel_id_type       m_next_channel_id;
		channel_error_handler m_on_channel_read_error;
		bool                  m_is_closing;
	};

	class client_adapter : public client_adapter_base
	{
	public:
		using client_context_type      = typename remote_type::client_context_type;
		using client_context_base_type = typename remote_type::client_context_base_type;
		using channel_id_type          = armi::channel_id_type;
		using channel_error_handler    = std::function<void(channel_id_type channel_id, std::error_code err)>;
		using client_connect_handler
				= std::function<void(typename client_context_type::target_reference, std::error_code)>;

		client_adapter(loop::ptr lp) : client_adapter_base{lp} {}

		virtual ~client_adapter()
		{
			client_adapter_base::close();
		}

		util::promise<typename client_context_type::target_reference>
		connect(async::options const& opts)
		{
			async::options options_override{opts};
			options_override.framing(true);
			util::promise<typename client_context_type::target_reference> p;
			std::error_code                                               err;
			client_adapter_base::m_loop->connect_channel(
					options_override, err, [=](async::channel::ptr chan, std::error_code err) mutable {
						if (err)
							p.reject(err);
						else
						{
							auto id = channel_manager::new_channel(chan);
							chan->start_read(
									err,
									[=](async::channel::ptr const& chan,
										util::const_buffer&&       buf,
										std::error_code            err) {
										if (err)
										{
											if (client_adapter_base::m_on_channel_read_error)
												client_adapter_base::m_on_channel_read_error(id, err);
											else
												client_adapter_base::close(id, err);
										}
										else
										{
											bridge_type::deserializer_from_const_buffer(
													std::move(buf),
													[&](typename bridge_type::deserializer_param_type reply) {
														client_context_base_type::handle_reply(reply);
													});
										}
									});
							if (err)
								p.reject(err);
							else
								p.resolve(client_context_type::create_target_reference(id));
						}
					});
			if (err)
				p.reject(err);
			return p;
		}

		void
		connect(async::options const& opts, std::error_code& err, client_connect_handler handler)
		{
			err.clear();
			async::options options_override{opts};
			options_override.framing(true);
			client_adapter_base::m_loop->connect_channel(
					options_override,
					err,
					[=, handler{std::move(handler)}](async::channel::ptr chan, std::error_code err) {
						if (err)
							handler(client_context_type::create_target_reference(0), err);
						else
						{
							auto id = channel_manager::new_channel(chan);
							chan->start_read(
									err,
									[=](async::channel::ptr const& chan,
										util::const_buffer&&       buf,
										std::error_code            err) {
										if (err)
										{
											if (client_adapter_base::m_on_channel_read_error)
												client_adapter_base::m_on_channel_read_error(id, err);
											else
												client_adapter_base::close(id, err);
										}
										else
										{
											// TODO: fix the ham-fisted insertion of stream_context_type
											bridge_type::deserializer_from_const_buffer(
													std::move(buf),
													[&](typename bridge_type::deserializer_param_type reply) {
														client_context_base_type::handle_reply(reply);
													});
											// bstream::imbstream reply{std::move(buf), stream_context_type::get()};
											// client_context_base_type::handle_reply(reply);
										}
									});
							handler(client_context_type::create_target_reference(id), std::error_code{});
						}
					});
		}
	};


	class server_adapter_base : public remote_type::server_context_type, public channel_manager
	{
	public:
		using channel_id_type = armi::channel_id_type;

		using channel_error_handler = std::function<void(channel_id_type, std::error_code)>;
		using accept_error_handler  = std::function<void(std::error_code err)>;
		using server_close_handler  = std::function<void()>;
		using channel_close_handler = std::function<void(channel_id_type)>;
		using connection_handler    = std::function<void(channel_id_type)>;

		using server_context_type = typename remote_type::server_context_type;

	protected:
		async::loop::ptr      m_loop;
		async::acceptor::ptr  m_acceptor;
		bool                  m_is_server_closing;
		channel_close_handler m_on_channel_close;
		channel_error_handler m_on_channel_error;
		accept_error_handler  m_on_accept_error;
		server_close_handler  m_on_server_close;
		connection_handler    m_on_channel_connect;

	public:
		server_adapter_base(async::loop::ptr lp) : m_loop{lp}, m_is_server_closing{false} {}

		virtual ~server_adapter_base() {}

		server_adapter_base&
		on_channel_close(channel_close_handler handler)
		{
			m_on_channel_close = std::move(handler);
			return *this;
		}

		server_adapter_base&
		on_channel_error(channel_error_handler handler)
		{
			m_on_channel_error = std::move(handler);
			return *this;
		}

		server_adapter_base&
		on_channel_connect(connection_handler handler)
		{
			m_on_channel_connect = std::move(handler);
			return *this;
		}

		server_adapter_base&
		on_server_close(server_close_handler handler)
		{
			m_on_server_close = std::move(handler);
			return *this;
		}

		server_adapter_base&
		on_accept_error(accept_error_handler handler)
		{
			m_on_accept_error = std::move(handler);
			return *this;
		}

		void
		close()
		{
			really_close();
		}


		virtual bool
		is_valid_channel(channel_id_type channel_id) override
		{
			bool result{false};
			auto chan = get_channel(channel_id);
			if (chan && !chan->is_closing())
				result = true;
			return result;
		}

		virtual void
		send_reply(channel_id_type channel_id, typename bridge_type::serializer_param_type rep) override
		{
			auto chan = get_channel(channel_id);
			if (!chan)
				channel_error(channel_id, make_error_code(armi::errc::invalid_channel_id));
			else
			{
				std::error_code err;

				bridge_type::mutable_buffer_from_serializer(
						rep, [&](util::mutable_buffer&& buf) { chan->write(std::move(buf), err); });

				// chan->write(std::move(req->release_mutable_buffer()), err);
				if (err)
					channel_error(channel_id, err);
			}
		}

		virtual void
		close(channel_id_type channel_id) override
		{
			really_close(channel_id);
		}

	protected:
		void
		accept_error(std::error_code err)
		{
			if (m_on_accept_error)
				m_on_accept_error(err);
			else
				default_accept_error_handler(err);
		}

		void
		channel_error(channel_id_type channel_id, std::error_code err)
		{
			if (m_on_channel_error)
				m_on_channel_error(channel_id, err);
			else
				default_channel_error_handler(channel_id, err);
		}
		void
		cleanup_base()
		{
			m_on_server_close = nullptr;
			// m_on_request         = nullptr;
			m_on_channel_close   = nullptr;
			m_on_channel_error   = nullptr;
			m_on_accept_error    = nullptr;
			m_on_channel_connect = nullptr;
		}

		void
		default_channel_error_handler(channel_id_type channel_id, std::error_code err)
		{
			close(channel_id);
		}

		void
		default_accept_error_handler(std::error_code err)
		{
			close();
		}

		virtual void
		cleanup()
				= 0;

		void
		really_close(channel_id_type channel_id)
		{
			visit_and_remove(channel_id, [](channel_id_type channel_id, async::channel::ptr chan) {
				if (chan)
					chan->close();
			});
			if (m_on_channel_close)
				m_on_channel_close(channel_id);
		}

		void
		really_close()
		{
			if (!m_is_server_closing)
			{
				m_is_server_closing = true;

				if (m_acceptor)
				{
					m_acceptor->close();
					if (m_on_server_close)
						m_on_server_close();
				}
				std::vector<channel_id_type> closed_channels;
				closed_channels.reserve(active_channel_count());
				visit_and_remove_all([&closed_channels](channel_id_type channel_id, async::channel::ptr chan) {
					closed_channels.push_back(channel_id);
					if (chan)
						chan->close();
				});
				if (m_on_channel_close)
				{
					for (auto channel_id : closed_channels)
						m_on_channel_close(channel_id);
				}
				cleanup();
			}
		}
	};

	class server_adapter : public server_adapter_base
	{
	public:
		using channel_id_type     = armi::channel_id_type;
		using server_context_type = typename remote_type::server_context_type;
		using target_ptr_type     = std::shared_ptr<typename server_context_type::target_type>;
		using request_handler     = std::function<target_ptr_type(channel_id_type)>;

	private:
		request_handler m_on_request;

	public:
		server_adapter(async::loop::ptr lp) : server_adapter_base{lp} {}

		~server_adapter()
		{
			server_adapter_base::close();
		}

		virtual void
		cleanup() override
		{
			m_on_request = nullptr;
			server_adapter_base::cleanup_base();
		}

		server_adapter&
		on_request(request_handler handler)
		{
			m_on_request = std::move(handler);
			return *this;
		}

		void
		bind(async::options const& opts, std::error_code& err)
		{
			err.clear();
			async::options opts_override{opts};
			opts_override.framing(true);
			server_adapter_base::m_acceptor = server_adapter_base::m_loop->create_acceptor(
					opts_override,
					err,
					[=](async::acceptor::ptr const& sp, async::channel::ptr const& chan, std::error_code err) {
						if (err)
							server_adapter_base::accept_error(err);
						else
						{
							auto channel_id = channel_manager::new_channel(chan);
							if (server_adapter_base::m_on_channel_connect)
								server_adapter_base::m_on_channel_connect(channel_id);
							chan->start_read(
									err,
									[=](async::channel::ptr const& chan,
										util::const_buffer&&       buf,
										std::error_code            err) {
										if (err)
											server_adapter_base::channel_error(channel_id, err);
										else
										{
											if (!m_on_request)
												server_adapter_base::accept_error(
														make_error_code(armi::errc::no_target_provided));
											else
											{
												// TODO: check to see if channel_id is still in channel_map first
												bstream::imbstream is{std::move(buf),
																	  serialization_traits::stream_context_type::get()};
												server_context_type::handle_request(
														channel_id, is, m_on_request(channel_id));
											}
										}
									});
						}
					});
		}
	};
};

}    // namespace async
}    // namespace logicmill
#endif    // LOGICMILL_ARMI_ADAPTER_ASYNC_CLIENT_H