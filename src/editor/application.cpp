
#include <wge/editor/application.hpp>

#include <wge/logging/log.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/core/scene.hpp>
#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/graphics/sprite_component.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/exception.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/util/unique_names.hpp>
#include <wge/graphics/graphics.hpp>
#include <wge/math/transform.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/game_settings.hpp>
#include <wge/util/ipair.hpp>
#include <wge/core/object_resource.hpp>
#include <wge/core/scene_resource.hpp>

#include "editor.hpp"
#include "history.hpp"
#include "context.hpp"
#include "imgui_editor_tools.hpp"
#include "imgui_ext.hpp"
#include "text_editor.hpp"
#include "icon_codepoints.hpp"
#include "asset_manager_window.hpp"

#include <imgui/imgui_internal.h>

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <wge/graphics/glfw_backend.hpp>

#include <variant>
#include <functional>
#include <future>
#include <unordered_map>
#include <array>

namespace wge::graphics
{

class camera
{
public:
	void set_zoom(float pZoom) { /* TODO */ }
	float get_zoom() const { return mZoom; }
	void zoom(float pAmount) { /* TODO */ }

	void set_focus(const math::vec2& pPosition) { /* TODO */ }
	math::vec2 get_focus() const { return mFocus; }
	void move_focus(const math::vec2& pAmount) { /* TODO */ }

	void set_boundary_enable(bool mEnabled) { /* TODO */ }
	bool is_boundary_enabled() const { return mBoundary_enabled; }
	void set_boundary(const math::aabb& pLimit) { /* TODO */ }
	math::aabb get_boundary() const { return mBoundary; }
	
	void envelop(const math::aabb& pTarget, const math::ivec2& pScreen_size)
	{
		mFocus = math::midpoint(pTarget.min, pTarget.max);
		// TODO
	}

	void envelop(const math::aabb& pTarget)
	{
		mNeeds_envelop = true;
		mEnvelop_target = pTarget;
	}

	math::aabb get_view(const math::ivec2& pScreen_size)
	{
		// TODO
		math::aabb view;
		return view;
	}

private:
	bool mBoundary_enabled = false;
	math::aabb mBoundary;

	bool mNeeds_envelop = false;
	math::aabb mEnvelop_target;
	
	math::vec2 mFocus;
	float mZoom = 0;
};

}

namespace wge::core
{

class texture_importer :
	public importer
{
public:
	virtual asset::ptr import(asset_manager& pAsset_mgr, const filesystem::path& pSystem_path) override
	{
		auto tex = std::make_shared<asset>();
		tex->set_name(pSystem_path.stem());
		tex->set_type("texture");
		
		// Create a new subdirectory for the asset's storage.
		auto directory = pAsset_mgr.get_root_directory() / pAsset_mgr.generate_asset_directory_name(tex);
		system_fs::create_directory(directory);
		tex->set_file_path(directory / (tex->get_name() + ".wga"));

		// Copy the texture file.
		auto dest = directory / (tex->get_name() + ".png");
		try {
			system_fs::copy_file(pSystem_path, dest);
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			log::error("Failed to copy texture from {}", pSystem_path.string());
			log::error("Failed with exception \"{}\"", e.what());
			return{};
		}

		// Create the new resource
		auto res = pAsset_mgr.create_resource("texture");
		res->load(directory, tex->get_name());
		tex->set_resource(std::move(res));

		// Save the configuration
		tex->save();

		pAsset_mgr.add_asset(tex);

		return tex;
	}
};

} // namespace wge::core

namespace wge::editor
{

// Creates an imgui dockspace in the main window
inline void main_viewport_dock(ImGuiID pDock_id)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowBgAlpha(0.0f);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |=  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoTitleBar;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("_MainDockSpace", nullptr, window_flags);
	ImGui::PopStyleVar(3);
	
	ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;//ImGuiDockNodeFlags_PassthruDockspace;
	ImGui::DockSpace(pDock_id, ImVec2(0.0f, 0.0f), dockspace_flags);

	ImGui::End();
}

inline bool collapsing_arrow(const char* pStr_id, bool* pOpen = nullptr, bool pDefault_open = false)
{
	ImGui::PushID(pStr_id);

	// Use internal instead
	if (!pOpen)
		pOpen = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("IsOpen"), pDefault_open);;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
	if (ImGui::ArrowButton("Arrow", *pOpen ? ImGuiDir_Down : ImGuiDir_Right))
		*pOpen = !*pOpen; // Toggle open flag
	ImGui::PopStyleColor(3);

	ImGui::PopID();
	return *pOpen;
}

inline void GLAPIENTRY opengl_message_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam);

