#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/system.hpp>
#include <wge/core/component_manager.hpp>
#include <wge/core/object_manager.hpp>
#include <wge/core/component_type.hpp>
#include <wge/core/factory.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

namespace wge::core
{

// A layer is a self-contained collection of objects
// with its own set of systems acting upon the components
// those objects.
// In short, this is where all the ECS goodies happen.
class layer
{
public:
	using uptr = std::unique_ptr<layer>;

	// Create a new layer object
	[[nodiscard]] static uptr create(const factory& pFactory)
	{
		return std::make_unique<layer>(pFactory);
	}

	layer(const factory&) noexcept;

	json serialize(serialize_type);
	void deserialize(const asset_manager&, const json&);

	// Get a system by its type
	template <typename T>
	T* get_system() const;
	// Get a system by its type
	system* get_system(int pID) const;
	// Get a system by name
	system* get_system(const std::string& pName) const;

	// Create a system that was not registered in the factory.
	template <typename T, typename...Targs>
	T* add_unregistered_system(Targs&&...);
	// Uses the current factory to construct the system.
	template <typename T>
	T* add_system();
	// Create a new system that was registered in the factory.
	system* add_system(int pType);

	// Set the name of this layer
	void set_name(const std::string& pName) noexcept;
	// Get the name of this layer
	const std::string& get_name() const noexcept;

	// Create a new game object in this layer.
	[[nodiscard]] game_object add_object();
	game_object add_object(const std::string& pName);
	// Remove a game object
	void remove_object(game_object& mObj);
	void remove_all_objects();
	// Get a game object at an index
	game_object get_object(std::size_t pIndex);
	// Get a game object by its instance id
	game_object get_object(const util::uuid& pId);
	// Get the amount of game objects in this layer
	std::size_t get_object_count() const noexcept;

	// Add a new component to a game object
	template <typename T>
	T* add_component(const game_object& pObj);
	// Add a new component by type
	component* add_component(const game_object& pObj, const component_type& pType);

	// Get a component by its instance id. The type is needed to check in
	// the correct container.
	component* get_component(const component_type& pType, const util::uuid& pId);

	// Get the container associated to a specific type of component.
	template <typename T>
	component_storage<T>& get_component_container();

	// Remove a component of a specific instance id and type.
	void remove_component(int pType, const util::uuid& pId);

	// Calls pCallable for each component specified by the first parameter.
	// Each component parameter should be a non-const reference and derive from
	// the component class.
	// Example:
	//    // This will iterate over each sprite component in the layer.
	//    layer.for_each([&](sprite_component& pTarget) {});
	//    
	//    // This will iterate over each sprite component and will collect
	//    // the first transform component of the owning game_object.
	//    layer.for_each([&](sprite_component& pTarget, transform_component& pTransform) {});
	//    
	//    // You can also get the current game object by adding a `game_object` parameter at the start.
	//    layer.for_each([&](game_object pObject, sprite_component& pTarget) {});
	//
	template <typename T>
	void for_each(T&& pCallable);

	// Set whether or not this layer will recieve updates.
	// Note: This does not affect the update methods in this class
	//   so they are still callable.
	void set_enabled(bool pEnabled) noexcept;
	// Check if this layer is eligible to recieve updates.
	bool is_enabled() const noexcept;

	// Set the scale for the delta parameters in the update methods.
	void set_time_scale(float pScale) noexcept;
	// Get the scale for the delta parameters in the update methods.
	float get_time_scale() const noexcept;

	void preupdate(float pDelta);
	void update(float pDelta);
	void postupdate(float pDelta);

	void clear();

private:
	template <typename Tcomponent, typename...Tdependencies>
	void for_each_impl(const std::function<void(game_object, Tcomponent&, Tdependencies&...)>& pCallable);

	template <typename Tcomponent, typename...Tdependencies>
	void for_each_impl(const std::function<void(Tcomponent&, Tdependencies&...)>& pCallable);

private:
	const factory* mFactory;
	float mTime_scale{ 1 };
	bool mRecieve_update{ true };
	std::string mName;
	std::vector<std::unique_ptr<system>> mSystems;
	component_manager mComponent_manager;
	object_manager mObject_manager;
};

template<typename T>
inline T* layer::get_system() const
{
	return dynamic_cast<T*>(get_system(T::SYSTEM_ID));
}

template<typename T, typename ...Targs>
inline T* layer::add_unregistered_system(Targs&&...pArgs)
{
	auto ptr = new T(*this, pArgs...);
	mSystems.emplace_back(ptr);
	return ptr;
}

template<typename T>
inline T* layer::add_system()
{
	return static_cast<T*>(add_system(T::SYSTEM_ID));
}

template<typename T>
inline T* layer::add_component(const game_object& pObj)
{
	auto* comp = &mComponent_manager.add_component<T>();
	comp->set_object(pObj);
	mObject_manager.register_component(comp);
	return comp;
}

template<typename T>
inline component_storage<T>& layer::get_component_container()
{
	return mComponent_manager.get_container<T>();
}

template<typename Tcomponent, typename...Tdependencies>
inline void layer::for_each_impl(const std::function<void(game_object, Tcomponent&, Tdependencies&...)>& pCallable)
{
	auto wrapper = [&](Tcomponent& pComp, Tdependencies&...pDep)
	{
		pCallable(get_object(pComp.get_object_id()), pComp, pDep...);
	};
	for_each(std::function(std::move(wrapper)));
}

template<typename Tcomponent, typename...Tdependencies>
inline void layer::for_each_impl(const std::function<void(Tcomponent&, Tdependencies&...)>& pCallable)
{
	for (auto& i : mComponent_manager.get_container<Tcomponent>())
	{
		// Skip unused or disabled
		if (i.is_unused() || !i.is_enabled())
			continue;

		if constexpr (sizeof...(Tdependencies) == 0)
		{
			// No dependencies
			pCallable(i);
		}
		else
		{
			// Retrieve dependencies
			game_object obj = get_object(i.get_object_id());
			std::tuple<Tdependencies*...> dependency_pointers;
			if (obj.unwrap_components(std::get<Tdependencies*>(dependency_pointers)...))
			{
				pCallable(i, *std::get<Tdependencies*>(dependency_pointers)...);
			}
		}
	}
}

template<typename T>
inline void layer::for_each(T&& pCallable)
{
	// Just for simplicity, this will be using std::function to deduce the arguments for us.
	for_each_impl(std::function(std::forward<T>(pCallable)));
}

} // namespace wge::core
