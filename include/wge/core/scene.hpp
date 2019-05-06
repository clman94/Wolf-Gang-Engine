#pragma once

#include <wge/core/layer.hpp>
#include <wge/filesystem/file_input_stream.hpp>

#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <tuple>

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

	template <typename T, typename...Targs>
	void register_system(Targs&&...pExtra_args)
	{
		mSystem_factories[T::SYSTEM_ID] =
			[pExtra_args = std::tuple<Targs...>(std::forward<Targs>(pExtra_args)...)](layer& pLayer)
				-> system::uptr
		{
			auto args = std::tuple_cat(std::tie(pLayer), pExtra_args);
			auto make_unique_wrapper = [](auto&...pArgs) { return std::make_unique<T>(pArgs...); };
			return std::apply(make_unique_wrapper, args);
		};
	}

	component* create_component(const component_type& pType, component_manager& pManager) const;
	system::uptr create_system(int pType, layer& pLayer) const;

private:
	using component_factory = std::function<component*(component_manager&)>;
	using system_factory = std::function<system::uptr(layer&)>;
	std::map<component_type, component_factory> mComponent_factories;
	std::map<int, system_factory> mSystem_factories;
};

class engine;

class scene
{
public:
	using layers = std::vector<layer::ptr>;

	scene(engine& pEngine) :
		mEngine(pEngine)
	{}

	// Get a layer to a specific index
	layer::ptr get_layer(std::size_t pIndex) const;

	// Creates a layer that references this context but is not
	// managed by it. Used by the editor.
	[[nodiscard]] layer::ptr create_freestanding_layer();
	layer::ptr add_layer();
	layer::ptr add_layer(const std::string& pName);
	layer::ptr add_layer(const std::string& pName, std::size_t pInsert);

	void remove_layer(const layer* pPtr);
	void remove_layer(const layer::ptr& pPtr);

	const layers& get_layer_container() const noexcept;

	void preupdate(float pDelta);
	void update(float pDelta);
	void postupdate(float pDelta);

	void clear();

	engine& get_engine() const noexcept
	{
		return mEngine;
	}
	asset_manager& get_asset_manager() noexcept;
	const factory& get_factory() const noexcept;

private:
	layers mLayers;
	engine& mEngine;
};

} // namespace wge::core
