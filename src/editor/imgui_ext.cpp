#include <editor/imgui_ext.hpp>

#include <engine/texture.hpp>

#include <imgui-SFML.h>

#include <SFML/Graphics/RenderTexture.hpp>

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
	ImDrawList* drawlist = ImGui::GetWindowDrawList();
	engine::frect box(ImGui::GetCursorScreenPos()
		, static_cast<engine::fvector>(pRender.getSize()) + static_cast<engine::fvector>(ImGui::GetCursorPos()));
	drawlist->AddImage((void*)pRender.getTexture().getNativeHandle()
		, box.get_offset(), box.get_corner()
		, ImVec2(0, 1), ImVec2(1, 0) // Render textures store textures upsidedown so we need to flip it
		, ImGui::GetColorU32(sf::Color::White));
}

// Returns true while the user interacts with it
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

// Returns true while the user interacts with it
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

}