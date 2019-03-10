#include "tcp_uv.h"
#include "loop_uv.h"

using namespace logicmill;

logicmill::util::shared_ptr<tcp_channel_uv>
connect_request_uv::get_channel_shared_ptr(uv_connect_t* req)
{
	return logicmill::util::dynamic_pointer_cast<tcp_channel_uv>(tcp_base_uv::get_base_shared_ptr(req->handle));
}

void
connect_request_uv::on_connect(uv_connect_t* req, int status)
{
	auto            request_ptr = reinterpret_cast<connect_request_uv*>(req);
	std::error_code err         = map_uv_error(status);

	request_ptr->m_handler(get_channel_shared_ptr(req), err);
	request_ptr->m_handler = nullptr;
	delete req;
}

/* tcp_write_buf_req_uv */

logicmill::util::shared_ptr<tcp_channel_uv>
tcp_write_buf_req_uv::get_channel_shared_ptr(uv_write_t* req)
{
	return logicmill::util::dynamic_pointer_cast<tcp_channel_uv>(tcp_base_uv::get_base_shared_ptr(req->handle));
}

void
tcp_write_buf_req_uv::on_write(uv_write_t* req, int status)
{
	auto target = reinterpret_cast<tcp_write_buf_req_uv*>(req);
	if (target->m_write_handler)
	{
		std::error_code err = map_uv_error(status);
		target->m_write_handler(get_channel_shared_ptr(req), std::move(target->m_buffer), err);
	}
	delete target;
}

/* tcp_write_bufs_req_uv */

logicmill::util::shared_ptr<tcp_channel_uv>
tcp_write_bufs_req_uv::get_channel_shared_ptr(uv_write_t* req)
{
	return logicmill::util::dynamic_pointer_cast<tcp_channel_uv>(tcp_base_uv::get_base_shared_ptr(req->handle));
}

void
tcp_write_bufs_req_uv::on_write(uv_write_t* req, int status)
{
	auto target = reinterpret_cast<tcp_write_bufs_req_uv*>(req);
	if (target->m_write_handler)
	{
		std::error_code err = map_uv_error(status);
		target->m_write_handler(get_channel_shared_ptr(req), std::move(target->m_buffers), err);
	}
	delete target;
}

/* tcp_base_uv */

endpoint
tcp_base_uv::really_get_endpoint(std::error_code& err)
{
	endpoint         result;
	sockaddr_storage saddr;
	int              sockaddr_size{sizeof(sockaddr_storage)};
	err.clear();
	auto stat = uv_tcp_getsockname(&m_tcp_handle, reinterpret_cast<sockaddr*>(&saddr), &sockaddr_size);
	if (stat < 0)
	{
		err = map_uv_error(stat);
	}
	else
	{
		result = endpoint{saddr, err};
	}
	return result;
}

std::shared_ptr<logicmill::async::loop>
tcp_base_uv::get_loop()
{
	return reinterpret_cast<loop_data*>(reinterpret_cast<uv_handle_t*>(&m_tcp_handle)->loop->data)->get_loop_ptr();
}

/* tcp_channel_uv */

void
tcp_channel_uv::clear_handler()
{
	if (m_close_handler)
	{
		m_close_handler(logicmill::util::dynamic_pointer_cast<tcp_channel_uv>(m_data.m_self_ptr));
		m_close_handler = nullptr;
	}
	m_read_handler = nullptr;
}

void
tcp_channel_uv::init(uv_loop_t* lp, ptr const& self, std::error_code& err)
{
	auto stat = uv_tcp_init(lp, get_tcp_handle());
	uv_handle_set_data(get_handle(), get_handle_data());
	set_self_ptr(self);
	UV_ERROR_CHECK(stat, err, exit);
exit:
	return;
}

