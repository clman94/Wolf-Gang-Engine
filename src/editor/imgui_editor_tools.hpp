#pragma once

#include <wge/math/rect.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/aabb.hpp>
#include <wge/graphics/color.hpp>
#include <wge/math/transform.hpp>

#include "imgui_ext.hpp"

namespace wge::editor::visual_editor
{

void begin(const char* pStr_id, const math::vec2& pCursor_offset, const math::vec2& pOffset, const math::vec2& pScale);
void end();

void begin_snap(const math::vec2& pRatio = { 1, 1 });
void end_snap();

void push_transform(const math::transform& pTransform);
void pop_transform(std::size_t pCount = 1);
const math::transform& get_transform() noexcept;
math::vec2 calc_absolute(const math::vec2&);

math::vec2 snap_behavior(const math::vec2& pDelta);

// This is affected by the transformation stack
math::vec2 get_mouse_delta();

void draw_circle(const math::vec2& pCenter, float pRadius, const graphics::color& pColor, float pThickness, bool pScale_radius = false);
void draw_circle(const math::vec2& pCenter, float pRadius, const graphics::color& pColor, bool pScale_radius = false);
void draw_rect(const math::aabb& pAABB, const graphics::color& pColor);
void draw_rect(const math::rect& pRect, const graphics::color& pColor);
void draw_line(const math::vec2& pP0, const math::vec2& pP1, const graphics::color& pColor, float pThickness = 1);
void draw_grid(const graphics::color& pColor, float pSquare_size);

math::vec2 get_mouse_position();

bool drag_behavior(ImGuiID pID, bool pHovered);
bool drag_behavior(ImGuiID pID, bool pHovered, float* pX, float* pY);
bool drag_behavior(ImGuiID pID, bool pHovered, math::vec2* pVec);
bool is_dragging();

bool drag_circle(const char* pStr_id, math::vec2 pPos, math::vec2* pDelta, float pRadius);
bool drag_circle(const char* pStr_id, math::vec2* pDelta, float pRadius);
bool drag_rect(const char* pStr_id, const math::rect& pDisplay, math::vec2* pDelta);
bool resizable_rect(const char* pStr_id, const math::rect& pDisplay, math::rect* pDelta);

enum class edit_type
{
	rect,
	transform,
};

class box_edit
{
public:
	box_edit(const math::rect& pRect, const math::transform& pTransform = math::transform{}) noexcept;

	bool resize(edit_type pType);
	bool drag(edit_type pType);

	const math::rect& get_rect() const noexcept;
	const math::transform& get_transform() const noexcept;

	bool is_dragging() const noexcept;

private:
	math::rect mRect;
	math::transform mTransform;
	bool mIs_dragging{ false };
};

} // namespace wge::editor::visual_editor
