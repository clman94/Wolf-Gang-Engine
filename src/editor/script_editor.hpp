#pragma once

#include "context.hpp"

#include <imgui/TextEditor.h>

namespace wge::editor
{

class script_editor :
	public asset_editor
{
public:
	script_editor(context& pContext, const core::asset::ptr& pAsset);
	virtual void on_gui() override;

private:
	void update_error_markers();

private:
	scripting::error_info mLast_error_info;
	TextEditor::ErrorMarkers mError_markers;
	TextEditor mText_editor;
};

} // namespace wge::editor