class sprite_editor :
	public asset_editor
{
public:
	sprite_editor(context& pContext, const core::asset::ptr& pAsset) :
		asset_editor(pContext, pAsset)
	{}

	void on_gui()
	{
		ImGui::BeginChild("AtlasInfo", ImVec2(mAtlas_info_width, 0));
		atlas_info_pane();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::VerticalSplitter("AtlasInfoSplitter", &mAtlas_info_width);

		ImGui::SameLine();
		ImGui::BeginChild("SpriteEditorViewport", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		auto texture = get_asset()->get_resource<graphics::texture>();

		float* zoom = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("_Zoom"), 0);

		const ImVec2 last_cursor = ImGui::GetCursorPos();
		ImGui::BeginGroup();

		const float scale = std::powf(2, *zoom);
		const ImVec2 image_size = texture->get_size() * scale;

		// Top and left padding
		ImGui::Dummy(ImVec2(image_size.x + ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2));
		ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, image_size.y));
		ImGui::SameLine();

		// Store the cursor so we can position things on top of the image
		const ImVec2 image_position = ImGui::GetCursorScreenPos();

		// A checker board will help us "see" the alpha channel of the image
		ImGui::DrawAlphaCheckerBoard(image_position, image_size);

		ImGui::Image(get_asset(), image_size);

		// Right and bottom padding
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, image_size.y));
		ImGui::Dummy(ImVec2(image_size.x + ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2));
		ImGui::EndGroup();

		// Draw grid
		if (*zoom > 2)
		{
			ImGui::DrawGridLines(image_position,
				ImVec2(image_position.x + image_size.x, image_position.y + image_size.y),
				{ 0, 1, 1, 0.2f }, scale);
		}

		// Overlap with an invisible button to recieve input
		ImGui::SetCursorPos(last_cursor);
		ImGui::InvisibleButton("_Input", ImVec2(image_size.x + ImGui::GetWindowWidth(), image_size.y + ImGui::GetWindowHeight()));

		visual_editor::begin("_SomeEditor", { image_position.x, image_position.y }, { 0, 0 }, { scale, scale });

		// Draw the rectangles for the frames
		for (const auto& i : texture->get_raw_atlas())
			visual_editor::draw_rect(i.frame_rect, { 0, 1, 1, 0.5f });

		const bool was_dragging = visual_editor::is_dragging();

		// Get the pointer to the selected animation
		graphics::animation* selected_animation = texture->get_animation(mSelected_animation_id);

		// Modify selected
		if (selected_animation)
		{
			visual_editor::begin_snap({ 1, 1 });

			// Edit the selection
			visual_editor::box_edit box_edit(selected_animation->frame_rect);
			box_edit.resize(visual_editor::edit_type::rect);
			box_edit.drag(visual_editor::edit_type::rect);
			selected_animation->frame_rect = box_edit.get_rect();

			// Limit the minimum size to +1 pixel so the user isn't using 0 or negitive numbers
			selected_animation->frame_rect.size = math::max(selected_animation->frame_rect.size, math::vec2(1, 1));

			// Notify a change in the asset
			if (box_edit.is_dragging() && ImGui::IsMouseReleased(0))
				mark_asset_modified();

			visual_editor::end_snap();
		}

		// Select a new one
		if (!was_dragging && ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
		{
			// Find all overlapping frames that the mouse is hovering
			std::vector<graphics::animation*> mOverlapping;
			for (auto& i : texture->get_raw_atlas())
				if (i.frame_rect.intersects(visual_editor::get_mouse_position()))
					mOverlapping.push_back(&i);

			if (!mOverlapping.empty())
			{
				// Check if the currently selected animation is being selected again
				// and cycle through the overlapping animations each click.
				auto iter = std::find(mOverlapping.begin(), mOverlapping.end(), selected_animation);
				if (iter == mOverlapping.end() || iter + 1 == mOverlapping.end())
					selected_animation = mOverlapping.front(); // Start/loop to front
				else
					selected_animation = *(iter + 1); // Next item
				mSelected_animation_id = selected_animation->id;
			}
		}

		visual_editor::end();

		if (ImGui::IsItemHovered())
		{
			// Zoom with ctrl and mousewheel
			if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0)
			{
				*zoom += ImGui::GetIO().MouseWheel;
				const float new_scale = std::powf(2, *zoom);
				const float ratio_changed = new_scale / scale;
				ImGui::SetScrollX(ImGui::GetScrollX() * ratio_changed);
				ImGui::SetScrollY(ImGui::GetScrollY() * ratio_changed);
			}

			// Hold middle mouse button to scroll
			ImGui::DragScroll(2);
		}

		ImGui::EndChild();
	}

	static void preview_image(const char* pStr_id, const graphics::texture::handle& pTexture, const math::vec2& pSize, const math::rect& pFrame_rect)
	{
		if (pSize.x <= 0 || pSize.y <= 0)
			return;

		// Scale the size of the image to preserve the aspect ratio but still fit in the
		// specified area.
		const float aspect_ratio = pFrame_rect.size.x / pFrame_rect.size.y;
		math::vec2 scaled_size =
		{
			math::min(pSize.y * aspect_ratio, pSize.x),
			math::min(pSize.x / aspect_ratio, pSize.y)
		};

		// Center the position
		const math::vec2 center_offset = pSize / 2 - scaled_size / 2;
		const math::vec2 pos = math::vec2(ImGui::GetCursorScreenPos()) + center_offset;

		// Draw the checkered background
		ImGui::DrawAlphaCheckerBoard(pos, scaled_size, 10);

		// Convert to UV coord
		math::aabb uv(pFrame_rect);
		uv.min /= pTexture->get_size();
		uv.max /= pTexture->get_size();

		// Draw the image
		const auto impl = std::dynamic_pointer_cast<graphics::opengl_texture_impl>(pTexture->get_implementation());
		auto dl = ImGui::GetWindowDrawList();
		dl->AddImage((void*)impl->get_gl_texture(), pos, pos + scaled_size, uv.min, uv.max);

		// Add an invisible button so we can interact with this image
		ImGui::InvisibleButton(pStr_id, pSize);
	}

	void atlas_info_pane()
	{
		const auto open_sprite_editor = []()
		{
			ImGui::Begin("Sprite Editor");
			ImGui::SetWindowFocus();
			ImGui::End();
		};

		graphics::texture::handle texture = get_asset();

		if (ImGui::CollapsingHeader("Atlas", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static float atlas_list_height = 200;
			// Atlas list
			ImGui::BeginChild("_AtlasList", { 0, atlas_list_height }, true);
			ImGui::Columns(2, "_Previews", false);
			ImGui::SetColumnWidth(0, 75 + ImGui::GetStyle().WindowPadding.x + ImGui::GetStyle().ItemSpacing.x);
			for (auto& i : texture->get_raw_atlas())
			{
				if (ImGui::Selectable(("###" + i.name).c_str(), mSelected_animation_id == i.id, ImGuiSelectableFlags_SpanAllColumns, { 0, 75 }))
					mSelected_animation_id = i.id;

				// Double click will focus the sprite editor
				if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
					open_sprite_editor();
				ImGui::SameLine();

				preview_image("SmallPreviewImage", texture, { 75, 75 }, i.frame_rect);

				ImGui::NextColumn();

				// Entry name
				ImGui::Text(i.name.c_str());
				ImGui::NextColumn();
			}
			ImGui::Columns();
			ImGui::EndChild();

			ImGui::HorizontalSplitter("AtlasListSplitter", &atlas_list_height);

			if (ImGui::Button("Add"))
			{
				graphics::animation& animation = texture->get_raw_atlas().emplace_back();
				animation.frame_rect = math::rect({ 0, 0 }, texture->get_size());
				animation.name = make_unique_animation_name(texture, "NewEntry");
				animation.id = util::generate_uuid();
				mark_asset_modified();
			}

			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				auto iter = std::find_if(
					texture->get_raw_atlas().begin(),
					texture->get_raw_atlas().end(),
					[&](auto& i) { return i.id == mSelected_animation_id; });
				if (iter != texture->get_raw_atlas().end())
				{
					// Set the next animation after this one as selected
					if (iter + 1 != texture->get_raw_atlas().end())
						mSelected_animation_id = (iter + 1)->id;

					// Remove it
					texture->get_raw_atlas().erase(iter);
				}
				mark_asset_modified();
			}
		}

		if (graphics::animation* selected_animation = texture->get_animation(mSelected_animation_id))
		{
			ImGui::PushID("_AnimationSettings");

			if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
			{
				static float preview_image_height = 200;
				ImGui::BeginChild("PreviewImageChild", { 0, preview_image_height }, true, ImGuiWindowFlags_NoInputs);
				preview_image("LargePreviewImage", texture, ImGui::GetWindowContentRegionSize(), selected_animation->frame_rect);
				ImGui::EndChild();
				ImGui::HorizontalSplitter("PreviewImageSplitter", &preview_image_height);
				preview_image_height = math::max(preview_image_height, 30.f);

				ImGui::Button("Play");
				static int a = 1;
				const std::string format = "%d/" + std::to_string(selected_animation->frames);
				ImGui::SliderInt("Frame", &a, 1, selected_animation->frames, format.c_str());
			}
			if (ImGui::CollapsingHeader("Basic", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::InputText("Name", &selected_animation->name);
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					std::string temp = std::move(selected_animation->name);
					selected_animation->name = make_unique_animation_name(texture, temp);
					mark_asset_modified();
				}
				ImGui::DragFloat2("Position", selected_animation->frame_rect.position.components().data()); check_if_edited();
				ImGui::DragFloat2("Size", selected_animation->frame_rect.size.components().data()); check_if_edited();
			}
			if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int frame_count = static_cast<int>(selected_animation->frames);
				if (ImGui::InputInt("Frame Count", &frame_count))
				{
					// Limit the minimun to 1
					selected_animation->frames = math::max<std::size_t>(static_cast<std::size_t>(frame_count), 1);
					mark_asset_modified();
				}
				ImGui::InputFloat("Interval", &selected_animation->interval, 0.01f, 0.1f, "%.3f Seconds"); check_if_edited();
			}
			ImGui::PopID();
		}
	}

