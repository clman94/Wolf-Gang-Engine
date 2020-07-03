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
	assert(pAsset != nullptr);
	mContext->claim_asset(*this, pAsset->get_id());
	mWindow_str_id = "###" + mAsset->get_id().to_string();
}

asset_editor::~asset_editor()
{
	mContext->release_asset(mAsset->get_id());
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
		ImGui::DockBuilderAddNode(pId);
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
	if (iter != mUnsaved_assets.end())
	{
		if (auto editor = get_editor(*iter))
		{
			editor->on_save();
			if (!editor->is_visible())
			{
				editor->on_close();
				iter->reset();
			}
		}
		mUnsaved_assets.erase(iter);
	}
	pAsset->save();
}

void context::save_all_assets()
{
	for (auto& [id, editor] : mGlobal_editors)
		editor->on_save();
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

	// If it's a secondary asset load the parent's asset editor first.
	if (pAsset->is_secondary_asset())
	{
		assert(pAsset->get_parent_id().is_valid());
		auto parent_asset = mEngine.get_asset_manager().get_asset(pAsset->get_parent_id());
		open_editor(parent_asset);
	}

	// If the editor already exists, set the focus on its window.
	if (auto editor = get_editor(pAsset))
	{
		editor->set_visible(true);
		editor->focus_window();
		return editor;
	}
	if (pAsset->is_primary_asset())
	{
		// Create a new editor for this asset.
		auto iter = mEditor_factories.find(pAsset->get_type());
		if (iter != mEditor_factories.end())
		{
			mGlobal_editors[pAsset->get_id()] = iter->second(pAsset);
			return mGlobal_editors[pAsset->get_id()].get();
		}
		else
		{
			log::error("No editor registered for asset type \"{}\"", pAsset->get_type());
		}
	}

	return nullptr;
}

void context::close_editor(const core::asset::ptr& pAsset)
{
	close_editor(pAsset->get_id());
}

void context::close_editor(const core::asset_id& pId)
{
	mGlobal_editors[pId] = nullptr;
}

void context::close_all_editors()
{
	for (auto [id, editor] : mCurrent_editors)
		editor->hide();
}

bool context::draw_editor(asset_editor& pEditor)
{
	if (!pEditor.is_visible())
		return false;

	auto& asset = pEditor.get_asset();

	// True until the 'X' button on the top right of the
	// window is clicked.
	bool is_window_open = true;

	// Construct a title with the id as the stringified asset id so the name can change freely
	// without affecting the window.
	const std::string title = asset->get_name() + "###" + asset->get_id().to_string();

	ImGuiWindowFlags flags = (is_asset_modified(asset) ? ImGuiWindowFlags_UnsavedDocument : 0);
	if (pEditor.is_first_time())
	{
		if (pEditor.get_dock_family_id() != 0)
		{
			ImGui::SetNextWindowDockID(pEditor.get_dock_family_id());
		}
		ImGui::SetNextWindowSize({ 400, 400 });
		pEditor.mark_first_time();
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
				save_asset(pEditor.get_asset());
			}
		}
		pEditor.on_gui();
	}
	ImGui::End();
	pEditor.set_visible(is_window_open);
	return is_window_open;
}

void context::show_editor_guis()
{
	for (auto& [id, editor] : mGlobal_editors)
	{
		if (editor)
		{
			draw_editor(*editor);
		}
	}
}

bool context::is_editor_open_for(const core::asset::ptr& pAsset) const
{
	return is_editor_open_for(pAsset->get_id());
}

bool context::is_editor_open_for(const core::asset_id& pAsset_id) const
{
	return mCurrent_editors.find(pAsset_id) != mCurrent_editors.end();
}

asset_editor* context::get_editor(const core::asset::ptr& pAsset) const
{
	for (const auto& [id, editor] : mCurrent_editors)
		if (pAsset->get_id() == id)
			return editor;
	return nullptr;
}

asset_editor* context::get_editor(const core::asset_id& pId) const
{
	for (const auto& [id, editor] : mCurrent_editors)
		if (pId == id)
			return editor;
	return nullptr;
}

} // namespace wge::editor
