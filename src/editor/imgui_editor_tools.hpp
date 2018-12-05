
#include <vector>
#include <map>

#include <wge/math/rect.hpp>
#include <wge/math/vector.hpp>

#include <imgui/imgui.h>


namespace wge::editor::visual_editor
{

struct editor_state
{
	math::vec2 offset;
	math::vec2 scale;
	math::vec2 mouse_position; // In pixels
	math::vec2 mouse_editor_position; // Scaled and offsetted
	ImGuiID active_dragger_id{ 0 };

	math::vec2 calc_absolute_position(math::vec2 pPos) const
	{
		return pPos * scale + offset;
	}
};

std::map<ImGuiID, editor_state> gEditor_states;
editor_state* gCurrent_editor_state = nullptr;

void begin_editor(const char* pStr_id, math::vec2 pCursor_offset, math::vec2 pScale)
{
	ImGui::PushID(pStr_id);

	// Update the state
	gCurrent_editor_state = &gEditor_states[ImGui::GetID("_EditorState")];
	gCurrent_editor_state->offset = pCursor_offset;
	gCurrent_editor_state->scale = pScale;
	gCurrent_editor_state->mouse_position = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
	gCurrent_editor_state->mouse_editor_position = (gCurrent_editor_state->mouse_position - pCursor_offset) / pScale;
}

void end_editor()
{
	assert(gCurrent_editor_state);
	gCurrent_editor_state = nullptr;
	ImGui::PopID();
}

math::vec2 get_mouse_position()
{
	assert(gCurrent_editor_state);
	return gCurrent_editor_state->mouse_editor_position;
}

bool drag_behavior(ImGuiID pID, bool pHovered)
{
	assert(gCurrent_editor_state);
	bool dragging = gCurrent_editor_state->active_dragger_id == pID;
	if (pHovered && ImGui::IsItemClicked(0) && gCurrent_editor_state->active_dragger_id == 0)
	{
		gCurrent_editor_state->active_dragger_id = pID; // Start drag
		dragging = true;
	}
	else if (!ImGui::IsMouseDown(0) && dragging)
	{
		gCurrent_editor_state->active_dragger_id = 0; // End drag
		dragging = true; // Return true for one more frame after the mouse is released
						 // so the user can handle mouse-released events.
	}
	return dragging;
}

bool drag_behavior(ImGuiID pID, bool pHovered, float* pX, float* pY)
{
	bool dragging = drag_behavior(pID, pHovered);
	if (dragging)
	{
		if (pX)
			*pX += ImGui::GetIO().MouseDelta.x / gCurrent_editor_state->scale.x;
		if (pY)
			*pY += ImGui::GetIO().MouseDelta.y / gCurrent_editor_state->scale.y;
	}
	return dragging;
}

bool drag_behavior(ImGuiID pID, bool pHovered, math::vec2* pVec)
{
	return drag_behavior(pID, pHovered, &pVec->x, &pVec->y);
}

bool drag_circle(const char* pStr_id, math::vec2 pPos, math::vec2* pDelta, float pRadius)
{
	assert(gCurrent_editor_state);
	const ImGuiID id = ImGui::GetID(pStr_id);
	const math::vec2 center = gCurrent_editor_state->calc_absolute_position(pPos);
	const bool hovered = gCurrent_editor_state->mouse_position.distance(center) <= pRadius;
	const bool dragging = drag_behavior(id, hovered, pDelta);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddCircle({ center.x, center.y }, pRadius, ImGui::GetColorU32({ 1, 1, 0, 0.5f }));

	return dragging;
}

bool drag_circle(const char* pStr_id, math::vec2* pDelta, float pRadius)
{
	return drag_circle(pStr_id, *pDelta, pDelta, pRadius);
}

bool drag_rect(const char* pStr_id, const math::rect& pDisplay, math::vec2* pDelta)
{
	const ImGuiID id = ImGui::GetID(pStr_id);
	const bool hovered = pDisplay.intersects(gCurrent_editor_state->mouse_editor_position);
	const bool dragging = drag_behavior(id, hovered, pDelta);

	const math::vec2 min = gCurrent_editor_state->calc_absolute_position(pDisplay.position);
	const math::vec2 max = gCurrent_editor_state->calc_absolute_position(pDisplay.position + pDisplay.size);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRect({ min.x, min.y }, { max.x, max.y }, ImGui::GetColorU32({ 1, 1, 0, 0.5f }));

	return dragging;
}

bool drag_resizable_rect(const char* pStr_id, math::rect* pRect)
{
	bool dragging = false;

	ImGui::PushID(pStr_id);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->ChannelsSplit(2);
	dl->ChannelsSetCurrent(1);

	math::vec2 topleft;
	if (drag_circle("TopLeft", pRect->position, &topleft, 6))
	{
		dragging = true;
		pRect->position += topleft;
		pRect->size -= topleft;
	}

	dragging |= drag_circle("BottomRight", pRect->get_corner(2), &pRect->size, 6);

	math::vec2 topright;
	if (drag_circle("TopRight", pRect->get_corner(1), &topright, 6))
	{
		dragging = true;
		pRect->y += topright.y;
		pRect->width += topright.x;
		pRect->height -= topright.y;
	}

	math::vec2 bottomleft;
	if (drag_circle("BottomLeft", pRect->get_corner(3), &bottomleft, 6))
	{
		dragging = true;
		pRect->x += bottomleft.x;
		pRect->width -= bottomleft.x;
		pRect->height += bottomleft.y;
	}

	math::vec2 top;
	if (drag_circle("Top", pRect->position + math::vec2(pRect->width / 2, 0), &top, 6))
	{
		dragging = true;
		pRect->y += top.y;
		pRect->height -= top.y;
	}

	math::vec2 bottom;
	if (drag_circle("Bottom", pRect->position + math::vec2(pRect->width / 2, pRect->height), &bottom, 6))
	{
		dragging = true;
		pRect->height += bottom.y;
	}

	math::vec2 left;
	if (drag_circle("Left", pRect->position + math::vec2(0, pRect->height / 2), &left, 6))
	{
		dragging = true;
		pRect->x += left.x;
		pRect->width -= left.x;
	}

	math::vec2 right;
	if (drag_circle("Right", pRect->position + math::vec2(pRect->width, pRect->height / 2), &right, 6))
	{
		dragging = true;
		pRect->width += right.x;
	}

	dl->ChannelsSetCurrent(0);

	dragging |= drag_rect("RectMove", *pRect, &pRect->position);

	dl->ChannelsMerge();

	ImGui::PopID();

	return dragging;
}

} // namespace wge::editor::visual_editor
