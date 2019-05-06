#pragma once

#include <wge/core/layer.hpp>
#include <wge/core/factory.hpp>

#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <tuple>

namespace wge::core
{

class engine;

class scene
{
public:
	using uptr = std::unique_ptr<scene>;
	using layers = std::vector<layer::uptr>;

	scene(engine& pEngine) :
		mEngine(&pEngine)
	{}

	// Get a layer to a specific index
	[[nodiscard]] layer* get_layer(std::size_t pIndex) const;

	// Creates a layer that is not owned by this scene object.
	[[nodiscard]] layer::uptr create_freestanding_layer();
	layer* add_layer();
	layer* add_layer(const std::string& pName);
	layer* add_layer(const std::string& pName, std::size_t pInsert);
	// Give ownership of a layer to this object.
	layer* add_layer(layer::uptr&);
	// Take ownership of a layer from this object.
	layer::uptr release_layer(const layer&);

	// Returns true if the layer was successfully removed.
	bool remove_layer(const layer& pPtr);

	const layers& get_layer_container() const noexcept;

	void preupdate(float pDelta);
	void update(float pDelta);
	void postupdate(float pDelta);

	void clear();

	engine& get_engine() const noexcept;
	asset_manager& get_asset_manager() noexcept;
	const factory& get_factory() const noexcept;

private:
	layers mLayers;
	engine* mEngine;
};

} // namespace wge::core
