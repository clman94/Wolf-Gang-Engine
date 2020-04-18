#include "imgui_editor_tools.hpp"

#include <vector>
#include <map>

#include <wge/math/transform_stack.hpp>

#include <imgui/imgui.h>

namespace wge::editor::visual_editor
{

struct editor_state
{
	math::vec2 cursor_offset; // in pixels. This is for offsetting the graphics themselves to align with your window or position of choice. 
	math::vec2 offset; // in editor units
	math::vec2 scale;
	math::vec2 mouse_position; // in pixels

	ImGuiID active_dragger_id{ 0 };

	math::transform_stack transform;

	// Snapping
	math::vec2 delta_accum;
	bool is_snap_enabled{ false };
	math::vec2 snap_ratio{ 1, 1 };

	math::vec2 snap_closest(const math::vec2& pVec) const
	{
		if (!is_snap_enabled)
			return pVec;
		return (pVec / snap_ratio).round() * snap_ratio;
	}

	math::rect snap_closest(const math::rect& pRect) const
	{
		return{ snap_closest(pRect.position), snap_closest(pRect.size) };
	}

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
		return ((transform.apply_to(pPos) - offset) * scale) + cursor_offset;
	}

	math::vec2 calc_from_absolute(const math::vec2& pPos) const
	{
		return transform.apply_inverse_to((pPos - cursor_offset) / scale + offset);
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
}

void end()
{
	assert(gCurrent_editor_state);
	gCurrent_editor_state = nullptr;
	ImGui::PopID();
}

void begin_snap(const math::vec2 & pRatio)
{
	assert(gCurrent_editor_state);
	gCurrent_editor_state->is_snap_enabled = true;
	gCurrent_editor_state->snap_ratio = pRatio;
}

void end_snap()
{
	assert(gCurrent_editor_state);
	gCurrent_editor_state->is_snap_enabled = false;
}

void push_transform(const math::transform& pTransform)
{
	assert(gCurrent_editor_state);
	gCurrent_editor_state->transform.push(pTransform);
}

void pop_transform(std::size_t pCount)
{
	assert(gCurrent_editor_state);
	for (std::size_t i = 0; i < pCount; i++)
		gCurrent_editor_state->transform.pop();
}

const math::transform& get_transform() noexcept
{
	assert(gCurrent_editor_state);
	return gCurrent_editor_state->transform.get();
}

math::vec2 snap_behavior(const math::vec2& pDelta)
{
	assert(gCurrent_editor_state);
	if (gCurrent_editor_state->is_snap_enabled)
	{
		// Start at 0
		if (ImGui::IsItemClicked())
			gCurrent_editor_state->delta_accum = math::vec2(0, 0);

		// Accumulate the delta...
		gCurrent_editor_state->delta_accum += pDelta;
		math::vec2 result = gCurrent_editor_state->snap_closest(gCurrent_editor_state->delta_accum);

		// ...until it snaps!
		if (!result.is_zero())
			gCurrent_editor_state->delta_accum -= result;

		return result;
	}
	else
		return pDelta;
}

// This is affected by the transformation stack
math::vec2 get_mouse_delta()
{
	assert(gCurrent_editor_state);
	const math::vec2 delta = math::vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
	return get_transform().apply_inverse_to(delta / gCurrent_editor_state->scale, math::transform_mask::position);
}

void draw_circle(const math::vec2& pCenter, float pRadius, const graphics::color& pColor, float pThickness, bool pScale_radius)
{
	assert(gCurrent_editor_state);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	math::vec2 center = gCurrent_editor_state->calc_absolute(pCenter);
	dl->AddCircle({ center.x, center.y },
		pRadius, ImGui::GetColorU32({ pColor.r, pColor.g, pColor.b, pColor.a }), 12, pThickness);
}

void draw_circle(const math::vec2& pCenter, float pRadius, const graphics::color& pColor, bool pScale_radius)
{
	draw_circle(pCenter, pRadius, pColor, 1, pScale_radius);
}

void draw_rect(const math::aabb& pAABB, const graphics::color& pColor)
{
	assert(gCurrent_editor_state);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	std::size_t start_index = dl->VtxBuffer.size();
	dl->PathRect({ pAABB.min.x, pAABB.min.y }, { pAABB.max.x, pAABB.max.y });
	for (std::size_t i = 0; i < static_cast<std::size_t>(dl->_Path.size()); i++)
	{
		math::vec2 pos(dl->_Path[i].x, dl->_Path[i].y);
		math::vec2 newpos = gCurrent_editor_state->calc_absolute(pos);
		dl->_Path[i].x = newpos.x;
		dl->_Path[i].y = newpos.y;
	}
	dl->PathStroke(ImGui::GetColorU32({ pColor.r, pColor.g, pColor.b, pColor.a }), true, 2);
}

void draw_rect(const math::rect& pRect, const graphics::color& pColor)
{
	draw_rect(math::aabb(pRect), pColor);
}

void draw_line(const math::vec2& pP0, const math::vec2& pP1, const graphics::color& pColor, float pThickness)
{
	assert(gCurrent_editor_state);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	math::vec2 p0 = gCurrent_editor_state->calc_absolute(pP0);
	math::vec2 p1 = gCurrent_editor_state->calc_absolute(pP1);
	dl->AddLine({ p0.x, p0.y }, { p1.x, p1.y }, ImGui::GetColorU32({ pColor.r, pColor.g, pColor.b, pColor.a }), pThickness);
}

void draw_grid(const graphics::color& pColor, float pSquare_size)
{
	assert(gCurrent_editor_state);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	math::vec2 min = gCurrent_editor_state->calc_absolute(math::vec2(-pSquare_size * 1000, -pSquare_size * 1000));
	ImGui::DrawGridLines({ min.x, min.y }, dl->GetClipRectMax(), { pColor.r, pColor.g, pColor.b, pColor.a }, gCurrent_editor_state->scale.x * pSquare_size);
}

math::vec2 get_mouse_position()
{
	assert(gCurrent_editor_state);
	return gCurrent_editor_state->calc_from_absolute(gCurrent_editor_state->mouse_position);
}

bool drag_behavior(ImGuiID pID, bool pHovered)
{
	assert(gCurrent_editor_state);
	bool is_dragging_this = gCurrent_editor_state->active_dragger_id == pID;
	bool is_dragging_at_all = gCurrent_editor_state->active_dragger_id != 0;
	if (pHovered && ImGui::IsItemHovered() && ImGui::IsMouseDragging(0, 0.1f) // Note: Threshold really tiny so then draggable circles work right.
		&& !is_dragging_at_all)
	{
		// Start drag
		gCurrent_editor_state->active_dragger_id = pID;
		is_dragging_this = true;
	}
	else if (ImGui::IsMouseReleased(0) && is_dragging_this)
	{
		// End drag
		gCurrent_editor_state->active_dragger_id = 0;

		// Return true for one more frame after the mouse is released
		// so the user can handle mouse-released events.
		is_dragging_this = true;
	}
	return is_dragging_this;
}

bool drag_behavior(ImGuiID pID, bool pHovered, float* pX, float* pY)
{
	assert(gCurrent_editor_state);
	bool dragging = drag_behavior(pID, pHovered);
	if (dragging)
	{
		math::vec2 delta = snap_behavior(get_mouse_delta());
		if (pX)
			*pX += delta.x;
		if (pY)
			*pY += delta.y;
	}
	return dragging;
}

bool drag_behavior(ImGuiID pID, bool pHovered, math::vec2* pVec)
{
	return drag_behavior(pID, pHovered, &pVec->x, &pVec->y);
}

bool is_dragging()
{
	assert(gCurrent_editor_state);
	return gCurrent_editor_state->active_dragger_id != 0;
}

bool drag_circle(const char* pStr_id, math::vec2 pPos, math::vec2* pDelta, float pRadius)
{
	assert(gCurrent_editor_state);
	const ImGuiID id = ImGui::GetID(pStr_id);
	const bool hovered = gCurrent_editor_state->mouse_position.distance(gCurrent_editor_state->calc_absolute(pPos)) <= pRadius;
	const bool dragging = drag_behavior(id, hovered, pDelta);

	if (hovered || dragging)
		draw_circle(pPos, pRadius, { 1, 1, 1, 0.8f }, 3.f);
	else
		draw_circle(pPos, pRadius, { 1, 1, 0, 0.8f });

	return dragging;
}

bool drag_circle(const char* pStr_id, math::vec2* pDelta, float pRadius)
{
	return drag_circle(pStr_id, *pDelta, pDelta, pRadius);
}

bool drag_rect(const char* pStr_id, const math::rect& pDisplay, math::vec2* pDelta)
{
	const ImGuiID id = ImGui::GetID(pStr_id);
	const bool hovered = pDisplay.intersects(get_mouse_position());
	const bool dragging = drag_behavior(id, hovered, pDelta);

	draw_rect(pDisplay, { 1, 1, 0, 0.5f });

	return dragging;
}

bool resizable_rect(const char* pStr_id, const math::rect& pDisplay, math::rect* pDelta)
{
	assert(gCurrent_editor_state);

	ImGui::PushID(pStr_id);

	bool dragging = false;
	math::vec2 topleft;
	if (drag_circle("TopLeft", pDisplay.position, &topleft, 6))
	{
		dragging = true;
		pDelta->position += topleft;
		pDelta->size -= topleft;
	}

	dragging |= drag_circle("BottomRight", pDisplay.get_corner(2), &pDelta->size, 6);

	math::vec2 topright;
	if (drag_circle("TopRight", pDisplay.get_corner(1), &topright, 6))
	{
		dragging = true;
		pDelta->y += topright.y;
		pDelta->width += topright.x;
		pDelta->height -= topright.y;
	}

	math::vec2 bottomleft;
	if (drag_circle("BottomLeft", pDisplay.get_corner(3), &bottomleft, 6))
	{
		dragging = true;
		pDelta->x += bottomleft.x;
		pDelta->width -= bottomleft.x;
		pDelta->height += bottomleft.y;
	}

	math::vec2 top;
	if (drag_circle("Top", pDisplay.position + math::vec2(pDisplay.width / 2, 0), &top, 6))
	{
		dragging = true;
		pDelta->y += top.y;
		pDelta->height -= top.y;
	}

	math::vec2 bottom;
	if (drag_circle("Bottom", pDisplay.position + math::vec2(pDisplay.width / 2, pDisplay.height), &bottom, 6))
	{
		dragging = true;
		pDelta->height += bottom.y;
	}

	math::vec2 left;
	if (drag_circle("Left", pDisplay.position + math::vec2(0, pDisplay.height / 2), &left, 6))
	{
		dragging = true;
		pDelta->x += left.x;
		pDelta->width -= left.x;
	}

	math::vec2 right;
	if (drag_circle("Right", pDisplay.position + math::vec2(pDisplay.width, pDisplay.height / 2), &right, 6))
	{
		dragging = true;
		pDelta->width += right.x;
	}

	ImGui::PopID();

	return dragging;
}

box_edit::box_edit(const math::rect& pRect, const math::transform& pTransform) noexcept :
	mRect(pRect),
	mTransform(pTransform)
{}

bool box_edit::resize(edit_type pType)
{
	push_transform(mTransform);

	math::rect delta;
	if (resizable_rect("edit", mRect, &delta))
	{
		if (pType == edit_type::rect)
		{
			mRect.position += delta.position;
			mRect.size += delta.size;
		}
		else if (pType == edit_type::transform)
		{
			const math::vec2 scale = (mRect.size + delta.size) / mRect.size;
			const math::vec2 position_delta = delta.position - mRect.position * (scale - math::vec2(1, 1));
			mTransform.position += visual_editor::get_transform().apply_to(position_delta, math::transform_mask::position);
			mTransform.scale *= scale;
		}
		mIs_dragging = true;
	}

	pop_transform();

	return mIs_dragging;
}

bool box_edit::drag(edit_type pType)
{
	push_transform(mTransform);

	math::vec2 delta;
	if (drag_rect("editdrag", mRect, &delta))
	{
		if (pType == edit_type::rect)
			mRect.position += delta;
		else if (pType == edit_type::transform)
			mTransform.position += mTransform.apply_to(delta, math::transform_mask::position);
		mIs_dragging = true;
	}

	pop_transform();

	return mIs_dragging;
}

const math::rect& box_edit::get_rect() const noexcept
{
	return mRect;
}

const math::transform& box_edit::get_transform() const noexcept
{
	return mTransform;
}

bool box_edit::is_dragging() const noexcept
{
	return mIs_dragging;
}

} // namespace wge::editor::visual_editor
