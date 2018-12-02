#include "timer_uv.h"
#include "loop_uv.h"
#include <iostream>

using namespace logicmill;

timer_uv::timer_uv(uv_loop_t* lp, std::error_code& err, async::timer::handler const& handler) : m_handler{handler}
{
	auto status = uv_timer_init(lp, &m_uv_timer);
	UV_ERROR_CHECK(status, err, exit);
exit:
	return;
}

timer_uv::timer_uv(uv_loop_t* lp, std::error_code& err, async::timer::handler&& handler) : m_handler{std::move(handler)}
{
	auto status = uv_timer_init(lp, &m_uv_timer);
	UV_ERROR_CHECK(status, err, exit);
exit:
	return;
}

timer_uv::~timer_uv()
{
	// std::cout << "in timer_uv destructor" << std::endl;
	// assert( m_uv_timer == nullptr );
}

void
timer_uv::init(timer_uv::ptr const& self)
{
	m_data.m_impl_ptr = self;
	uv_handle_set_data(reinterpret_cast<uv_handle_t*>(&m_uv_timer), &m_data);
}

void
timer_uv::start(std::chrono::milliseconds timeout, std::error_code& err)
{
	int status = 0;
	err.clear();

	if (is_active())
	{
		err = make_error_code(std::errc::operation_in_progress);
		goto exit;
	}

	status = uv_timer_start(&m_uv_timer, on_timer_expire, timeout.count(), 0);
	UV_ERROR_CHECK(status, err, exit);

exit:
	return;
}

void
timer_uv::stop(std::error_code& err)
{
	err.clear();

	if (is_active())
	{
		auto status = uv_timer_stop(&m_uv_timer);
		UV_ERROR_CHECK(status, err, exit);
	}

exit:
	return;
}

std::shared_ptr<logicmill::async::loop>
timer_uv::loop()
{
	return reinterpret_cast<loop_data*>(reinterpret_cast<uv_handle_t*>(&m_uv_timer)->loop->data)->get_loop_ptr();
}

void
timer_uv::close()
{
	if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_uv_timer)))
	{
		uv_close(reinterpret_cast<uv_handle_t*>(&m_uv_timer), on_timer_close);
	}
}

bool
timer_uv::is_active() const
{
	return uv_is_active(reinterpret_cast<const uv_handle_t*>(&m_uv_timer));
}

bool
timer_uv::is_pending() const
{
	return is_active();
}

void
timer_uv::clear()
{
	m_handler = nullptr;          // clear handler--possibly a closure holding shared references
	m_data.m_impl_ptr.reset();    // release shared self-reference
}

void
timer_uv::on_timer_close(uv_handle_t* handle)
{
	assert(uv_handle_get_type(handle) == uv_handle_type::UV_TIMER);
	// auto timer_handle = reinterpret_cast< uv_timer_t* >( handle );
	auto self_raw_ptr = reinterpret_cast<timer_handle_data*>(handle->data)->m_impl_ptr.get();
	assert(reinterpret_cast<uv_handle_t*>(&(self_raw_ptr->m_uv_timer)) == handle);
	self_raw_ptr->clear();
}

void
timer_uv::on_timer_expire(uv_timer_t* handle)
{
	timer_handle_data* const data
			= reinterpret_cast<timer_handle_data*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(handle)));
	data->m_impl_ptr->m_handler(data->m_impl_ptr);

	if (!uv_is_active(reinterpret_cast<uv_handle_t*>(handle)))
	{
		// std::cout << "timer refcount is " << data->m_impl_ptr.use_count() << std::endl;
		if (data->m_impl_ptr.use_count() <= 1)
		{
			// The timer is not active. If the self-reference ( in m_data ) is the only extant reference,
			// timer will become unusable and it will constitute a memory leak. Close it.
			// Closing resets the self-reference, resulting in destruction.
			// std::cout << "closing timer: low refcount" << std::endl;
			data->m_impl_ptr->close();
		}
	}
}
