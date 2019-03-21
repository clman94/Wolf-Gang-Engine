#pragma once

// ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stl.h>

#include <wge/math/math.hpp>
#include <wge/graphics/opengl_framebuffer.hpp>
#include <wge/graphics/opengl_texture.hpp>

// Written in imgui style to fit better.

namespace ImGui
{

// Draws a framebuffer
inline void Image(const wge::graphics::framebuffer::ptr& mFramebuffer, const ImVec2& pSize = ImVec2(0, 0))
{
	auto ogl_texture = std::dynamic_pointer_cast<wge::graphics::opengl_framebuffer>(mFramebuffer);
	ImGui::Image((void*)ogl_texture->get_gl_texture(), pSize,
		ImVec2(0, 1), ImVec2(1, 0)); // Y-axis needs to be flipped
}

// Draws a texture
inline void Image(wge::core::asset::ptr mTexture, const ImVec2& pSize = ImVec2(0, 0), const ImVec2& pUV0 = ImVec2(0, 0), const ImVec2& pUV1 = ImVec2(1, 1))
{
	auto res = mTexture->get_resource<wge::graphics::texture>();
	if (!res)
		return;
	auto impl = std::dynamic_pointer_cast<wge::graphics::opengl_texture_impl>(res->get_implementation());
	ImGui::Image((void*)impl->get_gl_texture(), pSize, pUV0, pUV1);
}

// Draws a framebuffer
inline bool ImageButton(const wge::graphics::framebuffer::ptr& mFramebuffer, const ImVec2& pSize = ImVec2(0, 0), const ImVec2& pUV0 = ImVec2(0, 0), const ImVec2& pUV1 = ImVec2(1, 1))
{
	auto ogl_texture = std::dynamic_pointer_cast<wge::graphics::opengl_framebuffer>(mFramebuffer);
	return ImGui::ImageButton((void*)ogl_texture->get_gl_texture(), pSize,
		ImVec2(0, 1), ImVec2(1, 0)); // Y-axis needs to be flipped
}

// Draws a texture
inline bool ImageButton(wge::core::asset::ptr mTexture, const ImVec2& pSize = ImVec2(0, 0), const ImVec2& pUV0 = ImVec2(0, 0), const ImVec2& pUV1 = ImVec2(1, 1))
{
	auto res = mTexture->get_resource<wge::graphics::texture>();
	if (!res)
		return false;
	auto impl = std::dynamic_pointer_cast<wge::graphics::opengl_texture_impl>(res->get_implementation());
	return ImGui::ImageButton((void*)impl->get_gl_texture(), pSize, pUV0, pUV1);
}

inline bool CollapsingArrow(const char* pStr_id, bool* pOpen = nullptr, bool pDefault_open = false)
{
	ImGui::PushID(pStr_id);

	// Use internal instead
	if (!pOpen)
		pOpen = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("IsOpen"), pDefault_open);;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
	if (ImGui::ArrowButton("Arrow", *pOpen ? ImGuiDir_Down : ImGuiDir_Right))
		*pOpen = !*pOpen; // Toggle open flag
	ImGui::PopStyleColor(3);

	ImGui::PopID();
	return *pOpen;
}

inline bool SelectableHeader(const char* pLabel, bool pSelected = false, bool pDefault_open = false)
{
	ImGui::PushID(pLabel);

	bool result = ImGui::Selectable(pLabel, pSelected);

	ImGui::PopID();
}

inline void DrawGridLines(ImVec2 pMin, ImVec2 pMax, ImVec4 pColor, float pSquare_size = 20)
{
	using namespace wge;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->PushClipRect(pMin, pMax, true);

	// Optimize drawing by clamping the iterations with the clip rect
	const int x_start = static_cast<int>(math::max<float>(0, (dl->GetClipRectMin().x - pMin.x) - pSquare_size) / pSquare_size);
	const int y_start = static_cast<int>(math::max<float>(0, (dl->GetClipRectMin().y - pMin.y) - pSquare_size) / pSquare_size);
	const int x_count = static_cast<int>(math::min<float>(pMax.x - pMin.x, dl->GetClipRectMax().x - pMin.x) / pSquare_size + 1);
	const int y_count = static_cast<int>(math::min<float>(pMax.y - pMin.y, dl->GetClipRectMax().y - pMin.y) / pSquare_size + 1);

	for (int x = x_start; x < x_count; x++)
	{
		const float x_pos = x * pSquare_size + pMin.x;
		dl->AddLine({ x_pos, 0 }, { x_pos, dl->GetClipRectMax().y }, ImGui::GetColorU32(pColor));
	}

	for (int y = y_start; y < y_count; y++)
	{
		const float y_pos = y * pSquare_size + pMin.y;
		dl->AddLine({ 0, y_pos }, { dl->GetClipRectMax().x, y_pos }, ImGui::GetColorU32(pColor));
	}

	dl->PopClipRect();
}

