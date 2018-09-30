#pragma once

#include <wge/core/asset_manager.hpp>

namespace wge::graphics
{

class texture_asset_loader :
	public core::asset_loader
{
public:
	virtual core::asset::ptr create_asset(core::asset_config::ptr pConfig, const filesystem::path& mRoot_path) override;
	
	virtual bool can_import(const filesystem::path & pPath) override;
	
	virtual core::asset::ptr import_asset(const filesystem::path & pPath, const filesystem::path& mRoot_path) override;
};

} // namespace wge::graphics
