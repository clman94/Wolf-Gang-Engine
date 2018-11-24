#pragma once

#include <wge/core/object_node.hpp>
#include <wge/core/system.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

namespace wge::core
{

class context;

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

	template <typename T>
	void add_system()
	{
		assert(!get_system(pSystem->get_system_id()));
		mSystems.emplace_back(new T);
	}

	void set_name(const std::string_view& pName)
	{
		mName = pName;
	}

	const std::string& get_name() const
	{
		return mName;
	}

	iterator begin()
	{
		return mObjects.begin();
	}

	iterator end()
	{
		return mObjects.end();
	}

	game_object get(std::size_t pIndex)
	{
		return mObjects[pIndex];
	}

	void add(game_object mObj)
	{
		mObjects.push_back(mObj);
	}

	void remove(game_object mObj)
	{
		auto iter = std::find(mObjects.begin(), mObjects.end(), mObj);
		if (iter != mObjects.end())
		{
			mObjects.erase(iter);
		}
	}

	game_object create_object()
	{
		auto& obj = mObjects.emplace_back();
	}

	template <typename T>
	T* add_component(game_object pObj)
	{
		auto comp = mComponent_manager.add_component<T>();
		comp->get_object(pObj);
		return comp;
	}

	template <typename T>
	T* get_first_component(game_object pObj)
	{
		return mComponent_manager.get_first_component(pObj);
	}

private:
	std::string mName;
	container mObjects;
	std::reference_wrapper<context> mContext;
	std::vector<std::unique_ptr<system>> mSystems;
	component_manager mComponent_manager;
};

} // namespace wge::core