void
tcp_channel_uv::on_read(uv_stream_t* stream_handle, ssize_t nread, const uv_buf_t* buf)
{
	std::error_code err;
	ptr             channel_ptr = logicmill::util::dynamic_pointer_cast<tcp_channel_uv>(get_base_shared_ptr(stream_handle));
	assert(channel_ptr);
	if (nread < 0)
	{
		if (buf->base)
		{
			delete[] reinterpret_cast<byte_type*>(buf->base);
		}
		err = map_uv_error(nread);
		channel_ptr->m_read_handler(channel_ptr, util::const_buffer{}, err);
	}
	else if (nread > 0)
	{
		channel_ptr->m_read_handler(
				channel_ptr,
				util::const_buffer{reinterpret_cast<byte_type*>(buf->base),
								   static_cast<size_type>(nread),
								   std::default_delete<byte_type[]>{}},
				err);
	}
}

void
tcp_channel_uv::on_allocate(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	// static buffer::memory_broker::ptr broker = buffer::default_broker::get();
	buf->base = reinterpret_cast<char*>(new byte_type[suggested_size]);
	buf->len  = suggested_size;
}

bool
tcp_channel_uv::really_close(logicmill::async::channel::close_handler&& handler)
{
	bool result{false};
	if (!uv_is_closing(get_handle()))
	{
		result = true;
		m_close_handler = std::move(handler);
		uv_close(get_handle(), tcp_base_uv::on_close);
	}
	return result;
}

bool
tcp_channel_uv::really_close()
{
	bool result{false};
	if (!uv_is_closing(get_handle()))
	{
		result = true;
		uv_close(get_handle(), tcp_base_uv::on_close);
	}
	return result;
}

bool
tcp_channel_uv::is_closing()
{
	return uv_is_closing(get_handle());
}

void
tcp_channel_uv::really_start_read(std::error_code& err, async::channel::read_handler&& handler)
{
	err.clear();
	m_read_handler = std::move(handler);
	auto stat      = uv_read_start(get_stream_handle(), on_allocate, on_read);
	if (stat < 0)
	{
		err = map_uv_error(stat);
	}
}

void
tcp_channel_uv::really_start_read(std::error_code& err, async::channel::read_handler const& handler)
{
	err.clear();
	m_read_handler = handler;
	auto stat      = uv_read_start(get_stream_handle(), on_allocate, on_read);
	if (stat < 0)
	{
		err = map_uv_error(stat);
	}
}

void
tcp_channel_uv::stop_read()
{
	uv_read_stop(get_stream_handle());
}

void
tcp_channel_uv::really_write(
		util::mutable_buffer&&                 buf,
		std::error_code&                       err,
		async::channel::write_buffer_handler&& handler)
{
	err.clear();
	auto request = new tcp_write_buf_req_uv{std::move(buf), std::move(handler)};
	auto status  = request->start(get_stream_handle());
	if (status < 0)
	{
		err = map_uv_error(status);
	}
}

void
tcp_channel_uv::really_write(
		util::mutable_buffer&&                      buf,
		std::error_code&                            err,
		async::channel::write_buffer_handler const& handler)
{
	err.clear();
	auto request = new tcp_write_buf_req_uv{std::move(buf), handler};
	auto status  = request->start(get_stream_handle());
	if (status < 0)
	{
		err = map_uv_error(status);
	}
}

void
tcp_channel_uv::really_write(
		std::deque<util::mutable_buffer>&&      bufs,
		std::error_code&                        err,
		async::channel::write_buffers_handler&& handler)
{
	err.clear();
	auto request = new tcp_write_bufs_req_uv{std::move(bufs), std::move(handler)};
	auto status  = request->start(get_stream_handle());
	if (status < 0)
	{
		err = map_uv_error(status);
	}
}

void
tcp_channel_uv::really_write(
		std::deque<util::mutable_buffer>&&           bufs,
		std::error_code&                             err,
		async::channel::write_buffers_handler const& handler)
{
	err.clear();
	auto request = new tcp_write_bufs_req_uv{std::move(bufs), handler};
	auto status  = request->start(get_stream_handle());
	if (status < 0)
	{
		err = map_uv_error(status);
	}
}

