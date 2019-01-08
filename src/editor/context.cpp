#include "context.hpp"

namespace wge::editor
{

void context::add_modified_asset(const core::asset::ptr& pAsset)
{
	if (!pAsset)
		return;
	if (std::find(mUnsaved_assets.begin(), mUnsaved_assets.end(), pAsset) == mUnsaved_assets.end())
		mUnsaved_assets.push_back(pAsset);
}

void context::mark_selection_as_modified()
{
	if (auto asset = get_selection<selection_type::asset>())
		add_modified_asset(asset);
}

bool context::is_asset_modified(const core::asset::ptr& pAsset) const
{
	return std::find(mUnsaved_assets.begin(), mUnsaved_assets.end(), pAsset) != mUnsaved_assets.end();
}

void context::save_asset(const core::asset::ptr& pAsset)
{
	auto iter = std::find(mUnsaved_assets.begin(), mUnsaved_assets.end(), pAsset);
	if (iter == mUnsaved_assets.end())
		return;
	(*iter)->save();
	mUnsaved_assets.erase(iter);
}

void context::save_all_assets()
{
	for (auto& i : mUnsaved_assets)
		i->save();
	mUnsaved_assets.clear();
}

context::modified_asset_list& context::get_unsaved_assets() noexcept
{
	return mUnsaved_assets;
}

} // namespace wge::editor