private:
	static std::string make_unique_animation_name(const graphics::texture::handle& pTexture, const std::string& pName)
	{
		return util::create_unique_name(pName,
			pTexture->get_raw_atlas().begin(), pTexture->get_raw_atlas().end(),
			[](auto& i) -> const std::string& { return i.name; });
	}

	void check_if_edited()
	{
		if (ImGui::IsItemDeactivatedAfterEdit())
			mark_asset_modified();
	}

private:
	util::uuid mSelected_animation_id;
	float mAtlas_info_width = 200;
};

class script_editor :
	public asset_editor
{
public:
	script_editor(context& pContext, const core::asset::ptr& pAsset) :
		asset_editor(pContext, pAsset)
	{}

	virtual void on_gui() override
	{
		auto source = get_asset()->get_resource<scripting::script>();

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
		if (ImGui::CodeEditor("CodeEditor", source->source))
			mark_asset_modified();
		ImGui::PopFont();
	}
};

class scene_editor :
	public asset_editor
{
public:
	// TODO: Implement a better and more generic messaging method at some point.
	using on_game_run_callback = std::function<void(const core::asset::ptr&)>;

	scene_editor(context& pContext, const core::asset::ptr& pAsset, const on_game_run_callback& pRun_callback) noexcept :
		asset_editor(pContext, pAsset),
		mScene(pContext.get_engine().get_factory()),
		mOn_game_run_callback(pRun_callback)
	{
		// Create a framebuffer for the scene to be rendered to.
		auto& graphics = pContext.get_engine().get_graphics();
		mViewport_framebuffer = graphics.get_graphics_backend()->create_framebuffer();

		mScene_resource = get_asset()->get_resource<core::scene_resource>();

		// Generate the layer.
		mScene_resource->generate_scene(mScene, pContext.get_engine().get_asset_manager());
	}

	virtual void on_gui() override
	{
		ImGui::BeginChild("SidePanelSettings", ImVec2(200, 0));
		if (ImGui::Button("Run") && mOn_game_run_callback)
		{
			// Make sure the actual asset data is up to date before
			// we start the scene.
			update_asset_data();
			// Invoke the callback. I may consider using a better
			// messaging mechanism later.
			mOn_game_run_callback(get_asset());
		}
		ImGui::TextUnformatted("Layers");
		ImGui::BeginChild("Layers", ImVec2(0, 300), true);
		for (const auto& i : mScene.get_layer_container())
		{
			ImGui::PushID(i.get());

			if (ImGui::Selectable(i->get_name().c_str(), mSelected_layer == i.get()))
				mSelected_layer = i.get();
			ImGui::PopID();
		}
		ImGui::EndChild();

		ImGui::TextUnformatted("Instances");
		ImGui::BeginChild("Instances", ImVec2(0, 0), true);
		if (mSelected_layer)
		{
			for (auto obj : *mSelected_layer)
			{
				ImGui::PushID(obj.get_id());
				if (ImGui::Selectable(obj.get_name().c_str(), obj.get_id() == mSelected_object_id))
					mSelected_object_id = obj.get_id();
				ImGui::PopID();
			}
		}
		ImGui::EndChild(); // Instances

		ImGui::EndChild(); // SidePanelSettings

		ImGui::SameLine();

		static float viewport_width = -100;
		ImGui::BeginChild("Scene", ImVec2(viewport_width, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		show_viewport();

		ImGui::EndChild(); // Scene
		ImGui::SameLine();
		ImGui::VerticalSplitter("ViewportInspectorSplitter", &viewport_width);
		ImGui::SameLine();
		ImGui::BeginChild("InspectorSidePanel", ImVec2(-viewport_width, 0));
		if (mSelected_layer && mSelected_object_id != core::invalid_id)
		{
			ImGui::TextUnformatted("Selected Object Settings");
			auto obj = mSelected_layer->get_object(mSelected_object_id);
			std::string name = obj.get_name();
			if (ImGui::InputText("Name", &name))
				obj.set_name(name);
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				obj.set_name(scripting::make_valid_identifier(name));
				mark_asset_modified();
			}
		}
		ImGui::EndChild();
	}

	void update_asset_data()
	{
		auto resource = get_asset()->get_resource<core::scene_resource>();
		resource->instances.clear();

		auto layer = mScene.get_layer(0);
		for (auto obj : *layer)
		{
			core::scene_resource::instance& inst = resource->instances.emplace_back();
			inst.asset_id = obj.get_asset()->get_id();
			inst.id = obj.get_id();
			inst.name = obj.get_name();
			if (auto transform = obj.get_component<math::transform>())
				inst.transform = *transform;
		}
	}

	virtual void on_save() override
	{
		update_asset_data();
	}

	void show_viewport()
	{
		core::engine& engine = get_context().get_engine();

		static bool show_center_point = true;
		static bool is_grid_enabled = true;
		static graphics::color grid_color{ 1, 1, 1, 0.7f };
		static bool show_collision = false;

		/*if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Object", NULL, false, false);
				ImGui::Checkbox("Center Point", &show_center_point);
				ImGui::Separator();
				ImGui::MenuItem("Grid", NULL, false, false);
				ImGui::Checkbox("Enable Grid", &is_grid_enabled);
				ImGui::ColorEdit4("Grid Color", grid_color.components, ImGuiColorEditFlags_NoInputs);

				ImGui::Separator();
				ImGui::MenuItem("Physics", NULL, false, false);
				ImGui::Checkbox("Collision", &show_collision);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}*/
		ImVec2 region_size = ImGui::GetContentRegionAvail();
		float width = region_size.x;//ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2 - ImGui::GetStyle().ScrollbarSize;
		float height = region_size.y;//ImGui::GetWindowHeight() - ImGui::GetCursorPos().y - ImGui::GetStyle().WindowPadding.y * 2 - ImGui::GetStyle().ScrollbarSize;
		float scroll_x_max = width * 2;
		float scroll_y_max = height * 2;

		ImGui::FillWithFramebuffer(mViewport_framebuffer);

		ImGui::BeginFixedScrollRegion({ width, height }, { scroll_x_max, scroll_y_max });

		ImVec2 cursor = ImGui::GetCursorScreenPos();

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		ImGui::ImageButton(mViewport_framebuffer, ImVec2(width, height));

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		// Middle mouse to drag
		if (ImGui::IsItemHovered())
			ImGui::DragScroll(2, 1);

		mViewport_offset = (math::vec2(ImGui::GetScrollX(), ImGui::GetScrollY()) / mViewport_scale);

		visual_editor::begin("_SceneEditor", { cursor.x, cursor.y }, mViewport_offset, mViewport_scale);
		{
			if (ImGui::BeginDragDropTarget() && mSelected_layer)
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("gameobjectAsset"))
				{
					// Retrieve the object asset from the payload.
					const util::uuid& id = *(const util::uuid*)payload->Data;
					auto obj = new_instance(id);

					// Set the transform to the position it was dropped at.
					if (auto transform = obj.get_component<math::transform>())
						transform->position = visual_editor::get_mouse_position();
				}
				ImGui::EndDragDropTarget();
			}

			if (is_grid_enabled)
				visual_editor::draw_grid(grid_color, 1);

			for (auto& layer : mScene.get_layer_container())
			{
				graphics::renderer* renderer = layer->get_system<graphics::renderer>();
				if (!renderer)
					continue;
				// Make sure we are working with the viewports framebuffer
				renderer->set_framebuffer(mViewport_framebuffer);

				if (show_collision)
				{
					for (auto& [id, comp, transform] :
						layer->each<physics::box_collider_component, math::transform>())
					{
						visual_editor::push_transform(transform);
						math::transform box_transform;
						box_transform.position = comp.get_offset();
						box_transform.rotation = comp.get_rotation();
						visual_editor::push_transform(box_transform);
						visual_editor::draw_rect(math::rect({ 0, 0 }, comp.get_size()), { 0, 1, 0, 0.8f });
						visual_editor::pop_transform(2);
					}
				}

				for (auto [id, transform] : layer->each<math::transform>())
				{
					const bool is_object_selected = id == mSelected_object_id;

					math::aabb aabb;
					if (create_aabb_from_object(layer->get_object(id), aabb))
					{
						if (is_object_selected)
						{
							visual_editor::box_edit box_edit(aabb, transform);
							box_edit.resize(visual_editor::edit_type::transform);
							box_edit.drag(visual_editor::edit_type::transform);
							transform = box_edit.get_transform();
							if (box_edit.is_dragging())
							{
								mark_asset_modified();
							}
						}

						visual_editor::push_transform(transform);
						if (aabb.intersect(visual_editor::get_mouse_position()))
						{
							if (ImGui::IsItemClicked())
								mSelected_object_id = id;
							// TODO: Show info about this object instance
							visual_editor::draw_rect(aabb, { 1, 1, 1, 1 });
						}
						visual_editor::pop_transform();
					}

					// Draw center point
					if (is_object_selected && show_center_point)
					{
						visual_editor::draw_circle(transform.position, 5, { 1, 1, 1, 0.6f }, 3.f);
					}
				}
			}
		}
		visual_editor::end();

		// Clear the framebuffer with black.
		mViewport_framebuffer->clear({ 0, 0, 0, 1 });

		// Render all layers.
		mScene.for_each_system<graphics::renderer>([&](core::layer&, graphics::renderer& pRenderer)
		{
			pRenderer.set_framebuffer(mViewport_framebuffer);
			pRenderer.set_render_view_to_framebuffer(mViewport_offset, 1.f / mViewport_scale);
			pRenderer.render(engine.get_graphics());
		});
		ImGui::EndFixedScrollRegion();
	}

	// Generate a new instance from an object asset.
	core::object new_instance(util::uuid pAsset)
	{
		core::engine& engine = get_context().get_engine();
		auto asset = engine.get_asset_manager().get_asset(pAsset);
		auto object_resource = asset->get_resource<core::object_resource>();

		// Generate the object
		core::object obj = mSelected_layer->add_object();
		object_resource->generate_object(obj, get_asset_manager());
		obj.set_asset(asset);

		mark_asset_modified();

		return obj;
	}

	static bool create_aabb_from_object(core::object& pObj, math::aabb& pAABB)
	{
		/*bool has_aabb = false;
		for (std::size_t comp_idx = 0; comp_idx < pObj.get_component_count(); comp_idx++)
		{
			auto comp = pObj.get_component_at(comp_idx);
			if (comp->has_aabb())
			{
				if (!has_aabb)
				{
					pAABB = comp->get_local_aabb();
					has_aabb = true;
				}
				else
					pAABB.merge(comp->get_local_aabb());
			}
		}*/
		return false;//has_aabb;
	}

