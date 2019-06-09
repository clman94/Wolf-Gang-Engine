#include "context.hpp"

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>
#include <wge/util/ipair.hpp>

namespace wge::editor
{

void asset_editor::focus_window() const noexcept
{
	ImGui::SetWindowFocus(("###" + mAsset->get_id().to_string()).c_str());
}

void asset_editor::mark_asset_modified() const
{
	mContext->add_modified_asset(mAsset);
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

asset_editor* context::open_editor(const core::asset::ptr& pAsset)
{
	assert(pAsset);

	// If the editor already exists, set the focus on its window.
	if (auto editor = get_editor(pAsset))
	{
		editor->focus_window();
		return editor;
	}

	// Create a new editor for this asset.
	auto iter = mEditor_factories.find(pAsset->get_type());
	if (iter != mEditor_factories.end())
	{
		mAsset_editors.push_back(iter->second(pAsset));
		return mAsset_editors.back().get();
	}
	else
	{
		log::error() << "No editor registered for asset type \"" << pAsset->get_type() << "\"" << log::endm;
	}

	return nullptr;
}

void context::close_editor(const core::asset::ptr& pAsset)
{
	for (auto iter = mAsset_editors.begin(); iter != mAsset_editors.end(); ++iter)
	{
		if (!*iter)
			continue;
		asset_editor* editor = iter->get();
		if (editor->get_asset() == pAsset)
		{
			editor->on_close();
			iter->reset();
			break;
		}
	}
}

void context::close_editor(const util::uuid& pIds)
{
	for (auto iter = mAsset_editors.begin(); iter != mAsset_editors.end(); ++iter)
	{
		if (!*iter)
			continue;
		asset_editor* editor = iter->get();
		if (editor->get_asset()->get_id() == pIds)
		{
			editor->on_close();
			iter->reset();
			break;
		}
	}
}

void context::close_all_editors()
{
	for (const auto& i : mAsset_editors)
		i->on_close();
	mAsset_editors.clear();
}

void context::show_editor_guis()
{
	for (auto iter = mAsset_editors.begin(); iter != mAsset_editors.end(); ++iter)
	{
		if (!*iter)
			continue;

		asset_editor* editor = iter->get();
		auto& asset = editor->get_asset();

		bool is_window_open = true;
		
		// Construct a title with the id as the stringified asset id so the name can change freely
		// without affecting the window.
		const std::string title = asset->get_name() + "###" + asset->get_id().to_string();

		ImGuiWindowFlags flags = is_asset_modified(asset) ? ImGuiWindowFlags_UnsavedDocument : 0;

		if (editor->get_dock_family_id() != 0)
		{
			ImGuiDockFamily family(editor->get_dock_family_id());
			ImGui::SetNextWindowDockFamily(&family);
		}
		if (ImGui::Begin(title.c_str(), &is_window_open, flags))
		{
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
			{
				ImGuiIO& io = ImGui::GetIO();
				bool ctrl = io.KeyCtrl;
				bool alt = io.KeyAlt;
				if (ctrl && !alt && ImGui::IsKeyPressed(GLFW_KEY_S))
				{
					save_asset(editor->get_asset());
				}
			}
			editor->on_gui();
		}
		ImGui::End();

		// Close button was pressed so we must delete this editor.
		if (!is_window_open)
		{
			editor->on_close();
			iter->reset();
		}
	}

	erase_deleted_editors();
}

bool context::is_editor_open_for(const core::asset::ptr& pAsset) const
{
	for (const auto& i : mAsset_editors)
		if (i && i->get_asset() == pAsset)
			return true;
	return false;
}

bool context::is_editor_open_for(const util::uuid & pAsset_id) const
{
	for (const auto& i : mAsset_editors)
		if (i && i->get_asset()->get_id() == pAsset_id)
			return true;
	return false;
}

asset_editor* context::get_editor(const core::asset::ptr & pAsset) const
{
	for (const auto& i : mAsset_editors)
		if (i->get_asset() == pAsset)
			return i.get();
	return nullptr;
}

void context::erase_deleted_editors()
{
	auto iter = mAsset_editors.begin();
	while (iter != mAsset_editors.end())
	{
		if (*iter)
			++iter;
		else
			mAsset_editors.erase(iter++);
	}
}

} // namespace wge::editor
