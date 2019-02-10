#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/system.hpp>
#include <wge/core/component_manager.hpp>
#include <wge/core/object_manager.hpp>
#include <wge/core/component_type.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

namespace wge::core
{

class context;

// A layer is a self-contained collection of objects
// with its own set of systems acting upon those objects.
class layer
{
public:
	using ptr = std::shared_ptr<layer>;
	using wptr = std::weak_ptr<layer>;

	// Create a new layer object
	[[nodiscard]] static ptr create(context& pContext)
	{
		return std::make_shared<layer>(pContext);
	}

	layer(context& pContext) noexcept;

	json serialize(serialize_type);
	void deserialize(const json&);

	// Get a system by its type
	template <typename T>
	T* get_system() const;
	// Get a system by its type
	system* get_system(int pID) const;
	// Get a system by name
	system* get_system(const std::string& pName) const;

	// Emplace a new system
	template <typename T, typename...Targs>
	T* add_system(Targs&&...pArgs);

	// Set the name of this layer
	void set_name(const std::string_view& pName) noexcept;
	// Get the name of this layer
	const std::string& get_name() const noexcept;

	// Create a new game object in this layer.
	[[nodiscard]] game_object add_object();
	game_object add_object(const std::string& pName);
	// Remove a game object
	void remove_object(const game_object& mObj);
	// Get a game object at an index
	game_object get_object(std::size_t pIndex);
	// Get a game object by its instance id
	game_object get_object(object_id pId);
	// Get the amount of game objects in this layer
	std::size_t get_object_count() const noexcept;

	// Add a new component to a game object
	template <typename T>
	T* add_component(const game_object& pObj);
	// Add a new component by type
	component* add_component(const game_object& pObj, const component_type& pType);

	// Get the first component of a specific type that refers to this game object
	template <typename T>
	T* get_first_component(const game_object& pObj);
	// Get the first component of a specific type that refers to this game object
	component* get_first_component(const game_object& pObj, const component_type& pType);

	// Get a component by its instance it. The type is needed to check in
	// the correct container.
	component* get_component(const component_type& pType, component_id pId);

	// Get the container associated to a specific type of component.
	template <typename T>
	component_storage<T>& get_component_container();

	// Remove a component of a specific instance id and type.
	void remove_component(int pType, component_id pId);

	// Populate these pointers with all the components this object has.
	// However, it will return false if it couldn't find them all.
	template <typename Tfirst, typename...Trest>
	[[nodiscard]] bool retrieve_components(game_object pObj, Tfirst*& pFirst, Trest*&...pRest);

	// Calls a callable for each component specified by the first parameter.
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
	//    // You can also get the curent game object by adding a `game_object` parameter at the start.
	//    layer.for_each([&](game_object pObject, sprite_component& pTarget) {})
	//
	template <typename T>
	void for_each(T&& pCallable);

	// Get the context.
	context& get_context() const noexcept;

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

private:
	template <typename Tcomponent, typename...Tdependencies>
	void for_each_impl(const std::function<void(game_object, Tcomponent&, Tdependencies&...)>& pCallable);

	template <typename Tcomponent, typename...Tdependencies>
	void for_each_impl(const std::function<void(Tcomponent&, Tdependencies&...)>& pCallable);

private:
	float mTime_scale{ 1 };
	bool mRecieve_update{ true };
	std::string mName;
	std::reference_wrapper<context> mContext;
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
inline T* layer::add_system(Targs&&...pArgs)
{
	auto ptr = new T(*this, pArgs...);
	mSystems.emplace_back(ptr);
	return ptr;
}

template<typename T>
inline T* layer::add_component(const game_object & pObj)
{
	auto* comp = &mComponent_manager.add_component<T>(get_context().get_unique_instance_id());
	comp->set_object(pObj);
	mObject_manager.register_component(comp);
	return comp;
}

template<typename T>
inline T* layer::get_first_component(const game_object & pObj)
{
	return mComponent_manager.get_first_component<T>(pObj.get_instance_id());
}

template<typename T>
inline component_storage<T>& layer::get_component_container()
{
	return mComponent_manager.get_container<T>();
}

template<typename Tfirst, typename...Trest>
inline bool layer::retrieve_components(game_object pObj, Tfirst *& pFirst, Trest *& ...pRest)
{
	auto comp = mComponent_manager.get_first_component<Tfirst>(pObj.get_instance_id());
	if (!comp)
		return false;
	pFirst = comp;
	if constexpr (sizeof...(pRest) == 0)
		return true;
	else
		return retrieve_components(pObj, pRest...);
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
			if (retrieve_components(obj, std::get<Tdependencies*>(dependency_pointers)...))
			{
				pCallable(i, *std::get<Tdependencies*>(dependency_pointers)...);
			}
		}
	}
}

template<typename T>
inline void layer::for_each(T&& pCallable)
{
	for_each_impl(std::function(std::forward<T>(pCallable)));
}

} // namespace wge::core
