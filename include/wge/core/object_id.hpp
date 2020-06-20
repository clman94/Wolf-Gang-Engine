#pragma once

#include <wge/util/uuid.hpp>
#include <wge/logging/log.hpp>
#include <queue>
#include <cstddef>
#include <utility>

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
			do {
				available_ids.pop();
				// Clean out the duplicates. If needed.
			} while (!available_ids.empty() && available_ids.top() == temp);
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
			// Unwind the queue.
			while (!available_ids.empty() && available_ids.top() >= counter)
			{
				// Duplicates would cause the counter to decrement more than it should.
				// This safeguards against that.
				if (available_ids.top() == counter)
					--counter;
				available_ids.pop();
			}
		}
	}

private:
	std::priority_queue<object_id> available_ids;
	object_id counter = 0;
};

class object_id_owner
{
public:
	object_id_owner() noexcept = default;
	object_id_owner(object_id pId, object_id_generator& pGenerator) noexcept
		: mId(pId), pGenerator(&pGenerator)
	{}

	object_id_owner(const object_id_owner&) = delete;
	object_id_owner& operator=(const object_id_owner&) = delete;

	object_id_owner(object_id_owner&& pOther) noexcept :
		mId(std::exchange(pOther.mId, invalid_id)),
		pGenerator(std::exchange(pOther.pGenerator, nullptr))
	{}
	object_id_owner& operator=(object_id_owner&& pOther) noexcept
	{
		mId = std::exchange(pOther.mId, invalid_id);
		pGenerator = std::exchange(pOther.pGenerator, nullptr);
		return *this;
	}

	~object_id_owner()
	{
		release();
	}

	object_id get_id() const noexcept
	{
		return mId;
	}

	void release()
	{
		if (pGenerator)
		{
			pGenerator->reclaim(mId);
			mId = invalid_id;
			pGenerator = nullptr;
		}
	}

private:
	object_id mId = invalid_id;
	object_id_generator* pGenerator = nullptr;
};

object_id_generator& get_global_generator();

} // namespace wge::core
