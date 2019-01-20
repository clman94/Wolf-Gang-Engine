#pragma once

#include <wge/core/instance_id.hpp>
#include <wge/core/layer.hpp>

#include <vector>
#include <map>

namespace wge::core
{

class asset_manager;

class context
{
public:
	using layer_container = std::vector<layer::ptr>;

	// Get a layer to a specific index
	layer::ptr get_layer(std::size_t pIndex) const;

	layer::ptr create_layer();
	layer::ptr create_layer(const std::string& pName);
	layer::ptr create_layer(const std::string& pName, std::size_t pInsert);

	const layer_container& get_layer_container() const noexcept;

	instance_id_t get_unique_instance_id() noexcept;

	void set_asset_manager(asset_manager* pAsset_manager) noexcept;
	asset_manager* get_asset_manager() const noexcept;

	void preupdate(float pDelta);
	void update(float pDelta);
	void postupdate(float pDelta);

private:
	layer_container mLayers;
	instance_id_t mCurrent_instance_id{ 0 };
	asset_manager* mAsset_manager{ nullptr };
};

} // namespace wge::core
