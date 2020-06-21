#pragma once 

#include "imgui_ext.hpp"
#include "icon_codepoints.hpp"

#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/graphics/texture.hpp>

#include <imgui/imgui.h>

namespace wge::editor
{

bool asset_item(const core::asset::ptr& pAsset, const core::asset_manager& pAsset_manager, ImVec2 pPreview_size = { 0, 0 });
void preview_image(const char* pStr_id, const graphics::texture& pTexture, const math::vec2& pSize, const math::rect& pFrame_rect);
core::asset::ptr asset_drag_drop_target(const std::string& pType, const core::asset_manager& pAsset_manager);
core::asset::ptr asset_selector(const char* pStr_id, const std::string& pType, const core::asset_manager& pAsset_manager, core::asset::ptr pCurrent_asset = nullptr);
void begin_image_editor(const char* pStr_id, const graphics::texture& pTexture, const math::aabb& pUV = { 0, 0, 1, 1 }, bool pShow_alpha = false);
void end_image_editor();

} // namespace wge::editor
