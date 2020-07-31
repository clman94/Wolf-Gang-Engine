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
		preview_image("Preview", sprite->get_texture(), ImGui::GetContentRegionAvail(), sprite->get_frame_uv(i));
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

		if (ImGui::CollapsingHeader("Collision"))
		{
			math::vec2 min = sprite->get_aabb_collision().min;
			if (ImGui::DragFloat2("Minimum", min.components().data(), 1, 0, static_cast<float>(sprite->get_frame_size().x)))
			{
				sprite->set_aabb_collision({ min , sprite->get_aabb_collision().max });
				mark_asset_modified();
			}

			math::vec2 max = sprite->get_aabb_collision().max;
			if (ImGui::DragFloat2("Maximum", max.components().data(), 1, 0, static_cast<float>(sprite->get_frame_size().y)))
			{
				sprite->set_aabb_collision({ sprite->get_aabb_collision().min , max });
				mark_asset_modified();
			}
		}

		if (ImGui::CollapsingHeader("Info"))
		{
			ImGui::TextUnformatted(fmt::format("Frame Width: {}\nFrame Height: {}\nTexture Width: {}\nTexture Height: {}\nMemory Usage: {} bytes",
				sprite->get_frame_width(),
				sprite->get_frame_height(),
				sprite->get_texture().get_width(),
				sprite->get_texture().get_height(),
				sprite->get_texture().get_width() * sprite->get_texture().get_height() * 4).c_str());
;		}
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::VerticalSplitter("AtlasInfoSplitter", &mAtlas_info_width);
	ImGui::SameLine();

	begin_image_editor("Editor", sprite->get_texture(), sprite->get_frame_uv(mController.get_frame()), true);

	visual_editor::draw_rect(sprite->get_aabb_collision(), { 0.2f, 1, 1, 0.7f });
	visual_editor::box_edit collision_edit(sprite->get_aabb_collision());
	collision_edit.drag(visual_editor::edit_type::rect);
	collision_edit.resize(visual_editor::edit_type::rect);
	if (collision_edit.is_dragging())
	{
		sprite->set_aabb_collision(collision_edit.get_rect());
		mark_asset_modified();
	}

	// Display anchor.
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
