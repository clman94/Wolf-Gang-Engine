#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/system.hpp>
#include <wge/core/component_manager.hpp>
#include <wge/core/object_manager.hpp>

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

	static ptr create(context& pContext)
	{
		return std::make_shared<layer>(pContext);
	}

	layer(context& pContext);

	template <typename T>
	T* get_system() const
	{
		return dynamic_cast<T*>(get_system(T::SYSTEM_ID));
	}

	system* get_system(int pID) const;
	system* get_system(const std::string& pName) const;

	template <typename T, typename...Targs>
	void add_system(Targs&&...pArgs)
	{
		mSystems.emplace_back(new T(*this, pArgs...));
	}

	void set_name(const std::string_view& pName);
	const std::string& get_name() const;

	game_object create_object();
	void remove_object(const game_object& mObj);
	game_object get_object(std::size_t pIndex);
	game_object get_object(object_id pId);
	std::size_t get_object_count() const;

	template <typename T>
	T* add_component(const game_object& pObj)
	{
		auto* comp = &mComponent_manager.add_component<T>(get_context().get_unique_instance_id());
		comp->set_object(pObj);
		mObject_manager.register_component(comp);
		return comp;
	}

	template <typename T>
	T* get_first_component(const game_object& pObj)
	{
		return mComponent_manager.get_first_component<T>(pObj.get_instance_id());
	}

	component* get_first_component(const game_object& pObj, int pType)
	{
		return mComponent_manager.get_first_component(pType, pObj.get_instance_id());
	}

	component* get_component(int pType, component_id pId)
	{
		return mComponent_manager.get_component(pType, pId);
	}

	template <typename T>
	component_storage<T>& get_component_container()
	{
		return mComponent_manager.get_container<T>();
	}

	void remove_component(int pType, component_id pId)
	{
		component* comp = get_component(pType, pId);
		object_id obj_id = comp->get_object_id();
		mObject_manager.unregister_component(obj_id, pId);
		mComponent_manager.remove_component(pType, pId);
	}

	// Populate these pointers with all the components this object has.
	// However, it will return false if it couldn't find them all.
	template <typename Tfirst, typename...Trest>
	bool retrieve_components(game_object pObj, Tfirst*& pFirst, Trest*&...pRest);

	template <typename T>
	void for_each(T&& pCallable);

	context& get_context() const;

	void set_enabled(bool pEnabled);
	bool is_enabled() const;

	float get_time_scale() const;
	void set_time_scale(float pScale);

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

template<typename Tfirst, typename ...Trest>
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
	for_each(std::function(wrapper));
}

template<typename Tcomponent, typename...Tdependencies>
inline void layer::for_each_impl(const std::function<void(Tcomponent&, Tdependencies&...)>& pCallable)
{
	std::tuple<Tdependencies*...> dependency_pointers;
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
