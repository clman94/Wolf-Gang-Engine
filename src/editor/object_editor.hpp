#pragma once

#include "context.hpp"

#include <wge/core/object_resource.hpp>

#include <imgui/imgui.h>

namespace wge::editor
{

class script_editor;

class eventful_sprite_editor :
	public asset_editor
{
public:
	eventful_sprite_editor(context& pContext, const core::asset::ptr& pAsset) noexcept;
	virtual void on_gui() override;
	virtual void on_close() override;

private:
	void display_sprite_input(core::object_resource* pGenerator);
	void display_event_list(core::object_resource* pGenerator);

private:
	core::asset::ptr create_event_script(const char* pName);

	std::map<util::uuid, std::unique_ptr<script_editor>> mScript_editors;

	ImGuiID mScript_editor_dock_id = 0;
};


} // namespace wge::editor

