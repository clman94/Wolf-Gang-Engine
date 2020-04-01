#pragma once

#include <wge/core/component_storage.hpp>
#include <wge/core/component_type.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/object_id.hpp>
#include <wge/util/ptr.hpp>

#include <map>

namespace wge::core
{

// Holds all types of components as contiguous arrays, each with their own container.
class component_manager
{
public:
	// Boilerplate for noexcept move because std::map can't get it right.
	component_manager() = default;
	component_manager(const component_manager&) = default;
	component_manager(component_manager&&) noexcept = default;
	component_manager& operator=(const component_manager&) = default;
	component_manager& operator=(component_manager&&) noexcept = default;

	template <typename T>
	auto& add_component(const object_id& pObject, bucket pBucket = default_bucket)
	{
		return get_storage<T>(pBucket).insert(pObject);
	}

	template <typename T, typename U = std::decay_t<T>,
		// Buckets can't be used as components.
		typename = std::enable_if_t<!std::is_same_v<U, bucket>>>
	auto& add_component(const object_id& pObject, T&& pComponent, bucket pBucket = default_bucket)
	{
		return get_storage<U>(pBucket).insert(pObject, std::forward<T>(pComponent));
	}

	template <typename T>
	void remove_component(const object_id& pObject, bucket pBucket = default_bucket)
	{
		remove_component(component_type::from<T>(pBucket), pObject);
	}

	void remove_component(const component_type& pType, const object_id& pObject)
	{
		auto iter = mContainers.find(pType);
		if (iter != mContainers.end())
			iter->second->remove(pObject);
	}

	template <typename T>
	auto* get_component(const object_id& pObject, bucket pBucket = default_bucket)
	{
		return get_storage<T>(pBucket).get(pObject);
	}

	template <typename T>
	const auto* get_component(const object_id& pObject, bucket pBucket = default_bucket) const
	{
		return get_storage<T>(pBucket).get(pObject);
	}

	template <typename T>
	auto& get_storage(bucket pBucket = default_bucket)
	{
		return get_container_impl<T>(pBucket);
	}

	template <typename T>
	const auto& get_storage(bucket pBucket = default_bucket) const
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
	template <typename T, typename U = bselect_adaptor<T>::type>
	component_storage<U>& get_container_impl(bucket pBucket = default_bucket) const
	{
		using storage_type = component_storage<U>;
		component_type type = component_type::from<T>(pBucket);
		auto iter = mContainers.find(type);
		// Create a new container if it doesn't exist
		if (iter == mContainers.end())
			iter = mContainers.emplace_hint(iter,
				std::make_pair(type, util::make_copyable_ptr<storage_type, component_storage_base>()));
		return *static_cast<storage_type*>(iter->second.get());
	}

private:
	mutable std::map<component_type, util::copyable_ptr<component_storage_base>> mContainers;
};

} // namespace wge::core
