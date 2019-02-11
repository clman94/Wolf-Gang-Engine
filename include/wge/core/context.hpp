#pragma once

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
			[](component_manager& pManager) -> component*
		{
			return &pManager.add_component<T>();
		};
	}

	template <typename T>
	void register_system()
	{
		mSystem_factories[T::SYSTEM_ID] =
			[](layer& pLayer) -> system*
		{
			return pLayer.add_system<T>();
		};
	}

	component* create_component(const component_type& pType, component_manager& pManager) const;
	system* create_system(int pType, layer& pLayer) const;

private:
	using component_factory = std::function<component*(component_manager&)>;
	using system_factory = std::function<system*(layer&)>;
	std::map<component_type, component_factory> mComponent_factories;
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

	void set_asset_manager(asset_manager* pAsset_manager) noexcept;
	asset_manager* get_asset_manager() const noexcept;

	void set_factory(factory* pFactory) noexcept;
	factory* get_factory() const noexcept;

	void preupdate(float pDelta);
	void update(float pDelta);
	void postupdate(float pDelta);

private:
	layer_container mLayers;
	factory* mFactory{ nullptr };
	asset_manager* mAsset_manager{ nullptr };
};

} // namespace wge::core
