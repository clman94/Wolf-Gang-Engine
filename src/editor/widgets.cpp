#include "widgets.hpp"
#include "imgui_editor_tools.hpp"
#include <wge/graphics/sprite.hpp>
#include <wge/graphics/tileset.hpp>
#include <wge/core/object_resource.hpp>

namespace wge::editor
{

bool asset_item(const core::asset::ptr& pAsset, const core::asset_manager& pAsset_manager, ImVec2 pPreview_size)
{
	ImGui::PushID(&*pAsset);
	ImGui::BeginGroup();
	if (pPreview_size.x > 0 || pPreview_size.y > 0)
	{
		asset_preview("Preview", pAsset, pAsset_manager, pPreview_size);
		ImGui::SameLine();
	}
	ImGui::BeginGroup();
	ImGui::TextColored({ 0, 1, 1, 1 }, pAsset->get_name().c_str());
	ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 0 }, pAsset_manager.get_asset_path(pAsset).parent().string().c_str());
	ImGui::EndGroup();
	ImGui::EndGroup();
	ImGui::PopID();

	const bool clicked = ImGui::IsItemClicked();

	math::vec2 item_min = ImGui::GetItemRectMin()
		- ImGui::GetStyle().ItemSpacing;
	math::vec2 item_max = ImGui::GetItemRectMax()
		+ ImGui::GetStyle().ItemSpacing;

	// Draw the background
	auto dl = ImGui::GetWindowDrawList();
	if (ImGui::IsItemHovered())
	{
		dl->AddRectFilled(item_min, item_max,
			ImGui::GetColorU32(ImGuiCol_ButtonHovered), ImGui::GetStyle().FrameRounding);
	}
	else if (clicked)
	{
		dl->AddRectFilled(item_min, item_max,
			ImGui::GetColorU32(ImGuiCol_ButtonActive), ImGui::GetStyle().FrameRounding);
	}
	return clicked;
}

void asset_preview(const char* pStr_id, const core::asset::ptr& pAsset, const core::asset_manager& pAsset_manager, const math::vec2& pSize)
{
	if (pAsset->get_type() == "sprite")
	{
		auto sprite = pAsset->get_resource<graphics::sprite>();
		preview_image(pStr_id, sprite->get_texture(), pSize, sprite->get_frame_uv(0));
	}
	else if (pAsset->get_type() == "object")
	{
		auto obj = pAsset->get_resource<core::object_resource>();
		auto sprite = pAsset_manager.get_resource<graphics::sprite>(obj->display_sprite);
		if (sprite)
		{
			preview_image(pStr_id, sprite->get_texture(), pSize, sprite->get_frame_uv(0));
		}
		else
		{
			ImGui::PushID(pStr_id);
			ImGui::Button("No Preview", pSize);
			ImGui::PopID();
		}
	}
	else if (pAsset->get_type() == "tileset")
	{
		auto tileset = pAsset->get_resource<graphics::tileset>();
		assert(tileset);
		preview_image(pStr_id, tileset->get_texture(), pSize);
	}
	else
	{
		ImGui::PushID(pStr_id);
		ImGui::Button("No Preview", pSize);
		ImGui::PopID();
	}
}

void preview_image(const char* pStr_id, const graphics::texture& pTexture, const math::vec2& pSize, const math::aabb& pFrame_uv)
{
	if (pSize.x <= 0 || pSize.y <= 0)
		return;

	// Scale the size of the image to preserve the aspect ratio but still fit in the
	// specified area.
	const math::vec2 uv_size = pFrame_uv.max - pFrame_uv.min;
	const float aspect_ratio = 
		static_cast<float>(pTexture.get_width() * uv_size.x) /
		static_cast<float>(pTexture.get_height() * uv_size.y);
	math::vec2 scaled_size =
	{
		math::min(pSize.y * aspect_ratio, pSize.x),
		math::min(pSize.x / aspect_ratio, pSize.y)
	};

	// Center the position
	const math::vec2 center_offset = pSize / 2 - scaled_size / 2;
	const math::vec2 pos = math::vec2(ImGui::GetCursorScreenPos()) + center_offset;

	// Draw the checkered background
	ImGui::DrawAlphaCheckerBoard(pos, scaled_size, 10);

	// Draw the image
	const auto impl = std::dynamic_pointer_cast<graphics::opengl_texture_impl>(pTexture.get_implementation());
	auto dl = ImGui::GetWindowDrawList();
	dl->AddImage(reinterpret_cast<void*>(static_cast<std::uintptr_t>(impl->get_gl_texture())), pos, pos + scaled_size, pFrame_uv.min, pFrame_uv.max);

	// Add an invisible button so we can interact with this image
	ImGui::InvisibleButton(pStr_id, pSize);
}

