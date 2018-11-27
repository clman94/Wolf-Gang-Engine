#pragma once

#include <vector>
#include <memory>

namespace wge::core
{

class component;

class component_storage_base
{
public:
	using component_ptr_list = std::vector<component*>;

	virtual ~component_storage_base() {}

	// Get all generic component referencing this object
	virtual component_ptr_list get_all_components(object_id) = 0;

	// Get first generic component referencing this object
	virtual component* get_first_component(object_id) = 0;

	virtual int get_component_type() const = 0;

	// Get component by its id
	virtual component* get_component(component_id) = 0;

	// Remove all components that reference this entity
	virtual void remove_object(object_id) = 0;
};

template <typename T>
class component_storage :
	public component_storage_base
{
public:
	using container = std::vector<T>;
	using iterator = typename container::iterator; 

	T& create_component(component_id pId)
	{
		return mStorage.emplace_back(pId);
	}

	virtual component_ptr_list get_all_components(object_id pObject) override
	{
		component_ptr_list result;
		for (auto& i : mStorage)
			if (i.get_object_id() == pObject)
				result.push_back(&i);
		return result;
	}

	virtual component* get_first_component(object_id pObject) override
	{
		for (auto& i : mStorage)
			if (i.get_object_id() == pObject)
				return &i;
		return nullptr;
	}
	virtual int get_component_type() const override
	{
		return T::COMPONENT_ID;
	}

	virtual component* get_component(component_id pId) override
	{
		for (auto& i : mStorage)
			if (i.get_instance_id() == pId)
				return &i;
		return nullptr;
	}

	virtual void remove_object(object_id pObject) override
	{
		for (std::size_t i = 0; i < mStorage.size(); i++)
			if (mStorage[i].get_object_id() == pObject)
				mStorage.erase(mStorage.begin() + i--);
	}

	iterator begin()
	{
		return mStorage.begin();
	}

	iterator end()
	{
		return mStorage.end();
	}

private:
	std::vector<T> mStorage;
};

} // namespace wge::core
