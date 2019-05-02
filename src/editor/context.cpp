#include "context.hpp"

namespace wge::editor
{

void asset_editor::mark_asset_modified() const
{
	mContext.add_modified_asset(mAsset);
}

void context::add_modified_asset(const core::asset::ptr& pAsset)
{
	if (!pAsset)
		return;
	if (std::find(mUnsaved_assets.begin(), mUnsaved_assets.end(), pAsset) == mUnsaved_assets.end())
		mUnsaved_assets.push_back(pAsset);
}

bool context::is_asset_modified(const core::asset::ptr& pAsset) const
{
	return std::find(mUnsaved_assets.begin(), mUnsaved_assets.end(), pAsset) != mUnsaved_assets.end();
}

bool context::are_there_modified_assets() const noexcept
{
	return !mUnsaved_assets.empty();
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

context::modified_assets& context::get_unsaved_assets() noexcept
{
	return mUnsaved_assets;
}

void context::open_editor(const core::asset::ptr& pAsset)
{
	assert(pAsset);
	if (is_editor_open_for(pAsset))
		return;
	auto iter = mEditor_factories.find(pAsset->get_type());
	if (iter == mEditor_factories.end())
	{
		log::error() << "No editor registered for asset type \"" << pAsset->get_type() << "\"" << log::endm;
		return;
	}
	mAsset_editors.push_back(iter->second(pAsset));
}

void context::close_editor(const core::asset::ptr& pAsset)
{
	
}

void context::show_editor_guis() const
{
	for (const auto& i : mAsset_editors)
		i->on_gui();
}

bool context::is_editor_open_for(const core::asset::ptr& pAsset) const
{
	for (const auto& i : mAsset_editors)
		if (i->get_asset() == pAsset)
			return true;
	return false;
}

} // namespace wge::editor
