#include "asset_manager_window.hpp"
#include "icon_codepoints.hpp"

#include <wge/core/scene_resource.hpp>
#include <wge/core/object_resource.hpp>
#include <wge/graphics/sprite.hpp>
#include <wge/graphics/tileset.hpp>
#include <wge/util/ipair.hpp>

#include <imgui/imgui.h>
#include "imgui_ext.hpp"
#include "widgets.hpp"

namespace wge::editor
{

asset_manager_window::asset_manager_window(context& pContext, core::asset_manager& pAsset_manager) :
	mContext(pContext),
	mAsset_manager(pAsset_manager)
{}

void asset_manager_window::on_gui()
{
	if (ImGui::Begin((const char*)(ICON_FA_FOLDER u8" Assets"), 0, ImGuiWindowFlags_MenuBar))
	{
		const filesystem::path current_path = mAsset_manager.get_asset_path(mCurrent_folder);

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
					asset->set_parent(mCurrent_folder);
					asset->set_type("object");
					asset->set_resource(std::make_unique<core::object_resource>());
					mAsset_manager.store_asset(asset);
					asset->save();
					mAsset_manager.add_asset(asset);
				}

				if (ImGui::MenuItem("Scene"))
				{
					auto asset = std::make_shared<core::asset>();
					asset->set_name("New_Scene");
					asset->set_parent(mCurrent_folder);
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
		show_asset_directory_tree(mCurrent_folder);
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::VerticalSplitter("DirectoryTreeSplitter", &directory_tree_width);

		ImGui::SameLine();
		ImGui::BeginGroup();

		ImGui::PushID("PathList");
		for (auto[i, segment] : util::enumerate{ current_path })
		{
			const bool last_item = i == current_path.size() - 1;
			if (ImGui::Button(segment.c_str()))
			{
				// TODO: Clicking the last item will open a popup to select a different directory..?
				auto new_path = current_path;
				new_path.erase(new_path.begin() + i, new_path.end());
				mCurrent_folder = mAsset_manager.get_asset(new_path);
				break;
			}
			ImGui::SameLine();
		}
		ImGui::NewLine();

		ImGui::PopID();

		const math::vec2 file_preview_size = { 100, 100 };

		ImGui::BeginChild("FileList", { 0, 0 }, true);
		
		mAsset_manager.for_each_child(mCurrent_folder, [&](auto& i)
		{
			// Only folders
			if (i->get_type() == "folder")
			{
				asset_tile(i, file_preview_size);

				// If there isn't any room left in this line, create a new one.
				if (ImGui::GetContentRegionAvailWidth() < file_preview_size.x)
					ImGui::NewLine();
			}
		});
		ImGui::NewLine();
		mAsset_manager.for_each_child(mCurrent_folder, [&](auto& i)
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

		remove_queued_assets();
	}
	ImGui::End();
}

void asset_manager_window::show_asset_directory_tree(core::asset::ptr& pCurrent_folder, const core::asset::ptr& pAsset)
{
	const bool is_root = !pAsset;
	bool open = false;

	const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | 
		(is_selected(pAsset) ? ImGuiTreeNodeFlags_Selected : 0);
	const char* name = is_root ? "Assets" : pAsset->get_name().c_str();
	if (pAsset && !mAsset_manager.has_subfolders(pAsset))
		ImGui::TreeNodeEx(name, flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
	else
		open = ImGui::TreeNodeEx(name, flags);

	if (pAsset)
	{
		ImGui::PushID("FolderId");
		ImGui::PushID(&*pAsset);
		asset_context_menu();
		ImGui::PopID();
		ImGui::PopID();
	}

	folder_dragdrop_target(pAsset);

	// Set the directory
	if (ImGui::IsItemClicked() || ImGui::IsItemClicked(1))
	{
		pCurrent_folder = pAsset;
		select(pAsset);
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

void asset_manager_window::asset_tile(const core::asset::ptr& pAsset, const math::vec2& pSize)
{
	ImGui::PushID(&*pAsset);

	ImGui::BeginGroup();

	const graphics::texture* preview_texture = nullptr;

	// Draw preview
	if (pAsset->get_type() == "sprite")
	{
		auto sprite = pAsset->get_resource<graphics::sprite>();
		assert(sprite);
		preview_texture = &sprite->get_texture();
	}
	else if (pAsset->get_type() == "tileset")
	{
		auto tileset = pAsset->get_resource<graphics::tileset>();
		assert(tileset);
		preview_texture = &tileset->get_texture();
	}
	else if (pAsset->get_type() == "object")
	{
		auto object_res = pAsset->get_resource<core::object_resource>();
		assert(object_res);
		if (object_res->display_sprite.is_valid())
		{
			auto sprite = mAsset_manager.get_resource<graphics::sprite>(object_res->display_sprite);
			assert(sprite);
			preview_texture = &sprite->get_texture();
		}
	}

	ImGui::BeginGroup();

	if (pAsset->get_type() == "folder")
	{
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + pSize.x);
		ImGui::Text(fmt::format("{} {}", (const char*)ICON_FA_FOLDER, pAsset->get_name()).c_str());
		ImGui::PopTextWrapPos();
	}
	else
	{
		if (preview_texture)
		{
			preview_image("Preview", *preview_texture, pSize - math::vec2(ImGui::GetStyle().FramePadding) * 2);
		}
		else
		{
			ImGui::Dummy(pSize);
		}

		ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1), pAsset->get_type().c_str());

		// Draw text.
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + pSize.x);
		ImGui::Text(pAsset->get_name().c_str());
		ImGui::PopTextWrapPos();
	}

	ImGui::EndGroup();

	auto last_cursor_position = ImGui::GetCursorScreenPos();
	ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
	ImGui::InvisibleButton("InteractiveButton", ImGui::GetItemRectSize());
	ImGui::SetCursorScreenPos(last_cursor_position);

	ImGui::EndGroup();
	ImGui::SameLine();

	// Allow asset to be dragged.
	if (ImGui::IsItemActive())
	{
		if (is_singlular_selection() && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload((pAsset->get_type() + "Asset").c_str(), &pAsset->get_id(), sizeof(util::uuid));
			ImGui::Text("Asset: %s", mAsset_manager.get_asset_path(pAsset).string().c_str());
			ImGui::EndDragDropSource();
		}
		else if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload("AssetMultiSelect", nullptr, 0);
			ImGui::Text(fmt::format("{} Asset(s)", mSelected_asset.size()).c_str());
			ImGui::EndDragDropSource();
		}
	}

