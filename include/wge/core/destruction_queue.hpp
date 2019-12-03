#pragma once

#include <wge/core/object_id.hpp>
#include <wge/core/component_type.hpp>
#include <wge/core/component_manager.hpp>

#include <vector>

namespace wge::core
{

class destruction_queue
{
	using component_entry = std::pair<object_id, component_type>;
public:
	void push_component(object_id pId, component_type pType)
	{
		mComponents_queue.push_back(std::make_pair(pId, pType));
	}

	void push_object(object_id pId)
	{
		mObjects_queue.push_back(pId);
	}

	void apply(component_manager& pComponent_manager)
	{
		// Remove all the components.
		for (auto[id, type] : mComponents_queue)
			pComponent_manager.remove_component(type, id);
		mComponents_queue.clear();

		// Remove all the objects.
		for (auto i : mObjects_queue)
		{
			get_global_generator().reclaim(i);
			pComponent_manager.remove_object(i);
		}
		mObjects_queue.clear();
	}

	bool empty() const noexcept
	{
		return mObjects_queue.empty() || mComponents_queue.empty();
	}

	void clear()
	{
		mObjects_queue.clear();
		mComponents_queue.clear();
	}

private:
	std::vector<object_id> mObjects_queue;
	std::vector<component_entry> mComponents_queue;
};

constexpr struct queue_destruction_flag {} queue_destruction;

} // namespace wge::core
