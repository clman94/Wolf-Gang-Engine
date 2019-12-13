#include "context.hpp"

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>
#include <wge/util/ipair.hpp>

#include <imgui/imgui_internal.h> // For dock builder functions

namespace wge::editor
{

asset_editor::asset_editor(context& pContext, const core::asset::ptr& pAsset) noexcept :
	mContext(&pContext),
	mAsset(pAsset)
{
	mWindow_str_id = "###" + mAsset->get_id().to_string();
}
void asset_editor::focus_window() const noexcept
{
	ImGui::SetWindowFocus(mWindow_str_id.c_str());
}

void asset_editor::mark_asset_modified() const
{
	mContext->add_modified_asset(mAsset);
}

void asset_editor::set_dock(unsigned int pId)
{
	assert(mAsset);
	if (!ImGui::DockBuilderGetNode(pId))
		ImGui::DockBuilderAddNode(pId, ImVec2(0, 0));
	ImGui::DockBuilderDockWindow(mWindow_str_id.c_str(), pId);
	ImGui::DockBuilderFinish(pId);
}

const std::string& asset_editor::get_window_str_id() const noexcept
{
	return mWindow_str_id;
}

core::asset_manager& asset_editor::get_asset_manager() const noexcept
{
	return mContext->get_engine().get_asset_manager();
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
	if (auto editor = get_editor(*iter))
	{
		editor->on_save();
		if (!editor->is_visible())
		{
			editor->on_close();
			iter->reset();
		}
	}
	(*iter)->save();
	mUnsaved_assets.erase(iter);
}

void context::save_all_assets()
{
	for (auto& i : mAsset_editors)
		i->on_save();
	for (auto& i : mUnsaved_assets)
		i->save();
	mUnsaved_assets.clear();
}

context::modified_assets& context::get_unsaved_assets() noexcept
{
	return mUnsaved_assets;
}

asset_editor* context::open_editor(const core::asset::ptr& pAsset, unsigned int pDock_id)
{
	assert(pAsset);

	// If the editor already exists, set the focus on its window.
	if (auto editor = get_editor(pAsset))
	{
		editor->set_visible(true);
		editor->focus_window();
		return editor;
	}

	// Create a new editor for this asset.
	auto iter = mEditor_factories.find(pAsset->get_type());
	if (iter != mEditor_factories.end())
	{
		mAsset_editors.push_back(iter->second(pAsset));
		asset_editor* editor = mAsset_editors.back().get();
		if (pDock_id == 0)
		{
			editor->set_dock(mDefault_dock_id);
		}
		else
		{
			editor->set_dock(pDock_id);
			editor->set_dock_family_id(pDock_id);
		}
		
		return editor;
	}
	else
	{
		log::error("No editor registered for asset type \"{}\"", pAsset->get_type());
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
	mAsset_editors.clear();
}

void context::show_editor_guis()
{
	for (auto iter = mAsset_editors.begin(); iter != mAsset_editors.end(); ++iter)
	{
		if (!*iter)
			continue;

		asset_editor* editor = iter->get();
		if (!editor->is_visible())
			continue;

		auto& asset = editor->get_asset();

		// True until the 'X' button on the top right of the
		// window is clicked.
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

		if (!is_window_open)
		{
			if (is_asset_modified(editor->get_asset()))
				editor->set_visible(false);
			else
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

bool context::is_editor_open_for(const util::uuid& pAsset_id) const
{
	for (const auto& i : mAsset_editors)
		if (i && i->get_asset()->get_id() == pAsset_id &&
			i->is_visible())
			return true;
	return false;
}

asset_editor* context::get_editor(const core::asset::ptr& pAsset) const
{
	for (const auto& i : mAsset_editors)
		if (i && i->get_asset() == pAsset)
			return i.get();
	return nullptr;
}

asset_editor * context::get_editor(const util::uuid& pId) const
{
	for (const auto& i : mAsset_editors)
		if (i && i->get_asset()->get_id() == pId)
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
