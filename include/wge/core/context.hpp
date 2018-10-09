#pragma once

#include <wge/core/system.hpp>
#include <wge/core/object_node.hpp>

namespace wge::core
{

class context
{
public:
	using collection_container = std::vector<core::object_node::ref>;

	core::component_factory& get_component_factory()
	{
		return mFactory;
	}

	template <typename T>
	T* get_system() const
	{
		return dynamic_cast<T*>(get_system(T::SYSTEM_ID));
	}

	system* get_system(int pID) const
	{
		for (auto i : mSystems)
			if (i->get_system_id() == pID)
				return i;
		return nullptr;
	}
	system* get_system(const std::string& pName) const
	{
		for (auto i : mSystems)
			if (i->get_system_name() == pName)
				return i;
		return nullptr;
	}

	void add_system(system* pSystem)
	{
		assert(!get_system(pSystem->get_system_id()));
		mSystems.push_back(pSystem);
	}

	// Get the root node of a collection.
	// The root node is where all system-specific components should go.
	core::object_node::ref get_collection(std::size_t pIndex) const
	{
		if (pIndex >= mCollections.size())
			return{};
		return mCollections[pIndex];
	}

	core::object_node::ref create_collection()
	{
		mCollections.push_back(core::object_node::create(this));
		return mCollections.back();
	}

	core::object_node::ref create_collection(const std::string& pName)
	{
		auto c = create_collection();
		c->set_name(pName);
		return c;
	}

	core::object_node::ref create_collection(const std::string& pName, std::size_t pInsert)
	{
		mCollections.insert(mCollections.begin() + pInsert, core::object_node::create(this));
		return mCollections[pInsert];
	}

	const collection_container& get_collection_container() const
	{
		return mCollections;
	}

	// Serializes all collections and their nodes.
	// Note: This does not serialize the factories and systems.
	json serialize() const
	{
		json result;
		for (auto& i : mCollections)
			result["collections"].push_back(i->serialize());
	}
	void deserialize(const json& pJson)
	{
		mCollections.clear();
		for (const json& i : pJson["collections"])
			create_collection()->deserialize(i);
	}

private:
	core::component_factory mFactory;
	std::vector<system*> mSystems;
	std::vector<core::object_node::ref> mCollections;
};

}