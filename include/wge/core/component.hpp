#pragma once

#include <wge/core/object_id.hpp>
#include <wge/core/component_storage.hpp>
#include <wge/util/ptr_adaptor.hpp>

namespace wge::core
{

// A lightweight handle to a component.
// A handle to a component won't expire until
// the component it points to is destroyed.
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

	T& get() const noexcept
	{
		assert(is_valid());
		return *mStorage->get(mId);
	}

	auto operator->() const noexcept
	{
		return util::ptr_adaptor{ get() };
	}

	T& operator*() const noexcept
	{
		return get();
	}

	void reset() noexcept
	{
		mId = invalid_id;
		mStorage = nullptr;
	}

private:
	object_id mId = invalid_id;
	component_storage<T>* mStorage = nullptr;
};

} // namespace wge::core
