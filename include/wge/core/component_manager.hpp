#pragma once

#include <wge/core/instance_id.hpp>
#include <wge/core/component_storage.hpp>

#include <map>
#include <any>
#include <vector>

namespace wge::core
{

// Holds all types of components as contiguous arrays, each with their own container.
class component_manager
{
public:
	template <typename T>
	T& add_component()
	{
		return get_container<T>().create_component();
	}

	// Get first component of this type for this object
	template <typename T>
	T* get_first_component(instance_id pId)
	{
		for (auto& i : get_container<T>())
			if (i.get_object_id() == pId)
				return &i;
		return nullptr;
	}

	// Returns the container associated with this type of component
	template <typename T>
	component_storage<T>& get_container()
	{
		using storage_type = component_storage<T>;
		if (mContainers.find(T::COMPONENT_ID) == mContainers.end())
			mContainers[T::COMPONENT_ID] = std::make_unique<storage_type>();
		return *dynamic_cast<storage_type*>(mContainers[T::COMPONENT_ID].get());
	}

	// Remove all components
	void remove_entity(instance_id pId)
	{
		for (auto& i : mContainers)
			i.second->remove_entity(pId);
	}

private:
	std::map<int, std::unique_ptr<component_storage_base>> mContainers;
};

} // namespace wge::core