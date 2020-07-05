#pragma once

#include "context.hpp"

#include <wge/core/object_resource.hpp>

#include <imgui/imgui.h>

namespace wge::editor
{

class script_editor;

class object_editor :
	public asset_editor
{
public:
	object_editor(context& pContext, const core::asset::ptr& pAsset) noexcept;
	virtual void on_gui() override;

private:
	void display_sprite_input(core::object_resource* pGenerator);
	void display_event_list(core::object_resource* pGenerator);

private:
	void create_event_script(std::size_t pIndex);

	ImGuiID mScript_editor_dock_id = 0;
};

} // namespace wge::editor
