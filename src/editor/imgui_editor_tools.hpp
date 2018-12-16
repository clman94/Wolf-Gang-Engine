#pragma once

#include <vector>
#include <map>

#include <wge/math/rect.hpp>
#include <wge/math/vector.hpp>
#include <wge/graphics/color.hpp>

#include <imgui/imgui.h>

#include "imgui_ext.hpp"

// This is essentually an overlay type editor.

namespace wge::editor::visual_editor
{

struct editor_state
{
	math::vec2 cursor_offset; // in pixels. This is for offsetting the graphics themselves to align with your window or position of choice. 
	math::vec2 offset; // in editor units
	math::vec2 scale;
	math::vec2 mouse_position; // in pixels
	math::vec2 mouse_editor_position; // in editor units
	ImGuiID active_dragger_id{ 0 };

	math::rect calc_absolute(const math::rect& pRect) const
	{
		return{ calc_absolute(pRect.position), pRect.size * scale };
	}

	math::aabb calc_absolute(const math::aabb& pAABB) const
	{
		return{ calc_absolute(pAABB.min), calc_absolute(pAABB.max) };
	}

	math::vec2 calc_absolute(const math::vec2& pPos) const
	{
		return (pPos - offset) * scale + cursor_offset;
	}

	math::vec2 calc_from_absolute(const math::vec2& pPos) const
	{
		return (pPos - cursor_offset) / scale + offset;
	}
};

std::map<ImGuiID, editor_state> gEditor_states;
editor_state* gCurrent_editor_state = nullptr;

void begin(const char* pStr_id, const math::vec2& pCursor_offset, const math::vec2& pOffset, const math::vec2& pScale)
{
	ImGui::PushID(pStr_id);

	// Update the state
	gCurrent_editor_state = &gEditor_states[ImGui::GetID("_EditorState")];
	gCurrent_editor_state->cursor_offset = pCursor_offset;
	gCurrent_editor_state->offset = pOffset;
	gCurrent_editor_state->scale = pScale;
	gCurrent_editor_state->mouse_position = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
	gCurrent_editor_state->mouse_editor_position = gCurrent_editor_state->calc_from_absolute(gCurrent_editor_state->mouse_position);
}

void end()
{
	assert(gCurrent_editor_state);
	gCurrent_editor_state = nullptr;
	ImGui::PopID();
}

void draw_circle(const math::vec2& pCenter, float pRadius, const graphics::color& pColor, float pThickness, bool pScale_radius = false)
{
	assert(gCurrent_editor_state);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	math::vec2 center = gCurrent_editor_state->calc_absolute(pCenter);
	dl->AddCircle({ center.x, center.y },
		pRadius, ImGui::GetColorU32({ pColor.r, pColor.g, pColor.b, pColor.a }), 12, pThickness);
}

void draw_circle(const math::vec2& pCenter, float pRadius, const graphics::color& pColor, bool pScale_radius = false)
{
	draw_circle(pCenter, pRadius, pColor, 1, pScale_radius);
}

void draw_rect(const math::aabb& pAABB, const graphics::color& pColor)
{
	assert(gCurrent_editor_state);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	math::aabb aabb(gCurrent_editor_state->calc_absolute(pAABB));
	dl->AddRect({ aabb.min.x, aabb.min.y }, { aabb.max.x, aabb.max.y }, ImGui::GetColorU32({ pColor.r, pColor.g, pColor.b, pColor.a }));
}

void draw_rect(const math::rect& pRect, const graphics::color& pColor)
{
	draw_rect(math::aabb(pRect), pColor);
}

void draw_line(const math::vec2& pP0, const math::vec2& pP1, const graphics::color& pColor, float pThickness = 1)
{
	assert(gCurrent_editor_state);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	math::vec2 p0 = gCurrent_editor_state->calc_absolute(pP0);
	math::vec2 p1 = gCurrent_editor_state->calc_absolute(pP1);
	dl->AddLine({ p0.x, p0.y }, { p1.x, p1.y }, ImGui::GetColorU32({ pColor.r, pColor.g, pColor.b, pColor.a }), pThickness);
}

inline void draw_grid(graphics::color pColor, float pSquare_size)
{
	assert(gCurrent_editor_state);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	math::vec2 min = gCurrent_editor_state->calc_absolute(math::vec2(-1000, -1000));
	ImGui::DrawGridLines({ min.x, min.y }, dl->GetClipRectMax(), { pColor.r, pColor.g, pColor.b, pColor.a }, gCurrent_editor_state->scale.x * pSquare_size);
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
	const bool hovered = gCurrent_editor_state->mouse_position.distance(gCurrent_editor_state->calc_absolute(pPos)) <= pRadius;
	const bool dragging = drag_behavior(id, hovered, pDelta);

	draw_circle(pPos, pRadius, { 1, 1, 0, 0.5f });

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

	draw_rect(pDisplay, { 1, 1, 0, 0.5f });

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
