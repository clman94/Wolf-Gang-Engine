#pragma once

#include <deque>

#include <wge/util/uuid.hpp>

namespace wge::core
{

class component;

class component_storage_base
{
public:
	using component_ptr_list = std::vector<component*>;

	virtual ~component_storage_base() {}

	virtual void remove_component(const util::uuid& pComponent_id) = 0;

	// Get all generic component referencing this object
	virtual component_ptr_list get_all_components(const util::uuid& pObject_id) = 0;

	// Get first generic component referencing this object
	virtual component* get_first_component(const util::uuid& pObject_id) = 0;

	virtual int get_component_type() const = 0;

	// Get component by its id
	virtual component* get_component(const util::uuid& pComponent_id) = 0;

	// Remove all components that reference this entity
	virtual void remove_object(const util::uuid& pObject_id) = 0;
};

template <typename T>
class component_storage :
	public component_storage_base
{
public:
	// TODO: Create a custom chunked container that allows control
	//   over the size of those chunks.
	using container = std::deque<T>;
	using iterator = typename container::iterator; 

	T& add_component()
	{
		// Find a slot that is unused and reinitialize it.
		for (auto& i : mStorage)
			if (i.is_unused())
				return i = T{};
		return mStorage.emplace_back();
	}

	virtual void remove_component(const util::uuid& pComponent_id)
	{
		for (auto& i : mStorage)
		{
			if (i.get_instance_id() == pComponent_id)
			{
				i.destroy();
				break;
			}
		}
	}

	virtual component_ptr_list get_all_components(const util::uuid& pObject_id) override
	{
		component_ptr_list result;
		for (auto& i : mStorage)
			if (i.get_object_id() == pObject_id && !i.is_unused())
				result.push_back(&i);
		return result;
	}

	virtual component* get_first_component(const util::uuid& pObject_id) override
	{
		for (auto& i : mStorage)
			if (i.get_object_id() == pObject_id && !i.is_unused())
				return &i;
		return nullptr;
	}

	virtual int get_component_type() const override
	{
		return T::COMPONENT_ID;
	}

	virtual component* get_component(const util::uuid& pComponent_id) override
	{
		for (auto& i : mStorage)
			if (i.get_instance_id() == pComponent_id && !i.is_unused())
				return &i;
		return nullptr;
	}

	virtual void remove_object(const util::uuid& pObject_id) override
	{
		// Mark all the components pointing to this object as unused.
		for (auto& i : mStorage)
			if (i.get_object_id() == pObject_id)
				i.destroy();
	}

	iterator begin() noexcept
	{
		return mStorage.begin();
	}

	iterator end() noexcept
	{
		return mStorage.end();
	}

private:
	container mStorage;
};

} // namespace wge::core
