
// ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stl.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>


namespace ImGui
{

bool Splitter(const char* pStr_id, float* pDelta, float pWidth, bool pIs_horizontal)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	PushID(pStr_id);

	// Calculate size for the button
	ImVec2 size;
	if (pIs_horizontal)
		size = ImVec2(GetItemRectSize().x, pWidth);
	else
		size = ImVec2(pWidth, GetItemRectSize().y);
	
	InvisibleButton("Splitter", size);

	// This will help us check if the mouse left the button
	// So we don't end up locking the mouse cursor type to "Arrow"
	// When we arn't hovering it.
	bool* was_hovered = GetStateStorage()->GetBoolRef(GetID("WasHovered"), false);
	bool hovered = IsItemHovered();
	bool active = IsItemActive();

	if (hovered || active)
		SetMouseCursor(pIs_horizontal ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
	else if (*was_hovered)
		SetMouseCursor(ImGuiMouseCursor_Arrow);

	*was_hovered = hovered;

	// Apply the delta
	if (active)
		*pDelta += pIs_horizontal ? GetIO().MouseDelta.y : GetIO().MouseDelta.x;

	PopID();

	// Rendering

	ImU32 color;
	if (hovered && !active)
		color = GetColorU32(ImGuiCol_ButtonHovered);
	else if (active)
		color = GetColorU32(ImGuiCol_ButtonActive);
	else
		color = GetColorU32(ImGuiCol_Button);

	ImDrawList* dl = GetWindowDrawList();
	float radius = pWidth - 2;
	ImVec2 center_offset = GetItemRectSize() / 2;
	float left_right_offset = radius * 2 + 1;
	if (pIs_horizontal)
	{
		dl->AddCircleFilled(GetItemRectMin() + center_offset - ImVec2(left_right_offset, 0), radius, color);
		dl->AddCircleFilled(GetItemRectMin() + center_offset, radius, color);
		dl->AddCircleFilled(GetItemRectMin() + center_offset + ImVec2(left_right_offset, 0), radius, color);
	}
	else
	{
		dl->AddCircleFilled(GetItemRectMin() + center_offset - ImVec2(0, left_right_offset), radius, color);
		dl->AddCircleFilled(GetItemRectMin() + center_offset, radius, color);
		dl->AddCircleFilled(GetItemRectMin() + center_offset + ImVec2(0, left_right_offset), radius, color);
	}

	return active;
}

bool VerticalSplitter(const char* pStr_id, float* pDelta, float pWidth)
{
	return Splitter(pStr_id, pDelta, pWidth, false);
}

bool HorizontalSplitter(const char* pStr_id, float* pDelta, float pWidth)
{
	return Splitter(pStr_id, pDelta, pWidth, true);
}


ImVec2 GetWindowContentRegionSize()
{
	ImGuiWindow* window = GetCurrentWindowRead();
	return window->ContentsRegionRect.GetSize();
}

} // namespace ImGui