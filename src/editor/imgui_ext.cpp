#include <editor/imgui_ext.hpp>

#include <engine/texture.hpp>

#include <imgui_internal.h>
#include <imgui-SFML.h>

#include <SFML/Graphics/RenderTexture.hpp>

// See editor.cpp; this is temprary
bool resize_to_window(sf::RenderTexture& pRender);
void draw_grid(engine::primitive_builder& pPrimitives, engine::fvector pAlign_to, engine::fvector pScale, engine::fvector pDisplay_size, engine::color pColor);

namespace ImGui
{

// This button only shows its frame when hovered or clicked
// This contrasts to InvisibleButton where it is always hidden.
bool HiddenButton(const char* pName, ImVec2 pSize)
{
	ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
	bool pressed = ImGui::Button(pName, pSize);
	ImGui::PopStyleColor();
	return pressed;
}

// This button only shows its frame when hovered or clicked
// This contrasts to InvisibleButton where it is always hidden.
bool HiddenSmallButton(const char* pName)
{
	ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
	bool pressed = ImGui::SmallButton(pName);
	ImGui::PopStyleColor();
	return pressed;
}

void AddBackgroundImage(sf::RenderTexture& pRender)
{
	AddBackgroundImage(pRender, ImGui::GetCursorScreenPos());
}

void AddBackgroundImage(sf::RenderTexture & pRender, ImVec2 pPosition)
{
	ImDrawList* drawlist = ImGui::GetWindowDrawList();
	engine::frect box(pPosition
		, static_cast<engine::fvector>(pRender.getSize()) /*+ static_cast<engine::fvector>(ImGui::GetCursorPos())*/);
	drawlist->AddImage((void*)pRender.getTexture().getNativeHandle()
		, box.get_offset(), box.get_corner()
		, ImVec2(0, 1), ImVec2(1, 0) // Render textures store textures upsidedown so we need to flip it
		, ImGui::GetColorU32(sf::Color::White));
}

// Returns true while the user is interacting with it
bool VSplitter(const char* pId, float pWidth, float* pX, bool pInverted)
{
	assert(pX);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::HiddenButton(pId, ImVec2(pWidth, ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y));
	ImGui::PopStyleVar();
	if (ImGui::IsItemActive())
		*pX += ImGui::GetIO().MouseDelta.x * (pInverted ? -1 : 1);
	return ImGui::IsItemActive();
}

// Returns true while the user is interacting with it
bool HSplitter(const char* pId, float pHeight, float* pY, bool pInverted)
{
	assert(pY);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::HiddenButton(pId, ImVec2(ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x, pHeight));
	ImGui::PopStyleVar();
	if (ImGui::IsItemActive())
		*pY += ImGui::GetIO().MouseDelta.y * (pInverted ? -1 : 1);
	return ImGui::IsItemActive();
}

// Construct a string representing the name and id "name###id"
std::string NameId(const std::string& pName, const std::string& pId)
{
	return pName + "###" + pId;
}

// Construct a string representing the id "###id"
std::string IdOnly(const std::string& pId)
{
	return "###" + pId;
}

// Shortcut for adding a text only tooltip to the last item
void QuickTooltip(const char * pString)
{
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.f);
		ImGui::TextUnformatted(pString);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// Returns 1 if the user pressed "Yes" and 2 if "No"
int ConfirmPopup(const char * pName, const char* pMessage)
{
	int answer = 0;
	if (ImGui::BeginPopupModal(pName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted(pMessage);
		if (ImGui::Button("Yes", ImVec2(100, 25)))
		{
			answer = 1;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(100, 25)))
		{
			answer = 2;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	return answer;
}

bool TextureSelectCombo(const char* pName, const engine::resource_manager& pRes_mgr, std::string* pTexture_name)
{
	assert(pTexture_name);
	bool ret = false;
	if (ImGui::BeginCombo(pName, pTexture_name->empty() ? "No Texture" : pTexture_name->c_str()))
	{
		if (ImGui::Selectable("No Texture", pTexture_name->empty()))
			pTexture_name->clear();
		for (auto& i : pRes_mgr.get_resources_with_type(engine::texture_restype))
		{
			if (ImGui::Selectable(i->get_name().c_str(), *pTexture_name == i->get_name()))
			{
				*pTexture_name = i->get_name();
				ret = true;
			}
		}
		ImGui::EndCombo();
	}
	return ret;
}
struct RendererData
{
	RendererData()
	{
		zoom = 0;
		renderTexture.create(10, 10);
		renderer.set_target_render(renderTexture);
		worldNode.set_parent(centerNode);
	}

	engine::primitive_builder primitives;
	engine::renderer renderer;
	engine::node centerNode, worldNode;
	sf::RenderTexture renderTexture;
	float zoom;
	ImVec2 windowTopLeft;
};

static RendererData* currentRenderData = nullptr;
static bool usingRenderer = false;

void OpenRenderer(RendererData** pRendererData)
{
	*pRendererData = new RendererData();
}

void CloseRenderer(RendererData ** pRendererData)
{
	delete *pRendererData;
	*pRendererData = nullptr;
}

void BeginRenderer(const char* pStr_id, RendererData* pRenderData, ImVec2 pSize, ImGuiRendererFlags pFlags)
{
	assert(!currentRenderData && !usingRenderer);
	assert(pRenderData);
	currentRenderData = pRenderData;

	ImGui::BeginChild(pStr_id, pSize);

	if (resize_to_window(pRenderData->renderTexture))
	{
		engine::fvector new_size = engine::vector_cast<float, unsigned int>(pRenderData->renderTexture.getSize());
		if (pFlags & ImGuiRendererFlags_ResizeTarget)
			pRenderData->renderer.set_target_size(new_size);
		pRenderData->renderer.refresh();
		pRenderData->centerNode.set_position(new_size / 2);
	}

	pRenderData->windowTopLeft = ImGui::GetCursorScreenPos();

	ImGui::InvisibleButton("interactbutton", ImVec2(-1, -1));

	// Handle mouse interaction with view
	if (ImGui::IsItemHovered())
	{
		if ((pFlags & ImGuiRendererFlags_MiddleMousePanning) && ImGui::IsMouseDown(2))
		{
			pRenderData->worldNode.set_position(pRenderData->worldNode.get_position()
				+ (engine::fvector(ImGui::GetIO().MouseDelta) / pRenderData->worldNode.get_unit()) // Delta has to be scaled to ingame coords
				/ pRenderData->worldNode.get_absolute_scale()); // Then scaled again to fit the zoom
		}

		if ((pFlags & ImGuiRendererFlags_MiddleMouseZooming) && ImGui::GetIO().MouseWheel != 0)
		{
			pRenderData->zoom += ImGui::GetIO().MouseWheel;
			pRenderData->zoom = util::clamp<float>(pRenderData->zoom, -2, 5);
			pRenderData->centerNode.set_scale(engine::fvector(1, 1)*std::pow(2.f, pRenderData->zoom));
		}
	}
	currentRenderData->renderTexture.clear(currentRenderData->renderer.get_background_color());
}

void EndRenderer()
{
	assert(currentRenderData);
	if (!usingRenderer)
	{
		currentRenderData->renderer.draw(false);
		currentRenderData->primitives.draw_and_clear(currentRenderData->renderer);
		currentRenderData->renderTexture.display();
		ImGui::AddBackgroundImage(currentRenderData->renderTexture, currentRenderData->windowTopLeft);

		ImGui::EndChild();
	}
	currentRenderData = nullptr;
	usingRenderer = false;
}

void UseRenderer(RendererData * pRenderData)
{
	assert(!currentRenderData && pRenderData);
	currentRenderData = pRenderData;
	usingRenderer = true;
}

engine::renderer & GetCurrentRenderer()
{
	assert(currentRenderData);
	return currentRenderData->renderer;
}

engine::primitive_builder & GetRendererPrimitives()
{
	assert(currentRenderData);
	return currentRenderData->primitives;
}

void SetRendererUnit(float pPixels)
{
	assert(currentRenderData);
	currentRenderData->worldNode.set_unit(pPixels);
}

float GetRendererUnit()
{
	return currentRenderData->worldNode.get_unit();
}

engine::fvector GetRendererMouse()
{
	assert(currentRenderData);
	const engine::ivector window_mouse_position = static_cast<engine::ivector>(ImGui::GetMousePos()) - static_cast<engine::ivector>(currentRenderData->windowTopLeft);
	return currentRenderData->renderer.window_to_game_coords(window_mouse_position);
}

engine::fvector GetRendererWorldMouse()
{
	assert(currentRenderData);
	const engine::ivector window_mouse_position = static_cast<engine::ivector>(ImGui::GetMousePos()) - static_cast<engine::ivector>(currentRenderData->windowTopLeft);
	return currentRenderData->renderer.window_to_game_coords(window_mouse_position, currentRenderData->worldNode);
}

void SetRendererBackground(const engine::color & pColor)
{
	assert(currentRenderData);
	currentRenderData->renderer.set_background_color(pColor);
}

engine::color GetRendererBackground()
{
	assert(currentRenderData);
	return currentRenderData->renderer.get_background_color();
}

void SetRendererPan(engine::fvector pPos)
{
	assert(currentRenderData);
	currentRenderData->worldNode.set_position(pPos);
}

engine::fvector GetRendererSize()
{
	assert(currentRenderData);
	return currentRenderData->renderTexture.getSize();
}

engine::node & GetRendererWorldNode()
{
	assert(currentRenderData);
	return currentRenderData->worldNode;
}

namespace RenderWidgets
{

static hash::hash32_t activeDraggerId = 0;

bool IsDragging()
{
	return activeDraggerId != 0;
}

static inline bool DragBehavior(hash::hash32_t pId, bool pHovered)
{
	assert(currentRenderData && !usingRenderer);
	const bool dragging = activeDraggerId == pId;
	const bool moving = engine::fvector(ImGui::GetIO().MouseDelta) != engine::fvector(0, 0);
	if (pHovered && ImGui::IsItemClicked(0))
	{
		activeDraggerId = pId; // Start drag
		return true;
	}
	else if (!ImGui::IsMouseDown(0) && dragging)
	{
		activeDraggerId = 0; // End drag
		return true; // Return true for one more frame after the mouse is released
	}
	return dragging;
}

bool CircleDragger(hash::hash32_t pId, const engine::fvector& pPosition, engine::fvector* pChange, bool pMove_X, bool pMove_Y)
{
	assert(currentRenderData && !usingRenderer);
	const bool hovered = pPosition.distance(ImGui::GetRendererWorldMouse()) < 3.f / (GetRendererUnit()*currentRenderData->centerNode.get_scale().x);
	const bool dragging = DragBehavior(pId, hovered);
	currentRenderData->primitives.add_circle(engine::exact_relative_to_node(pPosition, currentRenderData->worldNode), 3.f, { 1, 1, 1, (hovered || dragging ? 1.f : 0.f) }, { 1, 1, 1, 1 });
	if (dragging)
	{
		pChange->x += ImGui::GetIO().MouseDelta.x*(pMove_X ? 1 : 0) / (GetRendererUnit()*currentRenderData->centerNode.get_scale().x);
		pChange->y += ImGui::GetIO().MouseDelta.y*(pMove_Y ? 1 : 0) / (GetRendererUnit()*currentRenderData->centerNode.get_scale().y);
	}
	return dragging;
}

bool CircleDragger(const char * pStr_id, const engine::fvector& pPosition, engine::fvector* pChange, bool pMove_X, bool pMove_Y)
{
	assert(currentRenderData && !usingRenderer);
	return CircleDragger(hash::hash32(pStr_id), pPosition, pChange, pMove_X, pMove_Y);
}

bool RectDragger(hash::hash32_t pId, const engine::frect& pRect, engine::frect* pChange)
{
	assert(currentRenderData && !usingRenderer);
	const bool hovered = pRect.is_intersect(ImGui::GetRendererWorldMouse());
	const bool dragging = DragBehavior(pId, hovered);
	if (dragging)
	{
		pChange->x += ImGui::GetIO().MouseDelta.x / (GetRendererUnit()*currentRenderData->centerNode.get_scale().x);
		pChange->y += ImGui::GetIO().MouseDelta.y / (GetRendererUnit()*currentRenderData->centerNode.get_scale().y);
	}
	return dragging;
}

bool RectDragger(const char * pStr_id, const engine::frect& pRect, engine::frect* pChange)
{
	assert(currentRenderData && !usingRenderer);
	return RectDragger(hash::hash32(pStr_id), pRect, pChange);
}

bool RectResizer(hash::hash32_t pId, const engine::frect& pRect, engine::frect* pChange)
{
	assert(currentRenderData && !usingRenderer);
	bool dragging = false;

	const engine::fvector top_pos = pRect.get_offset() + engine::fvector(pRect.w / 2, 0);
	engine::fvector top_changed;
	if (CircleDragger(hash::combine(hash::hash32("top_move"), pId), top_pos, &top_changed, false, true))
	{
		pChange->y += top_changed.y;
		pChange->h -= top_changed.y;
		dragging = true;
	}

	const engine::fvector bottom_pos = pRect.get_offset() + engine::fvector(pRect.w / 2, pRect.h);
	engine::fvector bottom_changed;
	if (CircleDragger(hash::combine(hash::hash32("bottom_move"), pId), bottom_pos, &bottom_changed, false, true))
	{
		pChange->h += bottom_changed.y;
		dragging = true;
	}

	const engine::fvector left_pos = pRect.get_offset() + engine::fvector(0, pRect.h / 2);
	engine::fvector left_changed;
	if (CircleDragger(hash::combine(hash::hash32("left_move"), pId), left_pos, &left_changed, true, false))
	{
		pChange->x += left_changed.x;
		pChange->w -= left_changed.x;
		dragging = true;
	}

	const engine::fvector right_pos = pRect.get_offset() + engine::fvector(pRect.w, pRect.h / 2);
	engine::fvector right_changed;
	if (CircleDragger(hash::combine(hash::hash32("right_move"), pId), right_pos, &right_changed, true, false))
	{
		pChange->w += right_changed.x;
		dragging = true;
	}

	engine::fvector top_left_changed;
	if (CircleDragger(hash::combine(hash::hash32("top_left_move"), pId), pRect.get_vertex(0), &top_left_changed, true, true))
	{
		pChange->x += top_left_changed.x;
		pChange->y += top_left_changed.y;
		pChange->w -= top_left_changed.x;
		pChange->h -= top_left_changed.y;
		dragging = true;
	}

	engine::fvector top_right_changed;
	if (CircleDragger(hash::combine(hash::hash32("top_right_move"), pId), pRect.get_vertex(1), &top_right_changed, true, true))
	{
		pChange->y += top_right_changed.y;
		pChange->w += top_right_changed.x;
		pChange->h -= top_right_changed.y;
		dragging = true;
	}

	engine::fvector bottom_right_changed;
	if (CircleDragger(hash::combine(hash::hash32("bottom_right_move"), pId), pRect.get_vertex(2), &bottom_right_changed, true, true))
	{
		pChange->w += bottom_right_changed.x;
		pChange->h += bottom_right_changed.y;
		dragging = true;
	}

	engine::fvector bottom_left_changed;
	if (CircleDragger(hash::combine(hash::hash32("bottom_left_move"), pId), pRect.get_vertex(3), &bottom_left_changed, true, true))
	{
		pChange->x += bottom_left_changed.x;
		pChange->w -= bottom_left_changed.x;
		pChange->h += bottom_left_changed.y;
		dragging = true;
	}

	return dragging;
}

bool RectResizer(const char * pStr_id, const engine::frect& pRect, engine::frect* pChange)
{
	assert(currentRenderData && !usingRenderer);
	return RectResizer(hash::hash32(pStr_id), pRect, pChange);
}

bool RectDraggerAndResizer(const char * pStr_id, const engine::frect& pRect, engine::frect* pChange)
{
	assert(currentRenderData && !usingRenderer);
	hash::hash32_t id = hash::hash32(pStr_id);
	return RectResizer(hash::combine(hash::hash32("resizer"), id), pRect, pChange)
		|| RectDragger(hash::combine(hash::hash32("dragger"), id), pRect, pChange);
}

void Grid(const engine::color & pColor)
{
	assert(currentRenderData && !usingRenderer);
	engine::fvector offset = currentRenderData->worldNode.get_exact_position();
	engine::fvector scale = currentRenderData->worldNode.get_absolute_scale()*currentRenderData->worldNode.get_unit();
	engine::fvector display_size = engine::vector_cast<float, unsigned int>(currentRenderData->renderTexture.getSize());
	draw_grid(currentRenderData->primitives, offset, scale, display_size, pColor);
}

}

}