private:
	core::layer* mSelected_layer = nullptr;
	core::object_id mSelected_object_id = core::invalid_id;
	core::scene mScene;

	core::scene_resource* mScene_resource = nullptr;

	graphics::framebuffer::ptr mViewport_framebuffer;
	math::vec2 mViewport_offset;
	math::vec2 mViewport_scale{ 100, 100 };

	on_game_run_callback mOn_game_run_callback;
};

class drop_import_handler
{
public:
	drop_import_handler(core::asset_manager& pAsset_manager) :
		mAsset_manager(&pAsset_manager)
	{}

	~drop_import_handler()
	{
		mCallback_connection.disconnect();
	}

	void register_callback_to(decltype(graphics::window_backend::on_file_drop)& pCallback)
	{
		mCallback_connection = pCallback.connect([&](int pCount, const char** pPaths)
		{
			for (int i = 0; i < pCount; i++)
			{
				filesystem::path path = pPaths[i];
				if (path.extension() == ".png")
					mPaths.push_back(std::move(path));
			}
		});
	}

	void on_gui()
	{
		if (!mPaths.empty())
			ImGui::OpenPopup("Import Assets");

		if (ImGui::BeginPopupModal("Import Assets", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::BeginChild("AssetsToImport", ImVec2(400, 300), true);

			for (auto& i : mPaths)
				ImGui::Selectable(i.string().c_str());

			ImGui::EndChild();
			if (ImGui::Button("Import"))
			{
				core::texture_importer importer;
				for (auto& i : mPaths)
				{
					importer.import(*mAsset_manager, i);
				}
				mPaths.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				mPaths.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

private:
	core::asset_manager* mAsset_manager;
	std::vector<filesystem::path> mPaths;
	util::connection mCallback_connection;
};

static bool texture_asset_input(core::asset::ptr& pAsset, context& pContext, const core::asset_manager& pAsset_manager)
{
	std::string inputtext = pAsset ? pAsset_manager.get_asset_path(pAsset).string().c_str() : "None";
	ImGui::BeginGroup();
	if (pAsset)
	{
		const bool is_selected = ImGui::ImageButton(pAsset, { 100, 100 });
		ImGui::QuickToolTip("Open Sprite Editor");
		if (is_selected)
			pContext.open_editor(pAsset);
		ImGui::SameLine();
		ImGui::BeginGroup();
		auto res = pAsset->get_resource<graphics::texture>();
		ImGui::Text("Size: %i, %i", res->get_width(), res->get_height());
		ImGui::Text("Animations: %u", res->get_raw_atlas().size());
		ImGui::EndGroup();
		ImGui::InputText("Texture", &inputtext, ImGuiInputTextFlags_ReadOnly);
	}
	else
	{
		ImGui::Button("Drop a texture asset here", ImVec2(-1, 100));
	}
	ImGui::EndGroup();

	bool asset_dropped = false;

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("textureAsset"))
		{
			const util::uuid& id = *(const util::uuid*)payload->Data;
			pAsset = pAsset_manager.get_asset(id);
			asset_dropped = true;
		}
		ImGui::EndDragDropTarget();
	}
	return asset_dropped;
}

class eventful_sprite_editor :
	public asset_editor
{
private:
	static constexpr std::array event_display_name = {
		ICON_FA_PENCIL u8" Create",
		ICON_FA_STEP_FORWARD u8" Update"
	};

public:
	eventful_sprite_editor(context& pContext, const core::asset::ptr& pAsset) noexcept :
		asset_editor(pContext, pAsset)
	{}

	virtual void on_gui() override
	{
		mSCript_editor_dock_id = ImGui::GetID("EditorDock");

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

		auto generator = get_asset()->get_resource<core::object_resource>();

		display_sprite_input(generator);
		display_event_list(generator);

		ImGui::EndChild();

		ImGui::SameLine();

		ImGuiDockFamily dock_family(mSCript_editor_dock_id);
		// Event script editors are given a dedicated dockspace where they spawn. This helps
		// remove clutter windows popping up everywhere.
		ImGui::DockSpace(mSCript_editor_dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_None, &dock_family);
	}

	virtual void on_close() override
	{
		auto generator = get_asset()->get_resource<core::object_resource>();
		for (auto& i : generator->events)
			if (i.is_valid())
				get_context().close_editor(i);
	}

private:
	void display_sprite_input(core::object_resource* pGenerator)
	{
		core::asset::ptr sprite = get_asset_manager().get_asset(pGenerator->display_sprite);
		if (texture_asset_input(sprite, get_context(), get_asset_manager()))
		{
			pGenerator->display_sprite = sprite->get_id();
			mark_asset_modified();
		}
	}

	void display_event_list(core::object_resource* pGenerator)
	{
		ImGui::Text("Events:");
		ImGui::BeginChild("Events", ImVec2(0, 0), true);
		for (auto[type, asset_id] : util::ipair{ pGenerator->events })
		{
			const char* event_name = event_display_name[type];
			const bool editor_already_open = get_context().is_editor_open_for(asset_id);
			if (ImGui::Selectable(
				event_name,
				editor_already_open, ImGuiSelectableFlags_AllowDoubleClick)
				&& !editor_already_open && ImGui::IsMouseDoubleClicked(0))
			{
				bool first_time = false;
				core::asset::ptr asset;
				if (asset_id.is_valid())
				{
					asset = get_asset_manager().get_asset(asset_id);
				}
				else
				{
					asset = create_event_script(pGenerator->event_typenames[type]);
					asset_id = asset->get_id();
					mark_asset_modified();
					first_time = true;
				}
				auto editor = get_context().open_editor(asset, mSCript_editor_dock_id);
				editor->set_dock_family_id(mSCript_editor_dock_id);
			}
		}
		ImGui::EndChild();
	}

private:
	core::asset::ptr create_event_script(const char* pName)
	{
		core::asset_manager& asset_manager = get_context().get_engine().get_asset_manager();

		auto asset = std::make_shared<core::asset>();
		asset->set_name(pName);
		asset->set_parent(get_asset());
		asset->set_type("script");
		asset->set_resource(asset_manager.create_resource("script"));
		asset_manager.store_asset(asset);

		auto script = asset->get_resource<scripting::script>();
		script->save(asset->get_file_path().parent(), pName);
		asset->save();

		asset_manager.add_asset(asset);

		return asset;
	}

	ImGuiID mSCript_editor_dock_id;
};

class game_viewport
{
public:
	game_viewport(core::engine& pEngine) :
		mEngine(&pEngine)
	{}

	void open_scene(const core::asset::ptr& pAsset)
	{
		if (!pAsset)
		{
			log::error("No asset to generate scene with.");
			return;
		}
		mEngine->get_scene().clear();
		if (auto resource = pAsset->get_resource<core::scene_resource>())
		{
			resource->generate_scene(mEngine->get_scene(), mEngine->get_asset_manager());
			mIs_loaded = true;
			mAsset = pAsset;
		}
		else
		{
			log::error("Asset is not a scene.");
		}
	}

	void restart()
	{
		open_scene(mAsset);
	}

	void init_viewport()
	{
		auto& g = mEngine->get_graphics();
		mViewport_framebuffer = g.get_graphics_backend()->create_framebuffer();
		mViewport_framebuffer->resize(500, 500);
	}

	void register_input()
	{
		auto& state = mEngine->get_script_engine().state;
		auto input = state.create_named_table("input");

		state.new_enum<int>("key", {
			{"left", GLFW_KEY_LEFT},
			{"right", GLFW_KEY_RIGHT},
			{"up", GLFW_KEY_UP},
			{"down", GLFW_KEY_DOWN},
			{"a", GLFW_KEY_A},
			{"b", GLFW_KEY_B},
			{"c", GLFW_KEY_C},
			{"d", GLFW_KEY_D},
			{"e", GLFW_KEY_E},
			{"f", GLFW_KEY_F},
			{"g", GLFW_KEY_G},
			{"h", GLFW_KEY_H},
			{"i", GLFW_KEY_I},
			{"j", GLFW_KEY_J},
			{"k", GLFW_KEY_K},
			{"l", GLFW_KEY_L},
			{"m", GLFW_KEY_M},
			{"n", GLFW_KEY_N},
			{"o", GLFW_KEY_O},
			{"p", GLFW_KEY_P},
			{"q", GLFW_KEY_Q},
			{"r", GLFW_KEY_R},
			{"s", GLFW_KEY_S},
			{"t", GLFW_KEY_T},
			{"u", GLFW_KEY_U},
			{"v", GLFW_KEY_V},
			{"w", GLFW_KEY_W},
			{"x", GLFW_KEY_X},
			{"y", GLFW_KEY_Y},
			{"z", GLFW_KEY_Z},
			{"space", GLFW_KEY_SPACE},
			{"tab", GLFW_KEY_TAB},
			{"lshift", GLFW_KEY_LEFT_SHIFT},
			{"rshift", GLFW_KEY_RIGHT_SHIFT},
			{"lctrl", GLFW_KEY_LEFT_CONTROL},
			{"rctrl", GLFW_KEY_RIGHT_CONTROL},
			{"lalt", GLFW_KEY_LEFT_ALT},
			{"ralt", GLFW_KEY_RIGHT_ALT}
			});

		input["pressed"] = [this](int pKey) -> bool
		{
			return mCan_take_input && ImGui::IsKeyPressed(pKey, false);
		};
		input["down"] = [this](int pKey) -> bool
		{
			return mCan_take_input && ImGui::IsKeyDown(pKey);
		};
	}

	void step()
	{
		if (mIs_running)
		{
			mEngine->step();
		}

		// Clear the framebuffer with black.
		mViewport_framebuffer->clear({ 0, 0, 0, 1 });

		mEngine->render_to(mViewport_framebuffer, math::vec2(0, 0), math::vec2(1.f, 1.f) * 100.f);
	}

	void on_gui()
	{
		if (ImGui::Begin("Game##GameViewport"))
		{
			// Play/pause button.
			if (ImGui::Button(mIs_running ? ICON_FA_PAUSE " Pause" : ICON_FA_PLAY "Play"))
			{
				mIs_running = !mIs_running;
			}

			// Restart button.
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_UNDO " Restart"))
			{
				restart();
			}

			if (mIs_loaded)
			{
				ImGui::Image(mViewport_framebuffer, ImGui::FillWithFramebuffer(mViewport_framebuffer));
				mCan_take_input = ImGui::IsWindowFocused();
				step();
			}
			else
			{
				ImGui::TextUnformatted("No game to display... yet");
			}
		}
		ImGui::End();
	}

private:
	bool mIs_running = false;
	bool mIs_loaded = false;
	bool mCan_take_input = false;
	graphics::framebuffer::ptr mViewport_framebuffer;

