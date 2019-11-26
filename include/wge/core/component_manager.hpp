#pragma once

#include <wge/core/component_storage.hpp>
#include <wge/core/component_type.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/game_object.hpp>

#include <map>

namespace wge::core
{

// Holds all types of components as contiguous arrays, each with their own container.
class component_manager
{
public:
	template <typename T>
	T& add_component(const object_id& pObject)
	{
		return get_storage<T>().add(pObject);
	}

	void remove_component(const component_type& pType, const object_id& pObject)
	{
		auto iter = mContainers.find(pType);
		if (iter != mContainers.end())
			iter->second->remove(pObject);
	}

	template <typename T>
	T* get_component(const object_id& pObject, bucket pBucket = default_bucket)
	{
		return get_storage<T>().get(pObject);
	}

	template <typename T>
	component_storage<T>& get_storage(bucket pBucket = default_bucket)
	{
		return get_container_impl<T>(pBucket);
	}

	template <typename T>
	const component_storage<T>& get_storage(bucket pBucket = default_bucket) const
	{
		return get_container_impl<T>(pBucket);
	}


	component_storage_base* get_storage(const component_type& pType)
	{
		auto iter = mContainers.find(pType);
		if (iter == mContainers.end())
			return nullptr;
		return iter->second.get();
	}

	// Remove all components for this entity
	void remove_object(const object_id& pObject)
	{
		for (auto& i : mContainers)
			i.second->remove(pObject);
	}

	void clear()
	{
		mContainers.clear();
	}

private:
	template <typename T>
	component_storage<T>& get_container_impl(bucket pBucket = default_bucket) const
	{
		using storage_type = component_storage<T>;
		component_type type = component_type::from<T>(pBucket);
		auto iter = mContainers.find(type);
		// Create a new container if it doesn't exist
		if (iter == mContainers.end())
			iter = mContainers.emplace_hint(iter, std::make_pair(type, std::make_unique<storage_type>()));
		return *static_cast<storage_type*>(iter->second.get());
	}

private:
	mutable std::map<component_type, std::unique_ptr<component_storage_base>> mContainers;
};

} // namespace wge::core
