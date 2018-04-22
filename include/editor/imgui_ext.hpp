
#include <string>

#include <imgui.h>

#include <engine/rect.hpp>

namespace engine
{
class resource_manager;
}

namespace sf
{
class RenderTexture;
}

namespace ImGui
{
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

// Construct a string representing the name and id "name###id"
std::string NameId(const std::string& pName, const std::string& pId);

// Construct a string representing the id "###id"
std::string IdOnly(const std::string& pId);

// Shortcut for adding a text only tooltip to the last item
void QuickTooltip(const char * pString);

// Returns 1 if the user pressed "Yes" and 2 if "No"
int ConfirmPopup(const char * pName, const char* pMessage);

// Makes a list of all textures recognized by the resource manager and sets pTexture_name to the currently selected one.
// pTexture_name is empty if "No Texture" is selected.
bool TextureSelectCombo(const char* pName, const engine::resource_manager& pRes_mgr, std::string* pTexture_name);

}