	core::asset::ptr mAsset;
	core::engine* mEngine;
};

class log_window
{
public:
	void on_gui()
	{
		const std::size_t log_limit = 256;

		if (ImGui::Begin("Log", NULL, ImGuiWindowFlags_HorizontalScrollbar))
		{
			auto log = log::get_log();

			// If the window is scrolled to the bottom, keep it at the bottom.
			// To prevent it from locking the users mousewheel input, it will only lock the scroll
			// when the log actually changes.
			bool lock_scroll_at_bottom = ImGui::GetScrollY() == ImGui::GetScrollMaxY() && last_log_size != log.size();

			ImGui::Columns(2, 0, false);
			ImGui::SetColumnWidth(0, 70);

			// Limit the amount of items that can be shown in log.
			// This makes it more convenient to scroll and there is less to draw.
			for (auto& i : log.last(log_limit))
			{
				switch (i.severity_level)
				{
				case log::level::info:
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
					ImGui::TextUnformatted("Info");
					break;
				case log::level::debug:
					ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 1, 1, 1 }); // Cyan-ish
					ImGui::TextUnformatted("Debug");
					break;
				case log::level::warning:
					ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 0.5f, 1 }); // Yellow-ish
					ImGui::TextUnformatted("Warning");
					break;
				case log::level::error:
					ImGui::PushStyleColor(ImGuiCol_Text, { 1, 0.5f, 0.5f, 1 }); // Red
					ImGui::TextUnformatted("Error");
					break;
				}
				ImGui::NextColumn();

				// The actual message. Has the same color as the message type.
				ImGui::TextUnformatted(i.string.c_str());
				ImGui::PopStyleColor();

				ImGui::NextColumn();
			}

			ImGui::Columns(1);

			if (lock_scroll_at_bottom)
				ImGui::SetScrollHere();
			last_log_size = log.size();
		}
		ImGui::End();
	}

