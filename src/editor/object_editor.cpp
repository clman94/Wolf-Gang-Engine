
#include "object_editor.hpp"
#include "script_editor.hpp"
#include "widgets.hpp"

#include <wge/filesystem/file_input_stream.hpp>

#include <array>

static const std::array event_display_name = {
		(const char*)(ICON_FA_PLUS u8" Create"),
		(const char*)(ICON_FA_STEP_FORWARD u8" Update"),
		(const char*)(ICON_FA_PENCIL u8" Draw")
};

namespace wge::editor
{

object_editor::object_editor(context& pContext, const core::asset::ptr& pAsset) noexcept :
	asset_editor(pContext, pAsset)
{}

void object_editor::on_gui()
{
	mScript_editor_dock_id = ImGui::GetID("EditorDock");

	ImGui::BeginChild("LeftPanel", ImVec2(300, 0));

	auto res = get_asset()->get_resource<core::object_resource>();
	ImGui::Dummy({ 0, 10 });
	display_sprite_input(res);
	ImGui::Dummy({ 0, 10 });
	if (ImGui::Checkbox("Collision", &res->is_collision_enabled))
		mark_asset_modified();
	ImGui::Dummy({ 0, 10 });
	display_event_list(res);

	ImGui::EndChild();

	ImGui::SameLine();

	// Event script editors are given a dedicated dockspace where they spawn. This helps
	// remove clutter windows popping up everywhere.
	ImGui::DockSpace(mScript_editor_dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);

	display_sub_editors();
}

void object_editor::display_sprite_input(core::object_resource* pGenerator)
{
	core::asset::ptr sprite = get_asset_manager().get_asset(pGenerator->display_sprite);
	if (sprite = asset_selector("SpriteSelector", "sprite", get_asset_manager(), sprite))
	{
		pGenerator->display_sprite = sprite->get_id();
		mark_asset_modified();
	}
}

void object_editor::display_event_list(core::object_resource* pGenerator)
{
	ImGui::Text("Events:");
	ImGui::BeginChild("Events", ImVec2(0, 0), true);
	for (auto&& [type, info] : util::enumerate{ pGenerator->events })
	{
		const char* event_name = event_display_name[type];
		const bool script_exists = info.id.is_valid();
		ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;
		if (ImGui::Selectable(event_name, script_exists, flags, { 0, 0 }))
		{
			if (!script_exists)
			{
				// Create a new one.
				create_event_script(type);
				mark_asset_modified();
			}
			if (auto editor = get_context().get_editor(info.id))
			{
				editor->set_dock_family_id(mScript_editor_dock_id);
				editor->show();
				editor->focus_window();

			}
		}
	}
	ImGui::EndChild();
}

void object_editor::create_event_script(std::size_t pIndex)
{
	auto name = core::object_resource::event_typenames[pIndex];
	auto new_asset = get_asset_manager().create_secondary_asset(get_asset(), name, "script");
	get_asset()->get_resource<core::object_resource>()->events[pIndex].id = new_asset->get_id();
	get_asset_manager().save_asset(new_asset);
	create_sub_editors();
}

} // namespace wge::editor
