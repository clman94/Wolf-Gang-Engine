#pragma once

#include <wge/core/component_storage.hpp>
#include <wge/core/component_type.hpp>

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

	void remove_component(const component_type& pType, const util::uuid& pComponent_id)
	{
		auto iter = mContainers.find(pType);
		if (iter != mContainers.end())
			iter->second->remove_component(pComponent_id);
	}

	// Get first component of this type for this object
	template <typename T>
	T* get_first_component(const util::uuid& pObject_id)
	{
		for (auto& i : get_container<T>())
			if (i.get_object_id() == pObject_id && !i.will_be_destroyed())
				return &i;
		return nullptr;
	}
	// Get first component of this type for this object
	component* get_first_component(const component_type& pType, const util::uuid& pObject_id)
	{
		component_storage_base* storage = get_container(pType);
		if (!storage)
			return nullptr;
		return storage->get_first_component(pObject_id);
	}

	component* get_component(const component_type& pType, const util::uuid& pComponent_id)
	{
		auto iter = mContainers.find(pType);
		if (iter == mContainers.end())
			return nullptr;
		return iter->second->get_component(pComponent_id);
	}

	// Returns the container associated with this type of component
	template <typename T>
	component_storage<T>& get_container()
	{
		using storage_type = component_storage<T>;
		// Create a new container if it doesn't exist
		if (mContainers.find(T::COMPONENT_ID) == mContainers.end())
			mContainers[T::COMPONENT_ID] = std::make_unique<storage_type>();
		return *dynamic_cast<storage_type*>(mContainers[T::COMPONENT_ID].get());
	}
	component_storage_base* get_container(const component_type& pType)
	{
		auto iter = mContainers.find(pType);
		if (iter == mContainers.end())
			return nullptr;
		return iter->second.get();
	}

	// Remove all components for this entity
	void remove_object(const util::uuid& pObject_id)
	{
		for (auto& i : mContainers)
			i.second->remove_object(pObject_id);
	}

	// Run cleanup of marked components. Returns the amount of components destroyed.
	std::size_t cleanup(const std::function<void(component*)>& pBefore_destroy_callback = {})
	{
		std::size_t total = 0;
		for (auto& i : mContainers)
			total += i.second->clean_marked_components(pBefore_destroy_callback);
		return total;
	}

private:
	std::map<component_type, std::unique_ptr<component_storage_base>> mContainers;
};

} // namespace wge::core