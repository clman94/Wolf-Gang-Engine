
#include <wge/editor/application.hpp>

#include <wge/logging/log.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/core/context.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/graphics/sprite_component.hpp>
#include <wge/scripting/script.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/graphics/texture_asset_loader.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/exception.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/util/unique_names.hpp>
#include <wge/graphics/graphics.hpp>
#include <wge/core/behavior_system.hpp>
#include <wge/math/transform.hpp>
#include <wge/util/ptr.hpp>

#include "editor.hpp"
#include "history.hpp"
#include "context.hpp"
#include "component_inspector.hpp"
#include "imgui_editor_tools.hpp"
#include "imgui_ext.hpp"

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <wge/graphics/glfw_backend.hpp>

#include <functional>

namespace wge::editor
{

// Creates an imgui dockspace in the main window
inline void main_viewport_dock()
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
	ImGui::DockSpace(ImGui::GetID("_Dockspace"), ImVec2(0.0f, 0.0f), dockspace_flags);

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

class sprite_editor
{
public:
	sprite_editor(context& pContext) :
		mContext(&pContext)
	{
		mConnection_on_new_selection = pContext.on_new_selection.connect(
			[&]()
		{
			mSelected_animation_id = util::uuid{};
		});

		// Tell the context that the asset was modified
		on_change.connect([this]()
		{
			mContext->mark_selection_as_modified();
		});
	}

	virtual ~sprite_editor()
	{
		mConnection_on_new_selection.disconnect();
	}

	void on_gui()
	{
		if (ImGui::Begin("Sprite Editor", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
		{
			auto selection = mContext->get_selection<selection_type::asset>();
			if (selection && selection->get_type() == "texture")
			{
				auto texture = core::cast_asset<graphics::texture>(selection);

				float* zoom = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("_Zoom"), 0);

				const ImVec2 last_cursor = ImGui::GetCursorPos();
				ImGui::BeginGroup();

				const float scale = std::powf(2, *zoom);
				const ImVec2 image_size((float)texture->get_width() * scale, (float)texture->get_height() * scale);

				// Top and left padding
				ImGui::Dummy(ImVec2(image_size.x + ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2));
				ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, image_size.y));
				ImGui::SameLine();

				// Store the cursor so we can position things on top of the image
				const ImVec2 image_position = ImGui::GetCursorScreenPos();

				ImGui::DrawAlphaCheckerBoard(image_position, image_size);

				ImGui::Image(texture, image_size);

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

					if (box_edit.is_dragging() && ImGui::IsMouseReleased(0))
						on_change();

					visual_editor::end_snap();
				}

				// Select a new one
				if (!was_dragging && ImGui::IsMouseReleased(0))
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
			}
			else
			{
				ImGui::TextUnformatted("No texture asset selected");
			}
		}
		ImGui::End();
	}

	static void preview_image(const char* pStr_id, const graphics::texture::ptr& pTexture, const math::vec2& pSize, const math::rect& pFrame_rect)
	{
		// Scale the size of the image to preserve the aspect ratio but still fit in the
		// specified area.
		math::vec2 scaled_size =
		{
			math::min(pFrame_rect.size.x * (pSize.y / pFrame_rect.size.y), pSize.x),
			math::min(pFrame_rect.size.y * (pSize.x / pFrame_rect.size.x), pSize.y)
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

	void on_inspector_gui()
	{
		const auto open_sprite_editor = []()
		{
			ImGui::Begin("Sprite Editor");
			ImGui::SetWindowFocus();
			ImGui::End();
		};

		auto selection = mContext->get_selection<selection_type::asset>();
		auto texture = core::cast_asset<graphics::texture>(selection);

		if (ImGui::CollapsingHeader("Atlas", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button("Open Sprite Editor", { -1, 0 }))
				open_sprite_editor();

			// Atlas list
			ImGui::BeginChild("_AtlasList", { 0, 200 }, true);
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

			if (ImGui::Button("Add"))
			{
				graphics::animation& animation = texture->get_raw_atlas().emplace_back();
				animation.frame_rect = math::rect({ 0, 0 }, texture->get_size());
				animation.name = make_unique_animation_name(texture, "NewEntry");
				animation.id = util::generate_uuid();
				on_change();
			}

			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				on_change();
			}

			if (graphics::animation* selected_animation = texture->get_animation(mSelected_animation_id))
			{
				ImGui::PushID("_AnimationSettings");

				preview_image("LargePreviewImage", texture, { ImGui::GetWindowContentRegionWidth(), 200 }, selected_animation->frame_rect);

				ImGui::Button("Play");
				static int a = 0;
				ImGui::SliderInt("Frame", &a, 0, selected_animation->frames);
				
				ImGui::InputText("Name", &selected_animation->name);
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					std::string temp = std::move(selected_animation->name);
					selected_animation->name = make_unique_animation_name(texture, temp);
					on_change();
				}
				ImGui::DragFloat2("Position", selected_animation->frame_rect.position.components); check_if_edited();
				ImGui::DragFloat2("Size", selected_animation->frame_rect.size.components); check_if_edited();

				ImGui::PopID();
			}
		}
	}

	util::signal<void()> on_change;

