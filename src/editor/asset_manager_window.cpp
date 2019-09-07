#include "asset_manager_window.hpp"
#include "icon_codepoints.hpp"

#include <wge/core/scene_resource.hpp>
#include <wge/core/object_resource.hpp>
#include <wge/util/ipair.hpp>

#include <imgui/imgui.h>
#include "imgui_ext.hpp"

namespace wge::editor
{

asset_manager_window::asset_manager_window(context& pContext, core::asset_manager& pAsset_manager) :
	mContext(pContext),
	mAsset_manager(pAsset_manager)
{}

void asset_manager_window::on_gui()
{
	if (ImGui::Begin(ICON_FA_FOLDER " Assets", 0, ImGuiWindowFlags_MenuBar))
	{
		static core::asset::ptr current_folder;
		const filesystem::path current_path = mAsset_manager.get_asset_path(current_folder);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("New..."))
			{
				if (ImGui::MenuItem("Folder"))
				{
					mAsset_manager.create_folder(current_path / "new_folder");
				}

				if (ImGui::MenuItem("Object"))
				{
					auto asset = std::make_shared<core::asset>();
					asset->set_name("New_Object");
					asset->set_parent(current_folder);
					asset->set_type("gameobject");
					asset->set_resource(std::make_unique<core::object_resource>());
					mAsset_manager.store_asset(asset);
					asset->save();
					mAsset_manager.add_asset(asset);
				}

				if (ImGui::MenuItem("Scene"))
				{
					auto asset = std::make_shared<core::asset>();
					asset->set_name("New_Scene");
					asset->set_parent(current_folder);
					asset->set_type("scene");
					asset->set_resource(std::make_unique<core::scene_resource>());
					mAsset_manager.store_asset(asset);
					asset->save();
					mAsset_manager.add_asset(asset);
				}

				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Update Asset Directory"))
			{
				mAsset_manager.update_directory_structure();
			}
			ImGui::EndMenuBar();
		}

		static float directory_tree_width = 200;

		ImGui::BeginChild("DirectoryTree", { directory_tree_width, 0 }, true);
		show_asset_directory_tree(current_folder);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::VerticalSplitter("DirectoryTreeSplitter", &directory_tree_width);

		ImGui::SameLine();
		ImGui::BeginGroup();

		ImGui::PushID("PathList");
		for (auto[i, segment] : util::ipair{ current_path })
		{
			const bool last_item = i == current_path.size() - 1;
			if (ImGui::Button(segment.c_str()))
			{
				// TODO: Clicking the last item will open a popup to select a different directory..?
				auto new_path = current_path;
				new_path.erase(new_path.begin() + i, new_path.end());
				current_folder = mAsset_manager.get_asset(new_path);
				break;
			}
			ImGui::SameLine();
		}
		ImGui::NewLine();

		ImGui::PopID();

		const math::vec2 file_preview_size = { 100, 100 };

		ImGui::BeginChild("FileList", { 0, 0 }, true);
		mAsset_manager.for_each_child(current_folder, [&](auto& i)
		{
			// Skip folders
			if (i->get_type() != "folder")
			{
				asset_tile(i, file_preview_size);

				// If there isn't any room left in this line, create a new one.
				if (ImGui::GetContentRegionAvailWidth() < file_preview_size.x)
					ImGui::NewLine();
			}
		});
		ImGui::EndChild();

		ImGui::EndGroup();
	}
	ImGui::End();
}

void asset_manager_window::show_asset_directory_tree(core::asset::ptr& pCurrent_folder, const core::asset::ptr& pAsset)
{
	const bool is_root = !pAsset;
	bool open = false;

	const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	const char* name = is_root ? "Assets" : pAsset->get_name().c_str();
	if (pAsset && !mAsset_manager.has_subfolders(pAsset))
		ImGui::TreeNodeEx(name, flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
	else
		open = ImGui::TreeNodeEx(name, flags);

	// Set the directory
	if (ImGui::IsItemClicked())
	{
		if (is_root)
			pCurrent_folder = core::asset::ptr{};
		else
			pCurrent_folder = pAsset;
	}

	if (open)
	{
		// Show subdirectories
		mAsset_manager.for_each_child(pAsset, [&](auto& i)
		{
			if (i->get_type() == "folder")
				show_asset_directory_tree(pCurrent_folder, i);
		});

		ImGui::TreePop();
	}
}
void asset_manager_window::asset_tile(const core::asset::ptr & pAsset, const math::vec2 & pSize)
{
	ImGui::PushID(&*pAsset);

	const bool is_asset_selected = mSelected_asset == pAsset;

	ImGui::BeginGroup();

	// Draw preview
	if (pAsset->get_type() == "texture")
		ImGui::ImageButton(pAsset,
			pSize - math::vec2(ImGui::GetStyle().FramePadding) * 2);
	else
		ImGui::Button("No preview", pSize);

	ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1), pAsset->get_type().c_str());

	// Draw text.
	ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + pSize.x);
	ImGui::Text(pAsset->get_name().c_str());
	ImGui::PopTextWrapPos();

	ImGui::EndGroup();
	ImGui::SameLine();

	// Allow asset to be dragged.
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		ImGui::SetDragDropPayload((pAsset->get_type() + "Asset").c_str(), &pAsset->get_id(), sizeof(util::uuid));
		ImGui::Text("Asset: %s", mAsset_manager.get_asset_path(pAsset).string().c_str());
		ImGui::EndDragDropSource();
	}

	// Select the asset when clicked.
	if (ImGui::IsItemClicked())
		mSelected_asset = pAsset;

	// Open it if its double clicked.
	if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
		mContext.open_editor(mSelected_asset);


	// Context menu when right clicked
	if (ImGui::IsItemClicked(1))
	{
		// We still want the asset to be selected when we right click it.
		mSelected_asset = pAsset;
		ImGui::OpenPopup("AssetContextMenu");
	}
	if (ImGui::BeginPopup("AssetContextMenu",
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
	{
		// Delete asset. (Undoable atm)
		if (ImGui::MenuItem("Delete"))
		{
			mContext.close_editor(mSelected_asset);
			mAsset_manager.remove_asset(mSelected_asset);
			mSelected_asset.reset();
		}

		ImGui::EndPopup();
	}

	auto dl = ImGui::GetWindowDrawList();

	// Calculate item aabb that includes the item spacing.
	// We will use these to render the box around the preview and
	// the text.
	math::vec2 item_min = math::vec2(ImGui::GetItemRectMin())
		- math::vec2(ImGui::GetStyle().ItemSpacing) / 2;
	math::vec2 item_max = math::vec2(ImGui::GetItemRectMax())
		+ math::vec2(ImGui::GetStyle().ItemSpacing) / 2;

	// Draw the background
	if (ImGui::IsItemHovered())
	{
		dl->AddRectFilled(item_min, item_max,
			ImGui::GetColorU32(ImGuiCol_ButtonHovered), ImGui::GetStyle().FrameRounding);
	}
	else if (is_asset_selected)
	{
		dl->AddRectFilled(item_min, item_max,
			ImGui::GetColorU32(ImGuiCol_ButtonActive), ImGui::GetStyle().FrameRounding);
	}

	ImGui::PopID();
}

} // namespace wge::editor