endpoint
tcp_channel_uv::get_peer_endpoint(std::error_code& err)
{
	endpoint         result;
	sockaddr_storage saddr;
	int              sockaddr_size{sizeof(sockaddr_storage)};
	err.clear();
	auto stat = uv_tcp_getpeername(&m_tcp_handle, reinterpret_cast<sockaddr*>(&saddr), &sockaddr_size);
	if (stat < 0)
	{
		err = map_uv_error(stat);
	}
	else
	{
		result = endpoint{saddr, err};
	}
	return result;
}

endpoint
tcp_channel_uv::get_peer_endpoint()
{
	sockaddr_storage saddr;
	int              sockaddr_size{sizeof(sockaddr_storage)};
	auto stat = uv_tcp_getpeername(&m_tcp_handle, reinterpret_cast<sockaddr*>(&saddr), &sockaddr_size);
	if (stat < 0)
	{
		throw std::system_error{map_uv_error(stat)};
	}
	return endpoint{saddr};
}


// tcp_framed_channel_uv

void
tcp_framed_channel_uv::on_read(uv_stream_t* stream_handle, ssize_t nread, const uv_buf_t* buf)
{
	std::error_code err;
	ptr             channel_ptr = logicmill::util::dynamic_pointer_cast<tcp_framed_channel_uv>(get_base_shared_ptr(stream_handle));
	assert(channel_ptr);
	if (nread < 0)
	{
		if (buf->base)
		{
			delete[] reinterpret_cast<byte_type*>(buf->base);
		}
		err = map_uv_error(nread);
		channel_ptr->m_read_handler(channel_ptr, util::const_buffer{}, err);
	}
	else if (nread > 0)
	{
		channel_ptr->read_to_frame(
				channel_ptr,
				util::const_buffer{reinterpret_cast<byte_type*>(buf->base),
								   static_cast<size_type>(nread),
								   std::default_delete<byte_type[]>{}});
	}
}

void
tcp_framed_channel_uv::read_to_frame(ptr channel_ptr, util::const_buffer&& buf)
{
	assert((is_frame_size_valid() && (m_payload_buffer.size() < m_frame_size)) || (!is_frame_size_valid()));

	std::size_t current_buffer_position{0};
	std::size_t remaining_in_buffer{buf.size()};

	while (remaining_in_buffer > 0)
	{
		if (!is_frame_size_valid())
		{
			assert(!is_header_complete());
			std::size_t nbytes_to_move{0};
			auto        needed_to_complete = sizeof(m_header_buf) - m_header_byte_count;
			if (buf.size() >= needed_to_complete)
			{
				nbytes_to_move = needed_to_complete;
			}
			else
			{
				nbytes_to_move = buf.size();
			}
			util::mutable_buffer hbuf{&m_header_buf, 8};
			::memcpy(&m_header_buf[m_header_byte_count], buf.data() + current_buffer_position, nbytes_to_move);
			m_header_byte_count += nbytes_to_move;
			current_buffer_position += nbytes_to_move;
			remaining_in_buffer -= nbytes_to_move;
			if (is_header_complete())
			{
				m_frame_size = unpack_frame_header(&m_header_buf);
				assert(m_frame_size >= 0);

				assert(m_payload_buffer.size() == 0);
				if (m_frame_size > 0)
				{
					m_payload_buffer.expand(m_frame_size);
				}
			}
		}

		assert((is_frame_size_valid() || remaining_in_buffer < 1));

		if (is_frame_size_valid())
		{
			assert(m_payload_buffer.size() <= m_frame_size);
			std::size_t needed_to_complete{m_frame_size - m_payload_buffer.size()};
			if (needed_to_complete > 0 && remaining_in_buffer > 0)
			{
				assert(m_payload_buffer.capacity() == m_frame_size);
				std::size_t nbytes_to_move = std::min(remaining_in_buffer, needed_to_complete);
				assert(nbytes_to_move > 0);
				m_payload_buffer.putn(m_payload_buffer.size(), buf.data() + current_buffer_position, nbytes_to_move);
				needed_to_complete -= nbytes_to_move;
				current_buffer_position += nbytes_to_move;
				remaining_in_buffer -= nbytes_to_move;
				m_payload_buffer.size(m_payload_buffer.size() + nbytes_to_move);
			}
			if (needed_to_complete < 1)
			{
				std::error_code err;
				m_read_handler(channel_ptr, std::move(m_payload_buffer), err);
				m_header_byte_count = 0;
				assert(m_payload_buffer.size() == 0);
				m_frame_size = -1;
			}
		}
	}
}

