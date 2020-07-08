
#include "object_editor.hpp"
#include "script_editor.hpp"
#include "widgets.hpp"

#include <wge/filesystem/file_input_stream.hpp>

#include <array>

namespace wge::editor
{

object_editor::object_editor(context& pContext, const core::asset::ptr& pAsset) noexcept :
	asset_editor(pContext, pAsset)
{
	update_editor_titles();
}

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

void object_editor::open_script_editor(const core::asset_id& pId)
{
	if (auto editor = get_context().get_editor(pId))
	{
		editor->set_dock_family_id(mScript_editor_dock_id);
		editor->show();
		editor->focus_window();
	}
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
		ImGui::PushID(&info);
		const bool script_exists = info.id.is_valid();
		if (script_exists)
		{
			const std::string event_name = fmt::format("{} {}",
				scripting::event_descriptors[type].icon,
				scripting::event_descriptors[type].display_name);

			ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;
			if (ImGui::Selectable(event_name.c_str(), get_context().is_editor_open_for(info.id), flags, { 0, 0 }))
				open_script_editor(info.id);
			event_tooltop(type);
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete"))
				{
					get_context().save_asset(get_asset_manager().get_asset(info.id));
					get_context().close_editor(info.id);
					info.id = core::asset_id{};
					mark_asset_modified();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::PopID();
	}

	if (ImGui::BeginCombo("Add Event", nullptr, ImGuiComboFlags_NoPreview))
	{
		const auto event_menu_item = [&](auto pEvent_type)
		{
			constexpr auto descriptor_index = scripting::get_event_descriptor_index(pEvent_type);
			constexpr auto descriptor = scripting::get_event_descriptor(pEvent_type);
			if (ImGui::MenuItem(descriptor->display_name, nullptr, false, !pGenerator->events[descriptor_index].id.is_valid()))
				create_event_script(descriptor_index);
			event_tooltop(descriptor_index);
		};

		using namespace scripting::event_selector;

		event_menu_item(create{});

		if (ImGui::BeginMenu("Update"))
		{
			event_menu_item(preupdate{});
			event_menu_item(update{});
			event_menu_item(postupdate{});
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Rendering"))
		{
			event_menu_item(draw{});
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Alarm"))
		{
			event_menu_item(alarm_1{});
			event_menu_item(alarm_2{});
			event_menu_item(alarm_3{});
			event_menu_item(alarm_4{});
			event_menu_item(alarm_5{});
			event_menu_item(alarm_6{});
			event_menu_item(alarm_7{});
			event_menu_item(alarm_8{});
			ImGui::EndMenu();
		}
		ImGui::EndCombo();
	}

	ImGui::EndChild();
}

void object_editor::update_editor_titles()
{
	for (auto&& [i, info] : util::enumerate{ get_asset()->get_resource<core::object_resource>()->events })
	{
		if (info.id.is_valid())
		{
			if (auto editor = get_context().get_editor(info.id))
			{
				editor->set_title(
					fmt::format("{} [{}]",
						scripting::event_descriptors[i].display_name,
						get_asset()->get_name()));
			}
		}
	}
}

void object_editor::event_tooltop(std::size_t pIndex) const
{
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(400);
		ImGui::TextWrapped(scripting::event_descriptors[pIndex].tip);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void object_editor::create_event_script(std::size_t pIndex)
{
	auto resource = get_asset()->get_resource<core::object_resource>();
	if (resource->events[pIndex].id.is_valid())
		return;

	const auto name = scripting::event_descriptors[pIndex].serialize_name;

	if (auto existing = get_asset_manager().get_asset(get_asset_manager().get_asset_path(get_asset()) / name))
	{
		assert(existing->get_type() == "script");
		log::info("Reusing existing asset '{}' for event", existing->get_name());
		resource->events[pIndex].id = existing->get_id();
		update_editor_titles();
		open_script_editor(existing->get_id());
	}
	else
	{
		auto new_asset = get_asset_manager().create_secondary_asset(get_asset(), name, "script");
		new_asset->get_resource<scripting::script>()->source =
			fmt::format("-- Event: {}\n\n", scripting::event_descriptors[pIndex].display_name);
		resource->events[pIndex].id = new_asset->get_id();
		get_asset_manager().save_asset(new_asset);
		create_sub_editors();
		update_editor_titles();
		open_script_editor(new_asset->get_id());
	}
	mark_asset_modified();
}

} // namespace wge::editor