inline void DrawAlphaCheckerBoard(ImVec2 pMin, ImVec2 pSize, float pSquare_size = 20)
{
	using namespace wge;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->PushClipRect(pMin, { pMin.x + pSize.x, pMin.y + pSize.y }, true);

	// Optimize drawing by clamping the iterations with the clip rect
	const int x_start = static_cast<int>(math::max<float>(0, (dl->GetClipRectMin().x - pMin.x) - pSquare_size) / pSquare_size);
	const int y_start = static_cast<int>(math::max<float>(0, (dl->GetClipRectMin().y - pMin.y) - pSquare_size) / pSquare_size);
	const int x_count = static_cast<int>(math::min<float>(pSize.x, dl->GetClipRectMax().x - pMin.x) / pSquare_size + 1);
	const int y_count = static_cast<int>(math::min<float>(pSize.y, dl->GetClipRectMax().y - pMin.y) / pSquare_size + 1);

	for (int x = x_start; x < x_count; x++)
	{
		for (int y = y_start; y < y_count; y++)
		{
			const float x_pos = x * pSquare_size + pMin.x;
			const float y_pos = y * pSquare_size + pMin.y;
			ImU32 col = (x + y) % 2 == 0 ? ImGui::GetColorU32({ 0.9f, 0.9f, 0.9f, 1 }) : ImGui::GetColorU32({ 0.5f, 0.5f, 0.5f, 1 });
			dl->AddRectFilled(
				{ x_pos, y_pos },
				{ x_pos + pSquare_size, y_pos + pSquare_size },
				col);
		}
	}
	dl->PopClipRect();
}

inline struct
{
	ImVec2 contentSize, maxScroll;
} gFixedScrollRegion;

inline void BeginFixedScrollRegion(ImVec2 pContentSize, ImVec2 pMax)
{
	using namespace wge;

	gFixedScrollRegion.contentSize = pContentSize;
	gFixedScrollRegion.maxScroll = pMax;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	ImGui::Dummy({ pContentSize.x, math::min(pMax.y, ImGui::GetScrollY()) });
	ImGui::Dummy({ math::min(pMax.x, ImGui::GetScrollX()), pContentSize.y });
	ImGui::SameLine();
}

inline void EndFixedScrollRegion()
{
	using namespace wge;

	ImGui::SameLine();
	ImGui::Dummy({ math::max(0.f, gFixedScrollRegion.maxScroll.x - ImGui::GetScrollX()), gFixedScrollRegion.contentSize.y });
	ImGui::Dummy({ gFixedScrollRegion.contentSize.x, math::max(0.f, gFixedScrollRegion.maxScroll.y - ImGui::GetScrollY()) });

	ImGui::PopStyleVar();
}

inline bool DragScroll(int pMouseButton, float pLockThreshold = -1)
{
	if (ImGui::IsMouseDragging(pMouseButton, pLockThreshold))
	{
		ImGui::SetScrollX(ImGui::GetScrollX() - ImGui::GetIO().MouseDelta.x);
		ImGui::SetScrollY(ImGui::GetScrollY() - ImGui::GetIO().MouseDelta.y);
		return true;
	}
	return false;
}

bool VerticalSplitter(const char* pStr_id, float* pDelta, float pWidth = 5);
bool HorizontalSplitter(const char* pStr_id, float* pDelta, float pWidth = 5);

ImVec2 GetWindowContentRegionSize();

} // namespace ImGui
