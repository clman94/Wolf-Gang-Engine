#pragma once

#include <wge/core/instance_id.hpp>
#include <wge/core/layer.hpp>

#include <vector>
#include <map>

namespace wge::core
{

class asset_manager;

class factory
{
public:
	template <typename T>
	void register_component()
	{
		mComponent_factories[T::COMPONENT_ID] =
			[](component_manager& pManager, component_id pId) -> component*
		{
			return &pManager.add_component<T>(pId);
		};
	}

	template <typename T>
	void register_system()
	{
		mSystem_factories[T::SYSTEM_ID] =
			[](const layer::ptr& pLayer) -> system*
		{
			return pLayer->add_system<T>();
		};
	}

	component* create_component(int pType, component_manager& pManager, component_id pId) const
	{
		auto iter = mComponent_factories.find(pType);
		if (iter == mComponent_factories.end())
			return nullptr;
		return iter->second(pManager, pId);
	}

	system* create_system(int pType, const layer::ptr& pLayer) const
	{
		auto iter = mSystem_factories.find(pType);
		if (iter == mSystem_factories.end())
			return nullptr;
		return iter->second(pLayer);
	}

private:
	using component_factory = std::function<component*(component_manager&, component_id)>;
	using system_factory = std::function<system*(const layer::ptr&)>;
	std::map<int, component_factory> mComponent_factories;
	std::map<int, system_factory> mSystem_factories;
};

class context
{
public:
	using layer_container = std::vector<layer::ptr>;

	// Get a layer to a specific index
	layer::ptr get_layer(std::size_t pIndex) const;

	layer::ptr add_layer();
	layer::ptr add_layer(const std::string& pName);
	layer::ptr add_layer(const std::string& pName, std::size_t pInsert);

	void remove_layer(const layer* pPtr);
	void remove_layer(const layer::ptr& pPtr);

	const layer_container& get_layer_container() const noexcept;

	instance_id_t get_unique_instance_id() noexcept;

	void set_asset_manager(asset_manager* pAsset_manager) noexcept;
	asset_manager* get_asset_manager() const noexcept;

	void set_factory(factory* pFactory) noexcept;
	factory* get_factory() const noexcept;

	void preupdate(float pDelta);
	void update(float pDelta);
	void postupdate(float pDelta);

private:
	layer_container mLayers;
	instance_id_t mCurrent_instance_id{ 0 };
	factory* mFactory{ nullptr };
	asset_manager* mAsset_manager{ nullptr };
};

} // namespace wge::core
