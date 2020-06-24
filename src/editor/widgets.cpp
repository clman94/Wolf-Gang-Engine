#include "widgets.hpp"
#include "imgui_editor_tools.hpp"

namespace wge::editor
{

bool asset_item(const core::asset::ptr& pAsset, const core::asset_manager& pAsset_manager, ImVec2 pPreview_size)
{
	if (!pAsset)
		return false;
	ImGui::PushID(&*pAsset);
	ImGui::BeginGroup();
	if (pAsset->get_type() == "texture")
	{
		ImGui::ImageButton(pAsset, pPreview_size);
	}
	else
	{
		ImGui::Button("", pPreview_size);
	}
	ImGui::SameLine();
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


void preview_image(const char* pStr_id, const graphics::texture& pTexture, const math::vec2& pSize, const math::rect& pFrame_rect)
{
	if (pSize.x <= 0 || pSize.y <= 0)
		return;

	// Scale the size of the image to preserve the aspect ratio but still fit in the
	// specified area.
	const float aspect_ratio = pFrame_rect.size.x / pFrame_rect.size.y;
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

	// Convert to UV coord
	math::aabb uv(pFrame_rect);
	uv.min /= math::vec2(pTexture.get_size());
	uv.max /= math::vec2(pTexture.get_size());

	// Draw the image
	const auto impl = std::dynamic_pointer_cast<graphics::opengl_texture_impl>(pTexture.get_implementation());
	auto dl = ImGui::GetWindowDrawList();
	dl->AddImage(reinterpret_cast<void*>(static_cast<std::uintptr_t>(impl->get_gl_texture())), pos, pos + scaled_size, uv.min, uv.max);

	// Add an invisible button so we can interact with this image
	ImGui::InvisibleButton(pStr_id, pSize);
}

void preview_image(const char* pStr_id, const graphics::texture& pTexture, const math::vec2& pSize)
{
	preview_image(pStr_id, pTexture, pSize, math::rect{
		math::vec2{0, 0},
		math::vec2{ pTexture.get_size() }
		});
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

core::asset::ptr asset_selector(const char* pStr_id, const std::string& pType, const core::asset_manager& pAsset_manager, core::asset::ptr pCurrent_asset)
{
	const ImVec2 preview_size = { 50, 50 };

	core::asset::ptr asset = nullptr;
	ImGui::BeginGroup();

	if (pCurrent_asset)
	{
		asset_item(pCurrent_asset, pAsset_manager, preview_size);
	}
	else
	{
		ImGui::Button("", preview_size);
		ImGui::SameLine();
		ImGui::Text("Select/Drop asset");
	}

	ImGui::EndGroup();
	if (auto dropped_asset = asset_drag_drop_target(pType, pAsset_manager))
		asset = dropped_asset;
	if (ImGui::BeginPopupContextWindow("AssetSelectorWindow"))
	{
		ImGui::Text((const char*)ICON_FA_SEARCH);
		ImGui::SameLine();
		static std::string search_str;
		ImGui::InputText("##Search", &search_str);
		ImGui::BeginChild("AssetList", ImVec2(170, 400));
		for (const auto& i : pAsset_manager.get_asset_list())
		{
			std::string_view name{ i->get_name() };
			if (i->get_type() == pType &&
				name.size() >= search_str.size() &&
				name.substr(0, search_str.size()) == search_str)
			{
				if (asset_item(i, pAsset_manager, preview_size))
				{
					asset = i;
					ImGui::CloseCurrentPopup();
				}
			}
		}
		ImGui::EndChild();
		ImGui::EndPopup();
	}
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
