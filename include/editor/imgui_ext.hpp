
#include <string>

#include <imgui.h>

#include <engine/rect.hpp>
#include <engine/renderer.hpp>
#include <engine/primitive_builder.hpp>

namespace engine
{
class resource_manager;
}

namespace sf
{
class RenderTexture;
}

typedef int ImGuiRendererFlags;

enum ImGuiRendererFlags_
{
	ImGuiRendererFlags_NoFlags = 0,
	ImGuiRendererFlags_MiddleMousePanning = 1 << 0,
	ImGuiRendererFlags_MiddleMouseZooming = 1 << 1,
	ImGuiRendererFlags_ResizeTarget = 1 << 2, // The renderers target will be resized to fit the window
	ImGuiRendererFlags_Editor = ImGuiRendererFlags_MiddleMousePanning | ImGuiRendererFlags_MiddleMouseZooming | ImGuiRendererFlags_ResizeTarget,
};

namespace ImGui
{

bool InputText(const char* pLabel, std::string* pString, ImGuiInputTextFlags pFlags = 0, ImGuiTextEditCallback pCallback = NULL, void* pUser_data = NULL);

// This button only shows its frame when hovered or clicked
// This contrasts to InvisibleButton where it is always hidden.
bool HiddenButton(const char* pName, ImVec2 pSize = ImVec2(0, 0));
bool HiddenSmallButton(const char* pName);

// Displays a texture without moving the imgui cursor
void AddBackgroundImage(sf::RenderTexture& pRender);
void AddBackgroundImage(sf::RenderTexture& pRender, ImVec2 pPosition);

// Returns true while the user interacts with it
bool VSplitter(const char* pId, float pWidth, float* pX, bool pInverted = false);
bool HSplitter(const char* pId, float pHeight, float* pY, bool pInverted = false);

// Construct a string representing the name and id: "name###id"
std::string NameId(const std::string& pName, const std::string& pId);

// Construct a string representing the id: "###id"
std::string IdOnly(const std::string& pId);

// Shortcut for adding a text only tooltip to the last item
void QuickTooltip(const char * pString);

// Returns 1 if the user pressed "Yes" and 2 if "No"
int ConfirmPopup(const char * pName, const char* pMessage);

// Makes a list of all textures recognized by the resource manager and sets pTexture_name to the currently selected one.
// pTexture_name is empty if "No Texture" is selected.
bool TextureSelectCombo(const char* pName, const engine::resource_manager& pRes_mgr, std::string* pTexture_name);

struct RendererData;

void OpenRenderer(RendererData** pRendererData); // Creates the RendererData object
void CloseRenderer(RendererData** pRendererData); // Frees the RendererData object
void BeginRenderer(const char* pStr_id, RendererData* pRenderData, ImVec2 pSize = { 0.f, 0.f }, ImGuiRendererFlags pFlags = 0); // Always call EndRenderer() regardless of return value. Returns true if it is a first time run.
void EndRenderer(); // Everything will be rendered here, do all drawing and handle events before calling this.
void UseRenderer(RendererData* pRenderData); // Technically the same as BeginRenderer() but without the rendering. Remember to call EndRenderer()

engine::renderer&          GetCurrentRenderer();
engine::primitive_builder& GetRendererPrimitives();
void                       SetRendererUnit(float pPixels);
float                      GetRendererUnit();
engine::fvector            GetRendererMouse();
engine::fvector            GetRendererWorldMouse();
void                       SetRendererBackground(const engine::color& pColor);
engine::color              GetRendererBackground();
void                       SetRendererPan(engine::fvector pPos);
engine::fvector            GetRendererSize();
engine::node&              GetRendererWorldNode();
float                      GetRendererZoom();

namespace RenderWidgets
{

bool IsDragging();
bool CircleDragger(const char* pStr_id, const engine::fvector& pPosition, engine::fvector* pChange, bool pMove_X = true, bool pMove_Y = true);
bool RectDragger(const char* pStr_id, const engine::frect& pRect, engine::frect* pChange);
bool RectResizer(const char* pStr_id, const engine::frect& pRect, engine::frect* pChange);
bool RectDraggerAndResizer(const char* pStr_id, const engine::frect& pRect, engine::frect* pChange);

void Grid(const engine::color& pColor); // By default, the renderer unit (GetRendererUnit()) is used for the cell size

}

}
