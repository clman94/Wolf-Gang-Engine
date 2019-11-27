#pragma once

#include <wge/util/uuid.hpp>
#include <wge/logging/log.hpp>
#include <queue>
#include <cstddef>

namespace wge::core
{

using object_id = std::size_t;
constexpr object_id invalid_id = 0;

struct object_id_generator
{
public:
	object_id get()
	{
		if (!available_ids.empty())
		{
			object_id temp = available_ids.top();
			available_ids.pop();
			return temp;
		}
		return ++counter;
	}

	void reclaim(object_id pId)
	{
		assert(pId != invalid_id);
		if (pId <= counter)
		{
			available_ids.push(pId);
			while (!available_ids.empty() && available_ids.top() == counter)
			{
				--counter;
				available_ids.pop();
			}
		}
	}

private:
	std::priority_queue<object_id> available_ids;
	object_id counter = 0;
};

object_id_generator& get_global_generator();

} // namespace wge::core
