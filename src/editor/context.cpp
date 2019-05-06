#include "context.hpp"

#include <imgui/imgui.h>

namespace wge::editor
{

void asset_editor::mark_asset_modified() const
{
	mContext.add_modified_asset(mAsset);
}

void context::add_modified_asset(const core::asset::ptr& pAsset)
{
	if (pAsset && !is_asset_modified(pAsset))
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
	if (iter != mEditor_factories.end())
	{
		mAsset_editors.push_back(iter->second(pAsset));
	}
	else
	{
		log::error() << "No editor registered for asset type \"" << pAsset->get_type() << "\"" << log::endm;
	}
}

void context::close_editor(const core::asset::ptr& pAsset)
{
	for (std::size_t i = 0; i < mAsset_editors.size(); i++)
		if (mAsset_editors[i]->get_asset() == pAsset)
			mAsset_editors.erase(mAsset_editors.begin() + i);
}

void context::close_all_editors()
{
	mAsset_editors.clear();
}

void context::show_editor_guis()
{
	for (std::size_t i = 0; i < mAsset_editors.size(); i++)
	{
		asset_editor* editor = mAsset_editors[i].get();
		auto& asset = editor->get_asset();

		bool is_window_open = true;
		
		// Construct a title with the id as the stringified asset id so the name can change freely
		// without affecting the window.
		const std::string title = asset->get_path().string() + "##" + asset->get_id().to_string();

		ImGuiWindowFlags flags = is_asset_modified(asset) ? ImGuiWindowFlags_UnsavedDocument : 0;

		if (ImGui::Begin(title.c_str(), &is_window_open, flags))
			editor->on_gui();
		ImGui::End();

		// Close button was pressed so we must delete this editor.
		if (!is_window_open)
			mAsset_editors.erase(mAsset_editors.begin() + i--);
	}
}

bool context::is_editor_open_for(const core::asset::ptr& pAsset) const
{
	for (const auto& i : mAsset_editors)
		if (i->get_asset() == pAsset)
			return true;
	return false;
}

} // namespace wge::editor
