#pragma once 

#include "imgui_ext.hpp"
#include "icon_codepoints.hpp"

#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/graphics/texture.hpp>

#include <imgui/imgui.h>

namespace wge::editor
{

inline bool asset_item(const core::asset::ptr& pAsset, const core::asset_manager& pAsset_manager, ImVec2 pPreview_size = { 0, 0 })
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

inline void preview_image(const char* pStr_id, const graphics::texture& pTexture, const math::vec2& pSize, const math::rect& pFrame_rect)
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


inline core::asset::ptr asset_drag_drop_target(const std::string& pType, const core::asset_manager& pAsset_manager)
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

inline core::asset::ptr asset_selector(const char* pStr_id, const std::string& pType, const core::asset_manager& pAsset_manager, core::asset::ptr pCurrent_asset = nullptr)
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

} // namespace wge::editor
