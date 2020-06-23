
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
		if (i.id.is_valid())
			get_context().close_editor(i.id);
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
			if (info.id.is_valid() && info.handle.is_valid())
			{
				auto& editor = mScript_editors[info.id];
				if (!editor)
					editor = std::make_unique<script_editor>(get_context(), info.handle.get_asset());
				editor->set_dock_family_id(mScript_editor_dock_id);
				editor->focus_window();
				editor->set_visible(true);
			}
			else
			{
				log::error("Couldn't open editor for event.");
			}
		}
	}
	ImGui::EndChild();
}

void object_editor::create_event_script(std::size_t pIndex)
{
	core::asset_manager& asset_manager = get_context().get_engine().get_asset_manager();

	auto name = core::object_resource::event_typenames[pIndex];

	try {
		// Generate a file.
		filesystem::file_stream out;
		out.open(get_asset()->get_location()->get_file(std::string(name) + ".lua"), filesystem::stream_access::write);
		out.write(fmt::format("-- Event: {}\n\n", name));
		out.close();
	}
	catch (const filesystem::io_error& e)
	{
		log::error("Couldn't generate file for event.");
		log::error("io_error: {}", e.what());
		return;
	}

	// Generate a uuid for this event.
	auto resource = get_asset()->get_resource<core::object_resource>();
	resource->events[pIndex].id = util::generate_uuid();

	// Make sure all the assets are created.
	core::create_object_script_assets(get_asset(), get_asset_manager());
}

} // namespace wge::editor
