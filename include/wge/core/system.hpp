#pragma once

#include <wge/core/serialize_type.hpp>
#include <wge/util/json_helpers.hpp>
#include <wge/core/component_type.hpp>

#include <string>
#include <vector>
#include <utility>
#include <any>
#include <map>

namespace wge::core
{

class component_set
{
public:
	template <typename T, typename U = std::decay_t<T>>
	U* insert(T&& pComponent, bucket pBucket = default_bucket)
	{
		if (auto existing = get<U>(pBucket))
		{
			return &(*existing = std::forward<T>(pComponent));
		}
		else
		{
			auto iter = mComponents.insert({ component_type::from<U>(pBucket), std::forward<T>(pComponent) }).first;
			return std::any_cast<U*>(&(*iter));
		}
	}

	template <typename T>
	T* get(bucket pBucket = default_bucket) noexcept
	{
		auto iter = mComponents.find(component_type::from<T>(pBucket));
		if (iter != mComponents.end())
			return std::any_cast<T*>(&(*iter));
		else
			return nullptr;
	}
	
	template <typename T>
	const T* get(bucket pBucket = default_bucket) const noexcept
	{
		auto iter = mComponents.find(component_type::from<T>(pBucket));
		if (iter != mComponents.end())
			return std::any_cast<T*>(&(*iter));
		else
			return nullptr;
	}

	template <typename T>
	bool has(bucket pBucket = default_bucket) const noexcept
	{
		return get<T>() != nullptr;
	}

	bool remove(component_type pType)
	{
		return mComponents.erase(pType) != 0;
	}

	template <typename T>
	bool remove(bucket pBucket = default_bucket)
	{
		return remove(component_type::from<T>(pBucket));
	}

	bool empty() const noexcept
	{
		return mComponents.empty();
	}

	std::size_t size() const noexcept
	{
		return mComponents.size();
	}

	void clear()
	{
		mComponents.clear();
	}

	void merge(component_set& pSet)
	{
		mComponents.merge(pSet.mComponents);
	}

private:
	std::map<component_type, std::any> mComponents;
};

} // namespace wge::core
