#pragma once

#include <wge/core/object_node.hpp>
#include <wge/core/serializable.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace wge::core
{

class layer :
	public serializable
{
public:
	using container = std::vector<object_node::ref>;
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
		register_property("name", mName);
		register_property("objects",
			[&]() -> json
		{
			return mObjects;
		},
			[&](const json& pJson)
		{
			mObjects.clear();
			for (auto& i : pJson)
			{
				auto obj = object_node::create(mContext);
				obj = i;
				add(obj);
			}
		});
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

	object_node::ref get(std::size_t pIndex)
	{
		return mObjects[pIndex];
	}

	void add(object_node::ref mObj)
	{
		mObjects.push_back(mObj);
	}

	void remove(object_node::ref mObj)
	{
		auto iter = std::find(mObjects.begin(), mObjects.end(), mObj);
		if (iter != mObjects.end())
		{
			mObjects.erase(iter);
		}
	}

	// Sends a message to all objects in this layer
	template<class...Targs>
	void send_all(const std::string& pEvent_name, Targs...pArgs)
	{
		for (auto& i : mObjects)
			i->send_down(pEvent_name, pArgs...);
	}

private:
	std::string mName;
	container mObjects;
	std::reference_wrapper<context> mContext;
};

} // namespace wge::core
