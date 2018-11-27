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
	T& add_component(component_id pId)
	{
		return get_container<T>().create_component(pId);
	}

	void remove_component(int pType, component_id pId)
	{
		auto iter = mContainers.find(pType);
		if (iter != mContainers.end())
			iter->second->remove_component(pId);
	}

	// Get first component of this type for this object
	template <typename T>
	T* get_first_component(object_id pId)
	{
		for (auto& i : get_container<T>())
			if (i.get_object_id() == pId)
				return &i;
		return nullptr;
	}
	// Get first component of this type for this object
	component* get_first_component(int pType, object_id pId)
	{
		component_storage_base* storage = get_container(pType);
		if (!storage)
			return nullptr;
		return storage->get_first_component(pId);
	}

	component* get_component(int pType, component_id pId)
	{
		auto iter = mContainers.find(pType);
		if (iter == mContainers.end())
			return nullptr;
		return iter->second->get_component(pId);
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
	component_storage_base* get_container(int pType)
	{
		auto iter = mContainers.find(pType);
		if (iter == mContainers.end())
			return nullptr;
		return iter->second.get();
	}


	// Remove all components for this entity
	void remove_object(object_id pId)
	{
		for (auto& i : mContainers)
			i.second->remove_object(pId);
	}

private:
	std::map<int, std::unique_ptr<component_storage_base>> mContainers;
};

} // namespace wge::core