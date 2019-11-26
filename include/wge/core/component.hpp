#pragma once

#include <string>
#include <array>

#include <wge/math/aabb.hpp>
#include <wge/core/serialize_type.hpp>
#include <wge/core/component_type.hpp>
#include <wge/core/component_storage.hpp>
#include <wge/util/uuid.hpp>
#include <wge/util/json_helpers.hpp>
#include <wge/core/asset_manager.hpp>

namespace wge::core
{

template <typename T>
class handle
{
public:
	using storage = component_storage<T>;

	handle() = default;
	handle(object_id pId, storage& pStorage) :
		mId(pId),
		mStorage(&pStorage)
	{}

	object_id get_object_id() const noexcept
	{
		assert(is_valid());
		return mId;
	}

	bool is_valid() const noexcept
	{
		return mStorage != nullptr && mStorage->has_component(mId);
	}

	T* operator->() const noexcept
	{
		assert(is_valid());
		return mStorage->get(mId);
	}

	T& operator*() const noexcept
	{
		assert(is_valid());
		return mStorage->get(mId);
	}

	void reset() noexcept
	{
		mId = invalid_id;
		mStorage = nullptr;
	}

private:
	object_id mId;
	component_storage<T>* mStorage = nullptr;
};

} // namespace wge::core