private:
	size_t last_log_size = 0;
};

class application
{
public:
	application()
	{
		mContext.register_editor<sprite_editor>("texture");
		//mContext.register_editor<object_editor>("gameobject", mInspectors);
		mContext.register_editor<script_editor>("script");
		mContext.register_editor<scene_editor>("scene", mOn_game_run);
		mContext.register_editor<eventful_sprite_editor>("gameobject");
		mEngine.get_asset_manager().register_default_resource_factory<core::object_resource>("gameobject");
		mEngine.get_asset_manager().register_default_resource_factory<core::scene_resource>("scene");

		mOn_game_run.connect([this](const core::asset::ptr& pAsset) {
			mGame_viewport.open_scene(pAsset);
		});
	}

	int run()
	{
		init_graphics();
		init_imgui();
		mGame_viewport.init_viewport();
		mGame_viewport.register_input();

		load_project("project");

		mainloop();
		return 0;
	}

private:
	void init_graphics()
	{
		auto& g = mEngine.get_graphics();
		// Only glfw and opengl is supported for editing
		g.initialize(graphics::window_backend_type::glfw, graphics::backend_type::opengl);

		// Store the glfw backend for initializing imgui's glfw backend
		mGLFW_backend = std::dynamic_pointer_cast<graphics::glfw_window_backend>(g.get_window_backend());

		mDrop_import_handler.register_callback_to(mGLFW_backend->on_file_drop);
	}

