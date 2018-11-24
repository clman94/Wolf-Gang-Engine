#pragma once

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
	using container = std::vector<T>;

	template <typename T>
	T& add_component()
	{
		return get_container<T>().emplace_back();
	}

	// Get first component of this type for this object
	template <typename T>
	T* get_first_component(int pObject_id)
	{
		for (auto& i : get_container<T>())
			if (i.get_object_id() == pObject_id)
				return &i;
		return nullptr;
	}

	// Call all components of this type
	template <typename T>
	std::vector<T*> get_all_components(int pObject_id)
	{
		for (auto& i : get_container<T>())
			if (i.get_object_id() == pObject_id)
				return &i;
		return{};
	}

	// Returns the container associated with this type of component
	template <typename T>
	container<T>& get_container()
	{
		if (mContainers.find(T::COMPONENT_ID) == mContainers.end())
			mContainers[T::COMPONENT_ID] = std::any(container<T>{});
		return std::any_cast<container<T>&>(mContainers[T::COMPONENT_ID]);
	}

private:
	std::map<int, std::any> mContainers;
};

} // namespace wge::core