	if (pAsset->get_type() == "folder")
	{
		folder_dragdrop_target(pAsset);
	}

	// Select the asset when clicked.
	// Doing some trickery here so we dont deselect everything when we want to drag multiple items.
	{
		bool clicked = ImGui::IsItemClicked() || ImGui::IsItemClicked(1);
		bool released = ImGui::IsItemHovered() && ImGui::IsMouseReleased(0);
		if ((is_multi_selection() && released) || (mSelected_asset.size() <= 1 && clicked))
			select(pAsset);
	}

	// Open it if its double clicked.
	if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
	{
		if (pAsset->get_type() == "folder")
		{
			mCurrent_folder = pAsset;
		}
		else
		{
			mContext.open_editor(pAsset);
		}
	}

	ImGui::PushID(&*pAsset);
	asset_context_menu();
	ImGui::PopID();

	auto dl = ImGui::GetWindowDrawList();

	if (pAsset->get_type() != "folder")
	{
		const char* icon = (const char*)ICON_FA_EXCLAMATION_TRIANGLE;
		if (pAsset->get_type() == "object")
			icon = (const char*)ICON_FA_CODE;
		else if (pAsset->get_type() == "sprite")
			icon = (const char*)ICON_FA_FILE_IMAGE_O;
		else if (pAsset->get_type() == "tileset")
			icon = (const char*)ICON_FA_FILE_IMAGE_O;
		else if (pAsset->get_type() == "scene")
			icon = (const char*)ICON_FA_GAMEPAD;
		dl->AddRectFilled(ImGui::GetItemRectMin(),
			ImGui::GetItemRectMin() + ImGui::CalcTextSize(icon) + ImGui::GetStyle().ItemInnerSpacing * 2,
			ImGui::GetColorU32(ImVec4(0, 0, 0, 0.5f)), 5);
		dl->AddText(ImGui::GetItemRectMin() + ImGui::GetStyle().ItemInnerSpacing, ImGui::GetColorU32(ImGuiCol_Text), icon);
	}

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
		dl->AddRect(item_min, item_max,
			ImGui::GetColorU32(ImGuiCol_ButtonHovered), ImGui::GetStyle().FrameRounding, ImDrawCornerFlags_All, 2);
	}

	if (is_selected(pAsset))
	{

		dl->AddRect(item_min, item_max,
			ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonHovered : ImGuiCol_ButtonActive), ImGui::GetStyle().FrameRounding, ImDrawCornerFlags_All, 4);
	}

	ImGui::PopID();
}

