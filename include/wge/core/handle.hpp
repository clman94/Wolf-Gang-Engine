#pragma once

#include <wge/core/object_id.hpp>
#include <wge/core/component_storage.hpp>

namespace wge::core
{

// A lightweight handle to a component.
// A handle to a component won't expire until
// the component it points to is destroyed.
template <typename T>
class handle
{
public:
	using const_storage = const component_storage<std::remove_const_t<T>>;
	using storage = std::conditional_t<std::is_const_v<T>, const_storage, std::remove_const_t<const_storage>>;

	handle() = default;
	handle(object_id pId, storage& pStorage) :
		mId(pId),
		mStorage(&pStorage)
	{}
	constexpr handle(std::nullptr_t) noexcept {}

	object_id get_object_id() const noexcept
	{
		assert(is_valid());
		return mId;
	}

	bool is_valid() const noexcept
	{
		return mStorage != nullptr && mStorage->has(mId);
	}

	T& get() const noexcept
	{
		assert(is_valid());
		return *mStorage->get(mId);
	}

	T* operator->() const noexcept
	{
		return &get();
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
	storage* mStorage = nullptr;
};

} // namespace wge::core
