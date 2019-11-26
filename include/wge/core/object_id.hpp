#pragma once

#include <wge/util/uuid.hpp>
#include <queue>
#include <cstddef>

namespace wge::core
{

using object_id = std::size_t;
constexpr object_id invalid_id = 0;

struct object_id_generator
{
public:
	static inline object_id get()
	{
		if (!available_ids.empty())
		{
			object_id temp = available_ids.top();
			available_ids.pop();
			return temp;
		}
		return ++counter;
	}

	static inline void reclaim(object_id pId)
	{
		available_ids.push(pId);
		while (!available_ids.empty() && available_ids.top() == counter)
		{
			--counter;
			available_ids.pop();
		}
	}

private:
	static inline std::priority_queue<object_id> available_ids;
	static inline object_id counter = 0;
};

} // namespace wge::core