void asset_manager_window::asset_context_menu()
{
	if (mSelected_asset.empty())
		return;
	ImGui::PushID(&*mSelected_asset.back());
	if (ImGui::BeginPopupContextWindow("AssetContextMenu"))
	{
		if (is_singlular_selection())
		{
			const core::asset::ptr selection = mSelected_asset.back();
			static std::string new_name;
			if (ImGui::IsWindowAppearing())
			{
				new_name = selection->get_name();
			}
			// Rename asset.
			ImGui::InputText("Name", &new_name);
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (!mAsset_manager.rename_asset(selection, new_name))
				{
					new_name = selection->get_name();
					mContext.save_asset(selection);
					log::error("Failed to rename asset from {} to {}", selection->get_name(), new_name);
				}
			}

			// Delete asset. (Undoable atm)
			if (ImGui::MenuItem("Delete"))
			{
				delete_selected();
			}
		}
		else
		{
			if (ImGui::MenuItem("Delete All"))
			{
				delete_selected();
			}

		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void asset_manager_window::folder_dragdrop_target(const core::asset::ptr& pAsset)
{
	if (ImGui::BeginDragDropTarget())
	{
		if (ImGui::AcceptDragDropPayload("AssetMultiSelect"))
		{
			for (auto i : mSelected_asset)
				mAsset_manager.move_asset(i, pAsset);
			deselect();
		}
		else if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
		{
			std::string_view datatype(payload->DataType);
			bool is_asset = datatype.find_first_of("Asset") != static_cast<size_t>(-1);
			if (is_asset && ImGui::AcceptDragDropPayload(payload->DataType))
			{
				// Retrieve the object asset from the payload.
				const util::uuid& id = *(const util::uuid*)payload->Data;
				core::asset::ptr from = mAsset_manager.get_asset(id);
				assert(from);
				mAsset_manager.move_asset(from, pAsset);
				deselect();
			}
		}
		ImGui::EndDragDropTarget();
	}
}

void asset_manager_window::remove_queued_assets()
{
	for (auto i : mRemove_queue)
	{
		mContext.close_editor(i);
		mAsset_manager.remove_asset(i);
	}
	mRemove_queue.clear();
}

void asset_manager_window::set_selection(const core::asset::ptr& pAsset)
{
	mSelected_asset.clear();
	if (pAsset)
		mSelected_asset.push_back(pAsset);
	mMulti_last_select = pAsset;
}

bool asset_manager_window::is_singlular_selection() const
{
	return mSelected_asset.size() == 1;
}

bool asset_manager_window::is_multi_selection() const
{
	return mSelected_asset.size() > 1;
}

bool asset_manager_window::is_selected(const core::asset::ptr& pAsset) const
{
	return std::find(mSelected_asset.begin(), mSelected_asset.end(), pAsset) != mSelected_asset.end();
}

void asset_manager_window::select(const core::asset::ptr& pAsset)
{
	if (ImGui::GetIO().KeyCtrl)
	{
		if (is_selected(pAsset))
			deselect(pAsset);
		else
		{
			mSelected_asset.push_back(pAsset);
			mMulti_last_select = pAsset;
		}
	}
	else if (ImGui::GetIO().KeyShift && !mSelected_asset.empty())
	{
		// Select based on how its displayed.
		bool selecting = false;
		mAsset_manager.for_each_child(mCurrent_folder, [&](auto& i)
		{
			// Only folders
			if (i->get_type() == "folder")
			{
				if (i == mMulti_last_select || i == pAsset)
				{
					selecting = !selecting;
					mSelected_asset.push_back(i);
				}
				else if (selecting)
					mSelected_asset.push_back(i);
			}
		});
		mAsset_manager.for_each_child(mCurrent_folder, [&](auto& i)
		{
			// Skip folders
			if (i->get_type() != "folder")
			{
				if (i == mMulti_last_select || i == pAsset)
				{
					selecting = !selecting;
					mSelected_asset.push_back(i);
				}
				else if (selecting)
					mSelected_asset.push_back(i);
			}
		});
	}
	else
		set_selection(pAsset);


}

void asset_manager_window::deselect()
{
	mSelected_asset.clear();
	mMulti_last_select.reset();
}

void asset_manager_window::deselect(const core::asset::ptr& pAsset)
{
	auto iter = std::find(mSelected_asset.begin(), mSelected_asset.end(), pAsset);
	if (iter != mSelected_asset.end())
	{
		mSelected_asset.erase(iter);
	}
}

void asset_manager_window::delete_selected()
{
	mRemove_queue.insert(mRemove_queue.end(), mSelected_asset.begin(), mSelected_asset.end());
	mSelected_asset.clear();
}

} // namespace wge::editor