void
tcp_framed_channel_uv::really_start_read(std::error_code& err, async::channel::read_handler&& handler)
{
	err.clear();
	m_read_handler = std::move(handler);
	auto stat      = uv_read_start(get_stream_handle(), on_allocate, on_read);
	if (stat < 0)
	{
		err = map_uv_error(stat);
	}
}

void
tcp_framed_channel_uv::really_start_read(std::error_code& err, async::channel::read_handler const& handler)
{
	err.clear();
	m_read_handler = handler;
	auto stat      = uv_read_start(get_stream_handle(), on_allocate, on_read);
	if (stat < 0)
	{
		err = map_uv_error(stat);
	}
}

class on_write_buffers
{
public:
	template<class T, class = std::enable_if_t<std::is_convertible<T, async::channel::write_buffer_handler>::value>>
	on_write_buffers(T&& handler) : m_handler{std::forward<T>(handler)}
	{}

	void
	operator()(async::channel::ptr const& chan, std::deque<util::mutable_buffer>&& bufs, std::error_code err)
	{
		if (m_handler)
		{
			m_handler(chan, std::move(bufs.back()), err);
		}
	}

private:
	async::channel::write_buffer_handler m_handler;
};


void
tcp_framed_channel_uv::really_write(
		util::mutable_buffer&&                 buf,
		std::error_code&                       err,
		async::channel::write_buffer_handler&& handler)
{
	err.clear();
	std::deque<util::mutable_buffer> frame_bufs;
	frame_bufs.emplace_back(pack_frame_header(buf.size()));
	frame_bufs.emplace_back(std::move(buf));

	auto request = new tcp_write_bufs_req_uv{std::move(frame_bufs), on_write_buffers{std::move(handler)}};

	auto status = request->start(get_stream_handle());
	if (status < 0)
	{
		err = map_uv_error(status);
	}
}

void
tcp_framed_channel_uv::really_write(
		util::mutable_buffer&&                      buf,
		std::error_code&                            err,
		async::channel::write_buffer_handler const& handler)
{
	err.clear();
	std::deque<util::mutable_buffer> frame_bufs;
	frame_bufs.emplace_back(pack_frame_header(buf.size()));
	frame_bufs.emplace_back(std::move(buf));

	auto request = new tcp_write_bufs_req_uv{std::move(frame_bufs), on_write_buffers{handler}};

	auto status = request->start(get_stream_handle());
	if (status < 0)
	{
		err = map_uv_error(status);
	}
}

void
tcp_framed_channel_uv::really_write(
		std::deque<util::mutable_buffer>&&      bufs,
		std::error_code&                        err,
		async::channel::write_buffers_handler&& handler)
{
	err.clear();
	std::uint64_t frame_size{0};
	for (auto& buf : bufs)
	{
		frame_size += buf.size();
	}
	bufs.emplace_front(pack_frame_header(frame_size));
	auto request = new tcp_write_bufs_req_uv{std::move(bufs), std::move(handler)};
	auto status  = request->start(get_stream_handle());
	if (status < 0)
	{
		err = map_uv_error(status);
	}
}

void
tcp_framed_channel_uv::really_write(
		std::deque<util::mutable_buffer>&&           bufs,
		std::error_code&                             err,
		async::channel::write_buffers_handler const& handler)
{
	err.clear();
	std::uint64_t frame_size{0};
	for (auto& buf : bufs)
	{
		frame_size += buf.size();
	}
	bufs.emplace_front(pack_frame_header(frame_size));
	auto request = new tcp_write_bufs_req_uv{std::move(bufs), handler};
	auto status  = request->start(get_stream_handle());
	if (status < 0)
	{
		err = map_uv_error(status);
	}
}

