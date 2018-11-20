#pragma once

#include <wge/core/system.hpp>
#include <wge/core/object_node.hpp>
#include <wge/core/layer.hpp>

#include <list>

namespace wge::core
{

class context
{
public:
	using layer_container = std::vector<layer::ptr>;

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
	layer::ptr get_layer(std::size_t pIndex) const
	{
		if (pIndex >= mLayers.size())
			return{};
		return mLayers[pIndex];
	}

	layer::ptr create_layer()
	{
		return mLayers.emplace_back(layer::create(*this));
	}

	layer::ptr create_layer(const std::string& pName)
	{
		auto l = create_layer();
		l->set_name(pName);
		return l;
	}

	layer::ptr create_collection(const std::string& pName, std::size_t pInsert)
	{
		mLayers.insert(mLayers.begin() + pInsert, layer::create(*this));
		return mLayers[pInsert];
	}

	const layer_container& get_layer_container() const
	{
		return mLayers;
	}

	// Serializes all collections and their nodes.
	// Note: This does not serialize the factories and systems.
	json serialize() const
	{
		json result;
		for (auto& i : mLayers)
			result["layers"].push_back(i->serialize());
		return result;
	}
	void deserialize(const json& pJson)
	{
		mLayers.clear();
		for (const json& i : pJson["layers"])
			create_layer()->deserialize(i);
	}

private:
	core::component_factory mFactory;
	std::vector<system*> mSystems;
	layer_container mLayers;
};

} // namespace wge::core
