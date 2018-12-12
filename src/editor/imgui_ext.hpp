
// ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stl.h>
#include <wge/math/math.hpp>
#include <wge/graphics/opengl_framebuffer.hpp>
#include <wge/graphics/opengl_texture.hpp>

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
inline void Image(wge::graphics::texture::ptr mTexture, const ImVec2& pSize = ImVec2(0, 0), const ImVec2& pUV0 = ImVec2(0, 0), const ImVec2& pUV1 = ImVec2(1, 1))
{
	auto impl = std::dynamic_pointer_cast<wge::graphics::opengl_texture_impl>(mTexture->get_implementation());
	ImGui::Image((void*)impl->get_gl_texture(), pSize, pUV0, pUV1);
}

// Draws a texture
inline bool ImageButton(wge::graphics::texture::ptr mTexture, const ImVec2& pSize = ImVec2(0, 0), const ImVec2& pUV0 = ImVec2(0, 0), const ImVec2& pUV1 = ImVec2(1, 1))
{
	auto impl = std::dynamic_pointer_cast<wge::graphics::opengl_texture_impl>(mTexture->get_implementation());
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

} // namespace ImGui