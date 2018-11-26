#pragma once

#include <wge/core/object_node.hpp>
#include <wge/core/system.hpp>
#include <wge/core/component_manager.hpp>

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
	using container = std::vector<game_object>;
	using iterator = container::iterator;
	using const_iterator = container::const_iterator;

	using ptr = std::shared_ptr<layer>;
	using wptr = std::weak_ptr<layer>;

	static ptr create(context& pContext)
	{
		return std::make_shared<layer>(pContext);
	}

	layer(context& pContext) :
		mContext(pContext)
	{
	}

	template <typename T>
	T* get_system() const
	{
		return dynamic_cast<T*>(get_system(T::SYSTEM_ID));
	}

	system* get_system(int pID) const
	{
		for (auto& i : mSystems)
			if (i->get_system_id() == pID)
				return i.get();
		return nullptr;
	}
	system* get_system(const std::string& pName) const
	{
		for (auto& i : mSystems)
			if (i->get_system_name() == pName)
				return i.get();
		return nullptr;
	}

	template <typename T, typename...Targs>
	void add_system(Targs&&...pArgs)
	{
		mSystems.emplace_back(new T(*this, pArgs...));
	}

	void set_name(const std::string_view& pName)
	{
		mName = pName;
	}

	const std::string& get_name() const
	{
		return mName;
	}

	void remove(const game_object& mObj)
	{
		for (std::size_t i = 0; i < mObjects.size(); i++)
		{
			if (mObjects[i].id == mObj.get_instance_id())
			{
				mComponent_manager.remove_entity(mObj.get_instance_id());
				mObjects.erase(mObjects.begin() + i);
				return;
			}
		}
	}

	game_object create_object();
	game_object get_object(std::size_t pIndex);
	game_object get_object(instance_id pId);
	std::size_t get_object_count() const;
	std::string get_object_name(const game_object& mObj);

	template <typename T>
	T* add_component(const game_object& pObj)
	{
		auto* comp = &mComponent_manager.add_component<T>();
		comp->set_object(pObj);
		return comp;
	}

	template <typename T>
	T* get_first_component(const game_object& pObj)
	{
		return mComponent_manager.get_first_component<T>(pObj.get_instance_id());
	}

	template <typename T>
	component_storage<T>& get_component_container()
	{
		return mComponent_manager.get_container<T>();
	}

	// Populate these pointers with all the components this object has.
	// However, it will return false if it couldn't find them all.
	template <typename Tfirst, typename...Trest>
	bool retrieve_components(game_object pObj, Tfirst*& pFirst, Trest*&...pRest)
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

	context& get_context() const
	{
		return mContext;
	}

	void set_enabled(bool pEnabled)
	{
		mRecieve_update = pEnabled;
	}

	bool is_enabled() const
	{
		return mRecieve_update;
	}

	float get_time_scale() const
	{
		return mTime_scale;
	}

	void set_time_scale(float pScale)
	{
		mTime_scale = pScale;
	}

	void preupdate(float pDelta)
	{
		pDelta *= mTime_scale;
		for (auto& i : mSystems)
			i->preupdate(pDelta);
	}

	void update(float pDelta)
	{
		pDelta *= mTime_scale;
		for (auto& i : mSystems)
			i->update(pDelta);
	}

	void postupdate(float pDelta)
	{
		pDelta *= mTime_scale;
		for (auto& i : mSystems)
			i->postupdate(pDelta);
	}

private:
	struct object_data
	{
		std::string name;
		instance_id id;
	};

private:
	float mTime_scale{ 1 };
	bool mRecieve_update{ true };
	std::string mName;
	std::vector<object_data> mObjects;
	std::reference_wrapper<context> mContext;
	std::vector<std::unique_ptr<system>> mSystems;
	component_manager mComponent_manager;
};

} // namespace wge::core
