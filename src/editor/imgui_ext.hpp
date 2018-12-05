
// ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stl.h>

namespace ImGui
{

// Draws a framebuffer
inline void Image(const wge::graphics::framebuffer& mFramebuffer, const ImVec2& pSize = ImVec2(0, 0))
{
	ImGui::Image((void*)mFramebuffer.get_gl_texture(), pSize,
		ImVec2(0, 1), ImVec2(1, 0)); // Y-axis needs to be flipped
}

// Draws a texture
inline void Image(wge::graphics::texture::ptr mTexture, const ImVec2& pSize = ImVec2(0, 0))
{
	ImGui::Image((void*)mTexture->get_gl_texture(), pSize);
}

// Draws a texture
inline void ImageButton(wge::graphics::texture::ptr mTexture, const ImVec2& pSize = ImVec2(0, 0))
{
	ImGui::ImageButton((void*)mTexture->get_gl_texture(), pSize);
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

} // namespace ImGui