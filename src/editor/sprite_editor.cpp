#include "sprite_editor.hpp"
#include "widgets.hpp"
#include "imgui_editor_tools.hpp"

namespace wge::editor
{

void sprite_editor::on_gui()
{
	mController.update(1.f / 60.f);

	auto sprite = get_asset()->get_resource<graphics::sprite>();

	ImGui::BeginGroup();
	{
		if (ImGui::Button(mController.is_playing() ? (const char*)(ICON_FA_PAUSE) : (const char*)(ICON_FA_PLAY), { 50, 50 }))
			mController.toggle();
		ImGui::SameLine();
		if (ImGui::Button((const char*)(ICON_FA_BACKWARD), { 50, 50 }))
			mController.restart();
		int frame_index = static_cast<int>(mController.get_frame() + 1);
		ImGui::PushItemWidth(50 + 50 + ImGui::GetStyle().FramePadding.x * 2);
		if (ImGui::SliderInt("##FrameIndex", &frame_index, 1, static_cast<int>(sprite->get_frame_count())))
		{
			assert(frame_index >= 1);
			mController.set_frame(static_cast<std::size_t>(frame_index - 1));
		}
		ImGui::PopItemWidth();
	}
	ImGui::EndGroup();

	ImGui::SameLine();

	ImGui::BeginChild("Frames", { 0, 110 }, true, ImGuiWindowFlags_AlwaysHorizontalScrollbar);

	for (std::size_t i = 0; i < sprite->get_frame_count(); i++)
	{
		ImGui::PushID(static_cast<int>(i));
		if (mController.get_frame() == i)
			ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.2f, 0.7f, 0.2f, 0.7f });
		ImGui::BeginChild("Frame", { 100, 0 }, true);
		ImGui::Text(std::to_string(i + 1).c_str());
		preview_image("Preview", sprite->get_texture(), ImGui::GetContentRegionAvail(), sprite->get_frame_aabb(i));
		ImGui::EndChild();
		if (mController.get_frame() == i)
			ImGui::PopStyleColor();
		if (ImGui::IsItemClicked())
			mController.set_frame(i);
		ImGui::PopID();
		ImGui::SameLine();
	}
	ImGui::EndChild();

	ImGui::BeginChild("AnimationSettings", ImVec2(mAtlas_info_width, 0));
	{
		math::vec2 anchor = sprite->get_default_anchor();
		if (ImGui::DragFloat2("Anchor", anchor.components().data()))
		{
			sprite->set_default_anchor(anchor);
			mark_asset_modified();
		}
		float duration = sprite->get_default_duration();
		if (ImGui::DragFloat("Duration", &duration, 0.01f))
		{
			sprite->set_default_duration(duration);
			mark_asset_modified();
		}
		bool loop = sprite->get_loop();
		if (ImGui::Checkbox("Loop", &loop))
		{
			sprite->set_loop(loop);
			mark_asset_modified();
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::VerticalSplitter("AtlasInfoSplitter", &mAtlas_info_width);
	ImGui::SameLine();

	begin_image_editor("Editor", sprite->get_texture(), sprite->get_frame_uv(mController.get_frame()), true);

	math::vec2 anchor = sprite->get_frame_anchor(mController.get_frame());

	visual_editor::draw_line(anchor - math::vec2{ 5, 0 }, anchor + math::vec2{ 5, 0 }, { 1, 1, 1, 1 });
	visual_editor::draw_line(anchor - math::vec2{ 0, 5 }, anchor + math::vec2{ 0, 5 }, { 1, 1, 1, 1 });

	end_image_editor();
}

void sprite_editor::check_if_edited()
{
	if (ImGui::IsItemDeactivatedAfterEdit())
		mark_asset_modified();
}

} // namespace wge::editor