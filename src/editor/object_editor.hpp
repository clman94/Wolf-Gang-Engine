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
	void open_script_editor(const core::asset_id& pId);
	void display_sprite_input(core::object_resource* pGenerator);
	void display_event_list(core::object_resource* pGenerator);
	void update_editor_titles();
	void event_tooltop(std::size_t pIndex) const;

private:
	void create_event_script(std::size_t pIndex);

	float mLeft_panel_width = 300;
	ImGuiID mScript_editor_dock_id = 0;
};

} // namespace wge::editor
