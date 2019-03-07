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

#ifndef LOGICMILL_ARMI_ADAPTER_ASYNC_CHANNEL_MANAGER_H
#define LOGICMILL_ARMI_ADAPTER_ASYNC_CHANNEL_MANAGER_H

#include <functional>
#include <logicmill/armi/types.h>
#include <logicmill/async/channel.h>
#include <unordered_map>
#include <vector>

namespace logicmill
{
namespace async
{

class channel_manager
{
public:
	async::channel::ptr
	get_channel(armi::channel_id_type channel_id);

protected:
	using channel_map_type         = std::unordered_map<armi::channel_id_type, async::channel::ptr>;
	using channel_map_iterator     = channel_map_type::iterator;
	using channel_map_element_type = channel_map_type::value_type;

	channel_manager() : m_next_channel_id{1} {}

	armi::channel_id_type
	new_channel(async::channel::ptr const& chan);

	void
	remove(armi::channel_id_type channel_id)
	{
		m_channel_map.erase(channel_id);
	}

	template<class Visitor>
	void
	visit(armi::channel_id_type channel_id, Visitor visitor)
	{
		auto it = m_channel_map.find(channel_id);
		if (it != m_channel_map.end())
			visitor(channel_id, it->second);
	}

	template<class Visitor>
	channel_map_iterator
	visit_and_remove(armi::channel_id_type channel_id, Visitor visitor)
	{
		auto it = m_channel_map.find(channel_id);
		if (it != m_channel_map.end())
			visitor(channel_id, it->second);
		return m_channel_map.erase(it);
	}

	template<class Visitor>
	void
	visit_and_remove_all(Visitor visitor)
	{
		auto it = m_channel_map.begin();
		while (it != m_channel_map.end())
		{
			visitor(it->first, it->second);
			it = m_channel_map.erase(it);
		}
	}

	template<class Visitor>
	void
	visit_all(Visitor visitor)
	{
		auto it = m_channel_map.begin();
		while (it != m_channel_map.end())
		{
			visitor(it->first, it->second);
			++it;
		}
	}

	void
	clear_map()
	{
		m_channel_map.clear();
	}

	std::size_t
	active_channel_count() const
	{
		return m_channel_map.size();
	}

	armi::channel_id_type
	get_next_channel_id()
	{
		return m_next_channel_id++;
	}

private:
	armi::channel_id_type m_next_channel_id;
	channel_map_type      m_channel_map;
};

}    // namespace async
}    // namespace logicmill

#endif    // LOGICMILL_ARMI_ADAPTER_ASYNC_CHANNEL_MANAGER_H
