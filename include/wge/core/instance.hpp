#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/object.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/math/transform.hpp>


namespace wge::core
{

struct instantiation_options
{
	std::string name;
	math::transform transform;
	asset_id instantiable_asset_id;
	asset_id creation_script_id;
};

void instantiate_asset(const instantiation_options& pOptions,
	object pObject, const core::asset_manager& pAsset_mgr);


} // namespace wge::core