// tcp_acceptor_uv

void
tcp_acceptor_uv::init(uv_loop_t* lp, ptr const& self, logicmill::async::options const& opt, std::error_code& err)
{
	err.clear();
	set_self_ptr(self);
	uv_handle_set_data(get_handle(), get_handle_data());
	auto stat = uv_tcp_init(lp, get_tcp_handle());
	UV_ERROR_CHECK(stat, err, exit);
	sockaddr_storage saddr;
	m_endpoint.to_sockaddr(saddr);
	stat = uv_tcp_bind(get_tcp_handle(), reinterpret_cast<sockaddr*>(&saddr), 0);
	UV_ERROR_CHECK(stat, err, exit);
	if (opt.framing())
	{
		stat = uv_listen(reinterpret_cast<uv_stream_t*>(get_tcp_handle()), 128, on_framing_connection);
	}
	else
	{
		stat = uv_listen(reinterpret_cast<uv_stream_t*>(get_tcp_handle()), 128, on_connection);
	}
	UV_ERROR_CHECK(stat, err, exit);
exit:
	return;
}

void
tcp_acceptor_uv::on_connection(uv_stream_t* handle, int stat)
{
	auto acceptor_ptr = logicmill::util::dynamic_pointer_cast<tcp_acceptor_uv>(get_base_shared_ptr(handle));

	if (stat < 0)
	{
		std::error_code err = map_uv_error(stat);
		acceptor_ptr->m_connection_handler(acceptor_ptr, nullptr, err);
	}
	else
	{
		std::error_code err;
		auto            channel_ptr = logicmill::util::make_shared<tcp_channel_uv>();
		channel_ptr->init(acceptor_ptr->get_handle()->loop, channel_ptr, err);
		if (err)
		{
			acceptor_ptr->m_connection_handler(acceptor_ptr, channel_ptr, err);
		}
		else
		{
			int status = uv_accept(acceptor_ptr->get_stream_handle(), channel_ptr->get_stream_handle());
			if (status)
			{
				err = map_uv_error(status);
			}
			acceptor_ptr->m_connection_handler(acceptor_ptr, channel_ptr, err);
		}
	}
}

void
tcp_acceptor_uv::on_framing_connection(uv_stream_t* handle, int stat)
{

	auto acceptor_ptr = logicmill::util::dynamic_pointer_cast<tcp_acceptor_uv>(get_base_shared_ptr(handle));

	if (stat < 0)
	{
		std::error_code err = map_uv_error(stat);
		acceptor_ptr->m_connection_handler(acceptor_ptr, nullptr, err);
	}
	else
	{
		std::error_code err;
		auto            channel_ptr = logicmill::util::make_shared<tcp_framed_channel_uv>();
		channel_ptr->init(acceptor_ptr->get_handle()->loop, channel_ptr, err);
		if (err)
		{
			acceptor_ptr->m_connection_handler(acceptor_ptr, channel_ptr, err);
		}
		else
		{
			int status = uv_accept(acceptor_ptr->get_stream_handle(), channel_ptr->get_stream_handle());
			if (status)
			{
				err = map_uv_error(status);
			}
			acceptor_ptr->m_connection_handler(acceptor_ptr, channel_ptr, err);
		}
	}
}

bool
tcp_acceptor_uv::really_close(logicmill::async::acceptor::close_handler&& handler)
{
	bool result{false};
	if (!uv_is_closing(get_handle()))
	{
		result = true;
		m_close_handler = std::move(handler);
		uv_close(get_handle(), tcp_base_uv::on_close);
	}
	return result;
}

bool
tcp_acceptor_uv::really_close()
{
	bool result{false};
	if (!uv_is_closing(get_handle()))
	{
		result = true;
		uv_close(get_handle(), tcp_base_uv::on_close);
	}
	return result;
}
