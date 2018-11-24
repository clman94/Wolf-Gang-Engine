#pragma once

#include <string>
#include <vector>
#include <utility>
#include <any>

#define WGE_SYSTEM(name__, id__) \
	public: \
	static constexpr int SYSTEM_ID = id__; \
	static constexpr const char* SYSTEM_NAME = name__; \
	virtual int get_system_id() const override { return id__; } \
	virtual std::string get_system_name() const override { return name__; }

namespace wge::core
{

class layer;

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
	}

	// Call all components of this type
	template <typename T>
	std::vector<T*> get_all_components(int pObject_id)
	{
		for (auto& i : get_container<T>())
			if (i.get_object_id() == pObject_id)
				return &i;
	}

	// Returns the container associated with this type of component
	template <typename T>
	container<T>& get_container()
	{
		if (mContainers.find(T::COMPONENT_ID) == mContainers.end())
			mContainers[T::COMPONENT_ID] = component_container<T>();
		return std::any_cast<container<T>&>(mContainers[T::COMPONENT_ID]);
	}

private:
	std::map<int, std::any> mContainers;
};

class system
{
public:
	system(layer& pLayer) :
		mLayer(pLayer)
	{}
	virtual ~system() {}
	virtual int get_system_id() const = 0;
	virtual std::string get_system_name() const = 0;

	layer& get_layer() const
	{
		return mLayer;
	}

	virtual void preupdate(float pDelta) {}
	virtual void update(float pDelta) {}
	virtual void postupdate(float pDelta) {}

private:
	std::reference_wrapper<layer> mLayer;
};

} // namespace wge::core
