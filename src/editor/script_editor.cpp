#include "script_editor.hpp"

namespace wge::editor
{

script_editor::script_editor(context& pContext, const core::asset::ptr& pAsset) :
	asset_editor(pContext, pAsset)
{
	auto source = get_asset()->get_resource<scripting::script>();
	mText_editor.SetText(source->source);
	mText_editor.SetPalette(TextEditor::GetDarkPalette());
	mText_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
	mText_editor.SetShowWhitespaces(false);
	source->parse_function_list();
}

void script_editor::on_gui()
{
	update_error_markers();
	auto source = get_asset()->get_resource<scripting::script>();
	if (ImGui::BeginCombo("##Functions", fmt::format("{} Function(s)", source->function_list.size()).c_str()))
	{
		for (auto&& [line, name] : source->function_list)
		{
			if (ImGui::MenuItem(fmt::format("{} [Line: {}]", name, line).c_str()))
			{
				TextEditor::Coordinates coord;
				coord.mColumn = 0;
				coord.mLine = line - 1;
				mText_editor.SetCursorPosition(coord);
			}
		}
		ImGui::EndCombo();
	}

	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	mText_editor.Render("Text");
	if (mText_editor.IsTextChanged())
	{
		source->parse_function_list();
		source->source = mText_editor.GetText();
		get_context().get_engine().get_script_engine().prepare_recompile(get_asset());
		mark_asset_modified();
	}
	ImGui::PopFont();
}

void script_editor::update_error_markers()
{
	mError_markers.clear();

	auto& script_engine = get_context().get_engine().get_script_engine();
	auto source = get_asset()->get_resource<scripting::script>();
	auto error_info = script_engine.get_script_error(get_asset()->get_id());
	if (error_info)
	{
		mLast_error_info = *error_info;
		mError_markers[error_info->line] = error_info->message;
	}

	for (auto& [obj_id, rt_info] : script_engine.get_runtime_errors())
	{
		if (rt_info.asset_id == get_asset()->get_id())
		{
			auto obj = get_context().get_engine().get_scene().get_object(obj_id);
			auto& line = mError_markers[rt_info.line];
			bool was_empty = line.empty();
			line += fmt::format("{} [{} id:{}] {}", obj.get_name(), obj.get_asset()->get_name(), obj.get_id(), rt_info.message);
			if (!was_empty)
				line += "\n";
		}
	}

	mText_editor.SetErrorMarkers(mError_markers);
}

} // namespace wge::editor