core::asset::ptr asset_drag_drop_target(const std::string& pType, const core::asset_manager& pAsset_manager)
{
	core::asset::ptr result;
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload((pType + "Asset").c_str()))
		{
			// Retrieve the object asset from the payload.
			const util::uuid& id = *(const util::uuid*)payload->Data;
			result = pAsset_manager.get_asset(id);
		}
		ImGui::EndDragDropTarget();
	}
	return result;
}

core::asset::ptr asset_selector(const char* pStr_id, const std::string& pType, const core::asset_manager& pAsset_manager, core::asset::ptr pCurrent_asset, const math::vec2& pPreview_size)
{
	ImGui::PushID(pStr_id);
	core::asset::ptr asset = nullptr;
	ImGui::BeginGroup();

	if (pCurrent_asset)
	{
		asset_item(pCurrent_asset, pAsset_manager, pPreview_size);
	}
	else if (pPreview_size.x <= 0 || pPreview_size.y <= 0)
	{
		ImGui::Button("Select/Drop asset");
	}
	else
	{
		ImGui::Button("", pPreview_size);
		ImGui::SameLine();
		ImGui::Text("Select/Drop asset");
	}

	ImGui::EndGroup();
	if (auto dropped_asset = asset_drag_drop_target(pType, pAsset_manager))
		asset = dropped_asset;
	if (ImGui::BeginPopupContextItem("AssetSelectorWindow", 0))
	{
		ImGui::Text((const char*)ICON_FA_SEARCH);
		ImGui::SameLine();
		static std::string search_str;
		ImGui::InputText("##Search", &search_str);
		ImGui::BeginChild("AssetList", ImVec2(0, 400));
		for (const auto& i : pAsset_manager.get_asset_list())
		{
			std::string_view name{ i->get_name() };
			if (i->get_type() == pType &&
				name.size() >= search_str.size() &&
				name.substr(0, search_str.size()) == search_str)
			{
				if (asset_item(i, pAsset_manager, pPreview_size))
				{
					asset = i;
					ImGui::CloseCurrentPopup();
				}
			}
		}
		ImGui::EndChild();
		ImGui::EndPopup();
	}
	ImGui::PopID();
	return asset;
}

void begin_image_editor(const char* pStr_id, const graphics::texture& pTexture, const math::aabb& pUV, bool pShow_alpha)
{
	ImGui::BeginChild(pStr_id, ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	float* zoom = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("_Zoom"), 3);
	float scale = std::powf(2, *zoom);

	ImVec2 image_size = (math::vec2(pUV.max - pUV.min) * math::vec2(pTexture.get_size())) * scale;

	const ImVec2 top_cursor = ImGui::GetCursorScreenPos();

	// Top and left padding
	ImGui::Dummy(ImVec2(0, ImGui::GetWindowHeight() / 2));
	ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, 0));
	ImGui::SameLine();

	// Store the cursor so we can position things on top of the image
	const ImVec2 image_position = ImGui::GetCursorScreenPos();

	// A checker board will help us "see" the alpha channel of the image
	ImGui::DrawAlphaCheckerBoard(image_position, image_size);

	ImGui::Image(pTexture, image_size, pUV.min, pUV.max);

	// Right and bottom padding
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, 0));
	ImGui::Dummy(ImVec2(0, ImGui::GetWindowHeight() / 2));

	ImGui::SetCursorScreenPos(top_cursor);

	ImGui::InvisibleButton("_Input", ImGui::GetWindowSize() + image_size);
	if (ImGui::IsItemHovered())
	{
		// Zoom with ctrl and mousewheel
		if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0)
		{
			*zoom += ImGui::GetIO().MouseWheel;
			const float new_scale = std::powf(2, *zoom);
			const float ratio_changed = new_scale / scale;
			const ImVec2 old_scroll{ ImGui::GetScrollX(), ImGui::GetScrollY() };
			const ImVec2 new_scroll = old_scroll * ratio_changed;
			// ImGui doesn't like content size changes after setting the scroll, this appears to fix it.
			ImGui::Dummy((new_scroll - old_scroll) + ImVec2(ImGui::GetWindowWidth() + image_size.x, 0));
			ImGui::SetScrollX(new_scroll.x);
			ImGui::SetScrollY(new_scroll.y);
		}
		else if (!ImGui::GetIO().KeyCtrl)
		{
			ImGui::SetScrollX(ImGui::GetScrollX() - ImGui::GetIO().MouseWheelH * 10);
			ImGui::SetScrollY(ImGui::GetScrollY() - ImGui::GetIO().MouseWheel * 10);
		}

		// Hold middle mouse button to scroll
		ImGui::DragScroll(2);
	}

	// Draw grid
	if (*zoom > 2)
	{
		ImGui::DrawGridLines(image_position,
			ImVec2(image_position.x + image_size.x, image_position.y + image_size.y),
			{ 0, 1, 1, 0.2f }, scale);
	}

	visual_editor::begin("_SomeEditor", { image_position.x, image_position.y }, { 0, 0 }, { scale, scale });
}

void end_image_editor()
{
	visual_editor::end();
	ImGui::EndChild();
}


} // namespace wge::editor