	void mainloop()
	{
		while (!glfwWindowShouldClose(mGLFW_backend->get_window()))
		{
			new_frame();

			mContext.set_default_dock_id(ImGui::GetID("_MainDockId"));
			main_viewport_dock(mContext.get_default_dock_id());

			if (ImGui::BeginMainMenuBar())
			{
				// Just an aesthetic
				ImGui::TextColored({ 0.5, 0.5, 0.5, 1 }, "WGE");

				if (ImGui::BeginMenu(ICON_FA_HOME " Project"))
				{
					ImGui::MenuItem(" New");
					ImGui::MenuItem(" Open");
					ImGui::Separator();
					//ImGui::MenuItem("Save", "Ctrl+S", false, mContext.are_there_modified_assets());
					if (ImGui::MenuItem("Save All", "Ctrl+Alt+S", false, mContext.are_there_modified_assets() && mEngine.is_loaded()))
						mContext.save_all_assets();
					ImGui::Separator();
					if (ImGui::BeginMenu("Recent"))
					{
						ImGui::EndMenu();
					}
					ImGui::Separator();
					ImGui::MenuItem("Exit", "Alt+F4");
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Windows"))
				{
					if (ImGui::MenuItem("Close all editors"))
					{
						mContext.close_all_editors();
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			mLog_window.on_gui();
			show_settings();
			//show_viewport();
			//show_objects();
			//show_inspector();
			mDrop_import_handler.on_gui();
			mAsset_manager_window.on_gui();
			mContext.show_editor_guis();
			mGame_viewport.on_gui();

			end_frame();
		}
		shutdown();
	}

	void shutdown()
	{
		// Cleanup ImGui
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void new_frame()
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void end_frame()
	{
		ImGui::Render();

		int display_w, display_h;
		glfwMakeContextCurrent(mGLFW_backend->get_window());
		glfwGetFramebufferSize(mGLFW_backend->get_window(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.15f, 0.15f, 0.15f, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		mGLFW_backend->refresh();
	}

	void init_imgui()
	{
		// Setup imgui, enable docking and dpi support.
		ImGui::CreateContext();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Viewports are disabled for now. Might make it an optional setting later.
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		ImGui::GetIO().ConfigDockingWithShift = true;
		ImGui_ImplGlfw_InitForOpenGL(mGLFW_backend->get_window(), true);
		ImGui_ImplOpenGL3_Init("#version 150");

		auto fonts = ImGui::GetIO().Fonts;

		// This will be our default font. It is quite a bit better than imguis builtin one.
		if (fonts->AddFontFromFileTTF("./editor/Roboto-Regular.ttf", 18) == NULL)
		{
			log::error("Could not load RobotoMono-Regular font. Using default.");
			fonts->AddFontDefault();
		}

		// Setup the icon font so we can fancy things up.
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = 14;
		icons_config.GlyphMaxAdvanceX = 14;
		if (fonts->AddFontFromFileTTF("./editor/forkawesome-webfont.ttf", 18, &icons_config, icons_ranges) == NULL)
		{
			log::error("Could not load forkawesome-webfont.ttf font.");
			fonts->AddFontDefault();
		}

		// Used in the code editor.
		if (fonts->AddFontFromFileTTF("./editor/RobotoMono-Regular.ttf", 18) == NULL)
		{
			log::error("Could not load RobotoMono-Regular font. Using default.");
			fonts->AddFontDefault();
		}


		// Theme
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.90f);
		colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.44f, 0.44f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.77f, 0.77f, 0.77f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.64f, 0.64f, 0.64f, 0.40f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.54f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.77f, 0.77f, 0.77f, 0.40f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.64f, 0.64f, 0.64f, 0.40f);
		colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.54f, 0.54f, 0.54f, 0.78f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.431f, 0.431f, 0.431f, 0.500f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.54f, 0.54f, 0.54f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.38f, 0.38f, 0.38f, 0.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.47f, 0.47f, 0.47f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

	void create_project(const filesystem::path& pPath)
	{

	}

	void load_project(const filesystem::path& pPath)
	{
		mEngine.close_game();
		mEngine.load_game(pPath);

		mEngine.get_script_engine().execute_global_scripts(mEngine.get_asset_manager());
	}

private:
	void show_settings()
	{
		if (ImGui::Begin(ICON_FA_COG " Settings"))
		{
			ImGui::BeginTabBar("SettingsTabBar");
			if (ImGui::BeginTabItem("Style"))
			{
				ImGui::ShowStyleEditor();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
	}

private:
	// ImGui needs access to some glfw specific objects
	graphics::glfw_window_backend::ptr mGLFW_backend;

	util::signal<void(const core::asset::ptr&)> mOn_game_run;

	context mContext;

	log_window mLog_window;

	// Reference the game engine for convenience.
	core::engine& mEngine{ mContext.get_engine() };

	asset_manager_window mAsset_manager_window{ mContext, mEngine.get_asset_manager() };
	drop_import_handler mDrop_import_handler{ mEngine.get_asset_manager() };
	game_viewport mGame_viewport{ mEngine };

	bool mUpdate{ false };
};

int main(int argc, char ** argv)
{
	log::open_file("./editor/log.txt");
	application app;
	return app.run();
}

} // namespace wge::editor
