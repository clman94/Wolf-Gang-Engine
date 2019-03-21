#pragma once

#include <vector>
#include <memory>
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

	// Remove all components that are marked for destruction.
	// Returns the amount of components destroyed.
	virtual std::size_t clean_marked_components(const std::function<void(component*)>& pBefore_destroy_callback = {}) = 0;
};

template <typename T>
class component_storage :
	public component_storage_base
{
public:
	using container = std::vector<T>;
	using iterator = typename container::iterator; 

	T& create_component()
	{
		return mStorage.emplace_back();
	}

	virtual void remove_component(const util::uuid& pComponent_id)
	{
		for (std::size_t i = 0; i < mStorage.size(); i++)
		{
			if (mStorage[i].get_instance_id() == pComponent_id)
			{
				mStorage.erase(mStorage.begin() + i);
				break;
			}
		}
	}

	virtual component_ptr_list get_all_components(const util::uuid& pObject_id) override
	{
		component_ptr_list result;
		for (auto& i : mStorage)
			if (i.get_object_id() == pObject_id && !i.will_be_destroyed())
				result.push_back(&i);
		return result;
	}

	virtual component* get_first_component(const util::uuid& pObject_id) override
	{
		for (auto& i : mStorage)
			if (i.get_object_id() == pObject_id && !i.will_be_destroyed())
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
			if (i.get_instance_id() == pComponent_id && !i.will_be_destroyed())
				return &i;
		return nullptr;
	}

	virtual void remove_object(const util::uuid& pObject_id) override
	{
		// Mark all the components for destruction.
		for (std::size_t i = 0; i < mStorage.size(); i++)
			if (mStorage[i].get_object_id() == pObject_id)
				mStorage[i].destroy();
	}

	virtual std::size_t clean_marked_components(const std::function<void(component*)>& pBefore_destroy_callback) override
	{
		std::size_t original = mStorage.size();
		for (std::size_t i = 0; i < mStorage.size(); i++)
		{
			if (mStorage[i].will_be_destroyed())
			{
				if (pBefore_destroy_callback)
					pBefore_destroy_callback(&mStorage[i]);
				mStorage.erase(mStorage.begin() + i--);
			}
		}
		return original - mStorage.size();
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
	std::vector<T> mStorage;
};

} // namespace wge::core