private:
	static std::string make_unique_animation_name(graphics::texture::ptr pTexture, const std::string& pName)
	{
		return util::create_unique_name(pName,
			pTexture->get_raw_atlas().begin(), pTexture->get_raw_atlas().end(),
			[](auto& i) -> const std::string& { return i.name; });
	}

	void check_if_edited()
	{
		if (ImGui::IsItemDeactivated())
			on_change();
	}

private:
	context* mContext;
	util::connection mConnection_on_new_selection;
	util::uuid mSelected_animation_id;
};

class mybehavior :
	public core::behavior_instance
{
public:
	virtual ~mybehavior() {}
	virtual void on_update(float pDelta, core::game_object pObject) override
	{
		auto& pLayer = pObject.get_layer();
		//auto transform = pObject.get_component<core::transform_component>();
		//transform->set_position(transform->get_position() + math::vec2(0.1f, 0) * pDelta);

		if (auto physics = pObject.get_component<physics::physics_component>())
		{
			physics->apply_force(math::vec2(0.1f, 0));
		}
	}
};

class application
{
public:
	application()
	{
		mGame_context.set_asset_manager(&mAsset_manager);
	}

	int run()
	{
		// Only glfw and opengl is supported for editing
		mGraphics.initialize(graphics::window_backend_type::glfw, graphics::backend_type::opengl);
		mGLFW_backend = std::dynamic_pointer_cast<graphics::glfw_window_backend>(mGraphics.get_window_backend());
		mViewport_framebuffer = mGraphics.get_graphics_backend()->create_framebuffer();

		init_imgui();
		init_game_context();
		init_inspectors();

		mainloop();
		return 0;
	}

private:
	void mainloop()
	{
		while (!glfwWindowShouldClose(mGLFW_backend->get_window()))
		{
			float delta = 1.f / 60.f;

			if (mUpdate)
			{
				mGame_context.preupdate(delta);
				mGame_context.update(delta);
			}

			new_frame();
			main_viewport_dock();

			if (ImGui::BeginMainMenuBar())
			{
				ImGui::TextColored({ 0.5, 0.5, 0.5, 1 }, "WGE");
				if (ImGui::BeginMenu("Project"))
				{
					ImGui::MenuItem("New");
					ImGui::MenuItem("Open");
					ImGui::Separator();
					//ImGui::MenuItem("Save", "Ctrl+S", false, mContext.are_there_modified_assets());
					if (ImGui::MenuItem("Save All", "Ctrl+Alt+S", false, mContext.are_there_modified_assets()))
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
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			show_settings();
			show_asset_manager();
			show_viewport();
			show_objects();
			show_inspector();
			mSprite_editor.on_gui();

			if (mUpdate)
				mGame_context.postupdate(delta);

			// Clear the framebuffer with black
			mViewport_framebuffer->clear({ 0, 0, 0, 1 });

			// Render all layers with the renderer system enabled
			for (auto& i : mGame_context.get_layer_container())
			{
				if (auto renderer = i->get_system<graphics::renderer>())
				{
					renderer->set_framebuffer(mViewport_framebuffer);
					renderer->set_render_view_to_framebuffer(mViewport_offset, 1 / mViewport_scale);
					renderer->render(mGraphics);
				}
			}

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
		ImGui::CreateContext();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		ImGui::GetIO().ConfigDockingWithShift = true;
		ImGui_ImplGlfw_InitForOpenGL(mGLFW_backend->get_window(), true);
		ImGui_ImplOpenGL3_Init("#version 150");

		auto font = ImGui::GetIO().Fonts->AddFontFromFileTTF("./editor/Roboto-Medium.ttf", 16);
		if (font == NULL)
		{
			log::error() << "Could not load editor font, aborting..." << log::endm;
			std::abort();
		}

		// Theme
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
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

	void init_game_context()
	{
		mFactory.register_system<graphics::renderer>();
		mFactory.register_system<physics::physics_world>();
		mFactory.register_component<core::transform_component>();
		mFactory.register_component<graphics::sprite_component>();
		mFactory.register_component<physics::physics_component>();
		mFactory.register_component<physics::box_collider_component>();
		mGame_context.set_factory(&mFactory);

		mAsset_manager.register_asset<core::asset>("scene");
		mAsset_manager.register_serial_config_extension("scene", ".wgescene");
		mAsset_manager.register_serial_config_extension("gameobject", ".wgegameobject");
		mAsset_manager.register_serial_config_extension("layer", ".wgelayer");

		mAsset_manager.register_asset("texture",
			[&](const filesystem::path& pPath, core::asset_config::ptr pConfig) -> core::asset::ptr
		{
			auto ptr = std::make_shared<graphics::texture>(pConfig);
			ptr->set_implementation(mGraphics.get_graphics_backend()->create_texture_implementation());
			ptr->set_path(pPath);
			auto path = pConfig->get_path();
			path.remove_extension();
			ptr->load(path.string());
			return ptr;
		});
		mAsset_manager.register_resource_extension("texture", ".png");

		mAsset_manager.register_asset("behavior",
			[&](const filesystem::path& pPath, core::asset_config::ptr pConfig) -> core::asset::ptr
		{
			auto ptr = std::make_shared<core::behavior>(pConfig);
			ptr->set_path(pPath);
			ptr->set_instance_factory([](const core::behavior&)
			{
				return std::make_shared<mybehavior>();
			});
			return ptr;
		});
		mAsset_manager.register_resource_extension("behavior", ".as");

		mAsset_manager.set_root_directory(".");
		mAsset_manager.load_assets();
		
		auto layer = mGame_context.add_layer();
		layer->set_name("Layer1");
		layer->add_system<graphics::renderer>();

		auto renderer = layer->get_system<graphics::renderer>();
		renderer->set_pixel_size(0.01f);

		auto obj = layer->add_object();
		obj.add_component<core::transform_component>();
		auto sprite = obj.add_component<graphics::sprite_component>();
		sprite->set_texture(mAsset_manager.get_asset<graphics::texture>("mytex.png"));
	}

	void init_inspectors()
	{
		// Inspector for transform_component
		mInspectors.add_inspector(core::transform_component::COMPONENT_ID,
			[](core::component* pComponent)
		{
			auto reset_context_menu = [](const char * pId)->bool
			{
				bool clicked = false;
				if (ImGui::BeginPopupContextItem(pId))
				{
					if (ImGui::Button("Reset"))
					{
						clicked = true;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				return clicked;
			};

			auto transform = dynamic_cast<core::transform_component*>(pComponent);
			math::vec2 position = transform->get_position();
			if (ImGui::DragFloat2("Position", position.components))
				transform->set_position(position);
			if (reset_context_menu("posreset"))
				transform->set_position(math::vec2(0, 0));

			float rotation = math::degrees(transform->get_rotation());
			if (ImGui::DragFloat("Rotation", &rotation, 1, 0, 0, "%.3f degrees"))
				transform->set_rotaton(math::degrees(rotation));
			if (reset_context_menu("rotreset"))
				transform->set_rotaton(0);

			math::vec2 scale = transform->get_scale();
			if (ImGui::DragFloat2("Scale", scale.components, 0.01f))
				transform->set_scale(scale);
			if (reset_context_menu("scalereset"))
				transform->set_scale(math::vec2(0, 0));
		});


		// Inspector for sprite_component
		mInspectors.add_inspector(graphics::sprite_component::COMPONENT_ID,
			[this](core::component* pComponent)
		{
			auto sprite = dynamic_cast<graphics::sprite_component*>(pComponent);
			math::vec2 offset = sprite->get_offset();
			if (ImGui::DragFloat2("Offset", offset.components))
				sprite->set_offset(offset);

			graphics::texture::ptr tex = sprite->get_texture();
			std::string inputtext = tex ? tex->get_path().string().c_str() : "None";
			ImGui::BeginGroup();
			if (tex)
			{
				if (ImGui::ImageButton(tex, { 100, 100 }))
				{
					mContext.set_selection(tex);
				}
				ImGui::SameLine();
				ImGui::BeginGroup();
				ImGui::Text("Size: %i, %i", tex->get_width(), tex->get_height());
				ImGui::Text("Animations: %u", tex->get_raw_atlas().size());
				ImGui::EndGroup();
			}
			ImGui::InputText("Texture", &inputtext, ImGuiInputTextFlags_ReadOnly);
			ImGui::EndGroup();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("textureAsset"))
				{
					const util::uuid& id = *(const util::uuid*)payload->Data;
					auto asset = mAsset_manager.get_asset<graphics::texture>(id);
					sprite->set_texture(asset);
				}
				ImGui::EndDragDropTarget();
			}
		});

		// Inspector for behavior_component
		mInspectors.add_inspector(core::behavior_component::COMPONENT_ID,
			[this](core::component* pComponent)
		{
			auto comp = dynamic_cast<core::behavior_component*>(pComponent);

			auto behavior = comp->get_behavior();
			ImGui::BeginGroup();
			std::string inputtext = behavior ? behavior->get_path().string().c_str() : "None";
			ImGui::InputText("Texture", &inputtext, ImGuiInputTextFlags_ReadOnly);
			ImGui::EndGroup();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("behaviorAsset"))
				{
					const util::uuid& id = *(const util::uuid*)payload->Data;
					auto asset = mAsset_manager.get_asset<core::behavior>(id);
					comp->set_behavior(asset);
				}
				ImGui::EndDragDropTarget();
			}
		});

		mInspectors.add_inspector(physics::physics_component::COMPONENT_ID,
			[](core::component* pComponent)
		{
			auto physics = dynamic_cast<physics::physics_component*>(pComponent);
			std::array options = { "Dynamic", "Static" };
			if (ImGui::BeginCombo("Type", options[physics->get_type()]))
			{
				for (std::size_t i = 0; i < options.size(); i++)
					if (ImGui::Selectable(options[i]))
						physics->set_type(i);
				ImGui::EndCombo();
			}
		});

		// Inspector for box_collider_component
		mInspectors.add_inspector(physics::box_collider_component::COMPONENT_ID,
			[](core::component* pComponent)
		{
			auto collider = dynamic_cast<physics::box_collider_component*>(pComponent);

			math::vec2 offset = collider->get_offset();
			if (ImGui::DragFloat2("Offset", offset.components))
				collider->set_offset(offset);

			math::vec2 size = collider->get_size();
			if (ImGui::DragFloat2("Size", size.components))
				collider->set_size(size);

			float rotation = math::degrees(collider->get_rotation());
			if (ImGui::DragFloat("Rotation", &rotation))
				collider->set_rotation(math::degrees(rotation));
		});
	}

private:
	void show_asset_manager()
	{
		if (ImGui::Begin("Asset Manager"))
		{
			static filesystem::path current_path;

			ImGui::Columns(2, "_AssetColumns");
			ImGui::SetColumnWidth(0, 100);

			ImGui::TextUnformatted("Type:");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Path:");
			ImGui::NextColumn();
			
			/*auto root = mAsset_manager.get_file_structure().find(current_path);

			for (auto& i : root.child())
			{
				core::asset::ptr asset = i.second->has_value() ? i.second->value() : core::asset::ptr{};
				if (asset)
				{
					const bool asset_is_selected = mContext.get_selection<selection_type::asset>() == asset;
					if (ImGui::Selectable(i.first->c_str(), asset_is_selected))
						mContext.set_selection(asset);
				}
				else
				{
					if (ImGui::Selectable(i.first->c_str()))
					{
						current_path = root.child();
					}
				}
			}*/
			
			for (auto& i : mAsset_manager.get_asset_list())
			{
				ImGui::PushID(i->get_id().to_hash32());
				const bool asset_is_selected = mContext.get_selection<selection_type::asset>() == i;
				if (ImGui::Selectable(i->get_type().c_str(), asset_is_selected, ImGuiSelectableFlags_SpanAllColumns))
					mContext.set_selection(i);
				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload((i->get_type() + "Asset").c_str(), &i->get_id(), sizeof(util::uuid));
					ImGui::Text("Asset: %s", i->get_path().string().c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::NextColumn();
				ImGui::TextUnformatted(i->get_path().string().c_str());
				ImGui::NextColumn();
				ImGui::PopID();
			}
			ImGui::Columns();
		}
		ImGui::End();
	}

	bool create_aabb_from_object(core::game_object& pObj, math::aabb& pAABB)
	{
		bool has_aabb = false;
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
		}
		return has_aabb;
	}

	void show_settings()
	{
		if (ImGui::Begin("Settings"))
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

	void show_viewport()
	{
		if (ImGui::Begin("Game"))
		{
			// Idea for later
			//ImGui::BeginGroup();
			//if (ImGui::Button("Open", { 70, 35 - ImGui::GetStyle().ItemSpacing.y / 2 }));
			//if (ImGui::Button("Save", { 70, 35 - ImGui::GetStyle().ItemSpacing.y / 2 }));
			//ImGui::EndGroup();
			//ImGui::SameLine();
			ImVec4 play_color = mUpdate ? ImVec4{ 0.4f, 0.1f, 0.1f, 1 } : ImVec4{ 0.1f, 0.4f, 0.1f, 1 };
			ImGui::PushStyleColor(ImGuiCol_Button, play_color);
			if (ImGui::Button(mUpdate ? "Pause" : "Play", { 70, 70 }))
				mUpdate = !mUpdate;
			ImGui::PopStyleColor();
		}
		ImGui::End();

		if (ImGui::Begin("Viewport", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar))
		{
			static bool show_center_point = true;
			static bool is_grid_enabled = false;
			static graphics::color grid_color{ 1, 1, 1, 0.7f };
			static bool show_collision = false;

			if (ImGui::BeginMenuBar())
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
			}

			float width = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2 - ImGui::GetStyle().ScrollbarSize;
			float height = ImGui::GetWindowHeight() - ImGui::GetCursorPos().y - ImGui::GetStyle().WindowPadding.y - ImGui::GetStyle().ScrollbarSize;
			float scroll_x_max = width * 2;
			float scroll_y_max = height * 2;

			if (mViewport_framebuffer->get_width() != width
				|| mViewport_framebuffer->get_height() != height)
				mViewport_framebuffer->resize(static_cast<int>(width), static_cast<int>(height));

			ImVec2 topleft_cursor = ImGui::GetCursorScreenPos();
			ImGui::Button("this thing");
			ImGui::SetCursorScreenPos(topleft_cursor);
			
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
				auto selected_layer = get_current_layer();
				if (ImGui::BeginDragDropTarget() && selected_layer)
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("gameobjectAsset"))
					{
						core::game_object obj = selected_layer->add_object();
						const util::uuid& id = *(const util::uuid*)payload->Data;
						auto asset = mAsset_manager.find_asset(id);
						obj.deserialize(asset->get_config()->get_metadata());
						if (auto transform = obj.get_component<core::transform_component>())
							transform->set_position(visual_editor::get_mouse_position());
						mContext.set_selection(obj);
					}
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("layerAsset"))
					{
						const util::uuid& id = *(const util::uuid*)payload->Data;
						auto asset = mAsset_manager.find_asset(id);
						auto new_layer = mGame_context.add_layer();
						new_layer->deserialize(asset->get_config()->get_metadata());
						mContext.set_selection(new_layer);
					}
					ImGui::EndDragDropTarget();
				}

				if (is_grid_enabled)
					visual_editor::draw_grid(grid_color, 1);

				for (auto& layer : mGame_context.get_layer_container())
				{
					graphics::renderer* renderer = layer->get_system<graphics::renderer>();
					if (!renderer)
						continue;
					// Make sure we are working with the viewports framebuffer
					renderer->set_framebuffer(mViewport_framebuffer);

					if (show_collision)
					{
						auto& box_collider_container = layer->get_component_container<physics::box_collider_component>();
						for (auto& i : box_collider_container)
						{
							auto transform = layer->get_object(i.get_object_id()).get_component<core::transform_component>();
							if (!transform)
								continue;
							visual_editor::push_transform(transform->get_transform());
							math::transform box_transform;
							box_transform.position = i.get_offset();
							box_transform.rotation = i.get_rotation();
							visual_editor::push_transform(box_transform);
							visual_editor::draw_rect(math::rect({ 0, 0 }, i.get_size()), { 0, 1, 0, 0.8f });
							visual_editor::pop_transform(2);
						}
					}

					for (std::size_t i = 0; i < layer->get_object_count(); i++)
					{
						auto obj = layer->get_object(i);
						auto transform = obj.get_component<core::transform_component>();
						if (!transform)
							continue;

						// Check for selection
						auto selection = mContext.get_selection<selection_type::game_object>();
						const bool is_object_selected = selection && obj == *selection;

						math::aabb aabb;
						if (create_aabb_from_object(obj, aabb))
						{
							if (is_object_selected)
							{
								visual_editor::box_edit box_edit(aabb, transform->get_transform());
								box_edit.resize(visual_editor::edit_type::transform);
								box_edit.drag(visual_editor::edit_type::transform);
								transform->set_transform(box_edit.get_transform());
							}

							visual_editor::push_transform(transform->get_transform());
							if (aabb.intersect(visual_editor::get_mouse_position()))
							{
								if (ImGui::IsItemClicked())
									mContext.set_selection(obj);
								visual_editor::draw_rect(aabb, { 1, 1, 1, 1 });
							}
							visual_editor::pop_transform();
						}

						// Draw center point
						if (is_object_selected && show_center_point)
						{
							visual_editor::draw_circle(transform->get_position(), 5, { 1, 1, 1, 0.6f }, 3.f);
						}
					}
				}
			}
			visual_editor::end();

