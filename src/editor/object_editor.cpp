
#include "object_editor.hpp"
#include "script_editor.hpp"
#include "widgets.hpp"

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

	std::string mut_name = get_asset()->get_name();
	if (ImGui::InputText("Name", &mut_name))
	{
		get_asset()->set_name(mut_name);
		mark_asset_modified();
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		get_asset()->set_name(scripting::make_valid_identifier(get_asset()->get_name()));
	}

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

	for (auto&& [_, editor] : mScript_editors)
	{
		if (editor)
			editor->set_visible(get_context().draw_editor(*editor));
	}
}

void object_editor::on_close()
{
	auto generator = get_asset()->get_resource<core::object_resource>();
	for (auto& i : generator->events)
		if (i.is_valid())
			get_context().close_editor(i);
}

inline void object_editor::display_sprite_input(core::object_resource* pGenerator)
{
	core::asset::ptr sprite = get_asset_manager().get_asset(pGenerator->display_sprite);
	if (sprite = asset_selector("SpriteSelector", "sprite", get_asset_manager(), sprite))
	{
		pGenerator->display_sprite = sprite->get_id();
		mark_asset_modified();
	}
}

inline void object_editor::display_event_list(core::object_resource* pGenerator)
{
	ImGui::Text("Events:");
	ImGui::BeginChild("Events", ImVec2(0, 0), true);
	for (auto [type, asset_id] : util::enumerate{ pGenerator->events })
	{
		const char* event_name = event_display_name[type];
		const bool script_exists = asset_id.is_valid();
		ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;
		if (ImGui::Selectable(event_name, script_exists, flags, { 0, 0 }))
		{
			core::asset::ptr asset;
			if (script_exists)
			{
				// Use existing asset.
				asset = get_asset_manager().get_asset(asset_id);
			}
			else
			{
				// Create a new one.
				asset = create_event_script(pGenerator->event_typenames[type]);
				asset_id = asset->get_id();
				mark_asset_modified();
			}
			assert(asset);
			auto& editor = mScript_editors[asset->get_id()];
			if (!editor)
				editor = std::make_unique<script_editor>(get_context(), asset);
			editor->set_dock_family_id(mScript_editor_dock_id);
			editor->focus_window();
			editor->set_visible(true);
		}
	}
	ImGui::EndChild();
}

inline core::asset::ptr object_editor::create_event_script(const char* pName)
{
	core::asset_manager& asset_manager = get_context().get_engine().get_asset_manager();

	auto asset = std::make_shared<core::asset>();
	asset->set_name(pName);
	asset->set_parent(get_asset());
	asset->set_type("script");
	asset_manager.store_asset(asset);

	asset->set_resource(asset_manager.create_resource("script"));
	auto script = asset->get_resource<scripting::script>();
	script->set_location(asset->get_location());
	asset->save();

	asset_manager.add_asset(asset);

	return asset;
}

} // namespace wge::editor
