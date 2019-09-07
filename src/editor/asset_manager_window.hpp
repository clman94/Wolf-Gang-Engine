#include "context.hpp"
#include <wge/core/asset_manager.hpp>

namespace wge::editor
{

class asset_manager_window
{
public:
	asset_manager_window(context& pContext, core::asset_manager& pAsset_manager);

	void on_gui();

private:
	void show_asset_directory_tree(core::asset::ptr& pCurrent_folder, const core::asset::ptr& pAsset = {});
	void asset_tile(const core::asset::ptr& pAsset, const math::vec2& pSize);

private:
	context& mContext;
	core::asset_manager& mAsset_manager;
	core::asset::ptr mSelected_asset;
};

} // namespace wge::editor