			ImGui::EndFixedScrollRegion();
		}
		ImGui::End();
	}

	static filesystem::path ensure_extension(filesystem::path pPath, const char* pExtension)
	{
		if (pPath.extension() != pExtension)
		{
			auto filename = pPath.filename();
			pPath.pop_filepath();
			pPath.push_back(filename + pExtension);
		}
		return pPath;
	}

	void show_layer_objects(const core::layer::ptr& pLayer)
	{
		for (std::size_t i = 0; i < pLayer->get_object_count(); i++)
		{
			core::game_object obj = pLayer->get_object(i);
			ImGui::PushID(obj.get_instance_id().to_hash32());

			const auto selection = mContext.get_selection<selection_type::game_object>();
			const bool is_selected = selection && selection->get_instance_id() == obj.get_instance_id();
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | (is_selected ? ImGuiTreeNodeFlags_Selected : 0);
			if (ImGui::TreeNodeEx((obj.get_name() + "###GameObject").c_str(), flags))
			{
				if (ImGui::IsItemClicked() || ImGui::IsItemClicked(1)) // Take both left and right click
					mContext.set_selection(obj);
				ImGui::TreePop();
			}

			ImGui::PopID();

			if (ImGui::IsItemClicked(1))
				ImGui::OpenPopup("ObjectContextMenu");
		}

		if (ImGui::BeginPopup("ObjectContextMenu"))
		{
			auto object = mContext.get_selection<selection_type::game_object>();
			if (!object)
				ImGui::CloseCurrentPopup();
			if (ImGui::BeginMenu("Save to asset..."))
			{
				static std::string destination;
				ImGui::InputText("Destination", &destination);

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Save"))
				{
					json data = object->serialize(core::serialize_type::properties);
					auto path_with_extension = ensure_extension(destination, ".wgegameobject");
					mAsset_manager.create_serialized_asset(path_with_extension, "gameobject", data);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Duplicate"))
			{
				json data = object->serialize(core::serialize_type::properties);
				object->get_layer().add_object().deserialize(data);
			}
			if (ImGui::MenuItem("Delete"))
			{
				mContext.reset_selection();
				object->get_layer().remove_object(*object);
			}
			ImGui::EndPopup();
		}
	}

	void show_layers()
	{
		bool open_context_menu = false;
		for (auto& i : mGame_context.get_layer_container())
		{
			ImGui::PushID(util::to_address(i));

			bool is_selected = get_current_layer() == util::to_address(i);

			ImVec4 color = is_selected ? ImVec4(0.2f, 0.4f, 0.4f, 1) : ImVec4(0.2f, 0.4f, 0.4f, 0.5f);
			ImGui::PushStyleColor(ImGuiCol_Header, color);

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ImGuiTreeNodeFlags_OpenOnArrow | (is_selected ? ImGuiTreeNodeFlags_Selected : 0);
			bool open = ImGui::TreeNodeEx((i->get_name() + "###Layer").c_str(), flags);

			if (ImGui::IsItemClicked(1))
				open_context_menu = true;

			ImGui::PopStyleColor();

			if (ImGui::IsItemClicked())
				mContext.set_selection(i);
			if (open)
			{
				show_layer_objects(i);
				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (open_context_menu)
			ImGui::OpenPopup("LayerContextMenu");

		if (ImGui::BeginPopup("LayerContextMenu"))
		{
			auto layer = get_current_layer();
			if (ImGui::BeginMenu("Save to asset..."))
			{
				// Will be implemented
				static std::string destination;
				ImGui::InputText("Destination", &destination);

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Save"))
				{
					json data = layer->serialize(core::serialize_type::properties);
					auto path_with_extension = ensure_extension(destination, ".wgelayer");
					mAsset_manager.create_serialized_asset(path_with_extension, "layer", data);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}
			ImGui::MenuItem("Duplicate");
			if (ImGui::MenuItem("Delete"))
			{
				mContext.reset_selection();
				mGame_context.remove_layer(layer);
			}
			ImGui::EndPopup();
		}
	}

	void show_objects()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, ImGui::GetStyle().WindowPadding.y));
		if (ImGui::Begin("Objects", NULL, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Add"))
				{
					if (ImGui::MenuItem("Layer"))
					{
						mGame_context.add_layer();
					}
					if (ImGui::MenuItem("Object 2D"))
					{
						// Find a layer to create the object in
						core::layer* layer = nullptr;
						if (auto selected_layer = mContext.get_selection<selection_type::layer>())
							layer = &(*selected_layer);
						else if (auto selected_object = mContext.get_selection<selection_type::game_object>())
							layer = &selected_object->get_layer();

						// Create the object
						if (layer)
						{
							auto obj = layer->add_object();
							obj.set_name("New 2D Object");
							obj.add_component<core::transform_component>();
						}
						else
						{
							log::error() << "Cannot create object; No layer or object is selected" << log::endm;
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			show_layers();
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void show_inspector()
	{
		if (ImGui::Begin("Inspector"))
		{
			if (auto selection = mContext.get_selection<selection_type::game_object>())
				show_component_inspector(*selection);
			else if (auto selection = mContext.get_selection<selection_type::layer>())
				show_layer_inspector(*selection);
			else if (auto selection = mContext.get_selection<selection_type::asset>())
				show_asset_inspector(selection);
		}
		ImGui::End();
	}

	void show_asset_inspector(const core::asset::ptr& pAsset)
	{
		std::string path = pAsset->get_path().string();
		ImGui::InputText("Name", &path, ImGuiInputTextFlags_ReadOnly);
		ImGui::LabelText("Asset ID", pAsset->get_id().to_string().c_str());

		std::string description = pAsset->get_config()->get_description();
		if (ImGui::InputText("Description", &description))
			pAsset->get_config()->set_description(description);
		if (ImGui::IsItemDeactivatedAfterEdit())
			mContext.add_modified_asset(pAsset);

		if (mContext.is_asset_modified(pAsset))
		{
			if (ImGui::Button("Save", { -1, 0 }))
				mContext.save_asset(pAsset);
		}

		ImGui::Separator();
		
		if (pAsset->get_type() == "texture")
			mSprite_editor.on_inspector_gui();
	}

	void show_layer_inspector(core::layer& pLayer)
	{
		std::string name = pLayer.get_name();
		if (ImGui::InputText("Name", &name))
			pLayer.set_name(name);
		ImGui::Separator();
		ImGui::BeginTabBar("LayerSystems");

		if (ImGui::BeginTabItem("Rendering"))
		{
			auto renderer = pLayer.get_system<graphics::renderer>();
			if (!renderer)
			{
				if (ImGui::Button("Enable"))
					pLayer.add_system<graphics::renderer>();
			}
			else
			{
				float pixel_size = renderer->get_pixel_size();
				if (ImGui::DragFloat("Pixel Size", &pixel_size, 0.01f))
					renderer->set_pixel_size(pixel_size);
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Physics"))
		{
			auto physics = pLayer.get_system<physics::physics_world>();
			if (!physics)
			{
				if (ImGui::Button("Enable"))
					pLayer.add_system<physics::physics_world>();
			}
			else
			{
				math::vec2 gravity = physics->get_gravity();
				if (ImGui::DragFloat2("Gravity", gravity.components))
					physics->set_gravity(gravity);
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Behaviors"))
		{
			auto sys = pLayer.get_system<core::behavior_system>();
			if (!sys)
			{
				if (ImGui::Button("Enable"))
					pLayer.add_system<core::behavior_system>();
			}
			else
			{
			}
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	void show_component_inspector(core::game_object pObj)
	{
		std::string name = pObj.get_name();
		if (ImGui::InputText("Name", &name))
			pObj.set_name(name);
		ImGui::Separator();
		for (std::size_t i = 0; i < pObj.get_component_count(); i++)
		{
			core::component* comp = pObj.get_component_at(i);
			ImGui::PushID(comp);

			if (i != 0)
				ImGui::Separator();

			bool open = collapsing_arrow("CollapsingArrow", nullptr, true);

			ImGui::SameLine();
			{
				ImGui::PushItemWidth(150);
				std::string name = comp->get_name();
				if (ImGui::InputText("##NameInput", &name))
					comp->set_name(name);
				ImGui::PopItemWidth();
			}

			ImGui::SameLine();
			ImGui::Text(comp->get_component_name().c_str());

			ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionWidth()
				- (ImGui::CalcTextSize("Delete ").x
					+ ImGui::GetStyle().WindowPadding.x * 2
					+ ImGui::GetStyle().FramePadding.x * 2), 1));
			ImGui::SameLine();

			bool delete_component = false;
			delete_component = ImGui::Button("Delete");

			if (open)
				mInspectors.on_gui(comp);

			ImGui::PopID();
			if (delete_component)
				pObj.remove_component(i--);
		}

		ImGui::Separator();
		if (ImGui::BeginCombo("###Add Component", "Add Component"))
		{
			if (ImGui::Selectable("Transform 2D"))
				pObj.add_component<core::transform_component>();
			if (ImGui::Selectable("Physics"))
				pObj.add_component<physics::physics_component>();
			if (ImGui::Selectable("Box Collider"))
				pObj.add_component<physics::box_collider_component>();
			if (ImGui::Selectable("Sprite"))
				pObj.add_component<graphics::sprite_component>();
			if (ImGui::Selectable("Behavior"))
				pObj.add_component<core::behavior_component>();
			//if (ImGui::Selectable("Script"))
			//	mSelected_node->add_component<script_component>();
			ImGui::EndCombo();
		}
	}

	core::layer* get_current_layer() const
	{
		if (auto layer = mContext.get_selection<selection_type::layer>())
			return &(*layer);

		if (auto obj = mContext.get_selection<selection_type::game_object>())
			return &obj->get_layer();

		return nullptr;
	}

private:
	graphics::graphics mGraphics;
	// ImGui needs access to some glfw specific objects
	graphics::glfw_window_backend::ptr mGLFW_backend;

	context mContext;

	bool mDragging{ false };
	math::vec2 mDrag_offset;

	graphics::framebuffer::ptr mViewport_framebuffer;
	math::vec2 mViewport_offset;
	math::vec2 mViewport_scale{ 100, 100 };

	core::context mGame_context;
	filesystem::path mGame_path;
	core::asset_manager mAsset_manager;
	core::factory mFactory;

	component_inspector mInspectors;

	bool mUpdate{ false };

	sprite_editor mSprite_editor{ mContext };
};

void GLAPIENTRY opengl_message_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH: log::out << log::level::error; break;
	case GL_DEBUG_SEVERITY_MEDIUM: log::out << log::level::warning; break;
	case GL_DEBUG_SEVERITY_LOW: log::out << log::level::warning; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: log::out << log::level::info; break;
	default: log::out << log::level::unknown;
	}
	log::out << "OpenGL: " << message << log::endm;
}

int main(int argc, char ** argv)
{
	log::open_file("./editor/log.txt");
	application app;
	return app.run();
}

} // namespace wge::editor
