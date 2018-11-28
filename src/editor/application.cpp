
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

#include "editor.hpp"
#include "history.hpp"
#include "context.hpp"
#include "component_inspector.hpp"
#include "imgui_editor_tools.hpp"
#include "imgui_ext.hpp"

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <functional>

namespace wge::editor
{

class object_editor :
	public editor
{

};

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
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

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

class application
{
public:
	application()
	{
		mGame_context.set_asset_manager(&mAsset_manager);
	}

	int run()
	{
		init_glfw();
		init_imgui();
		init_game_context();
		init_inspectors();
		mViewport_framebuffer.create(200, 200); // Some arbitrary default
		mainloop();
		return 0;
	}

private:
	void mainloop()
	{
		while (!glfwWindowShouldClose(mWindow))
		{
			float delta = 1.f / 60.f;

			new_frame();
			main_viewport_dock();

			for (auto& i : mEditors)
			{
				i->on_update(delta);
				i->on_gui(delta);
			}

			show_settings();
			show_asset_manager();
			show_viewport();
			show_objects();
			show_inspector();

			// Clear the framebuffer with black
			mViewport_framebuffer.begin_framebuffer();
			mViewport_framebuffer.clear({ 0, 0, 0, 1 });
			mViewport_framebuffer.end_framebuffer();

			// Render all layers with the renderer system enabled
			for (auto& i : mGame_context.get_layer_container())
			{
				auto renderer = i->get_system<graphics::renderer>();
				if (!renderer)
					continue;
				renderer->set_framebuffer(&mViewport_framebuffer);
				renderer->set_render_view({
					{ 0.f, 0.f },
					{ (float)mViewport_framebuffer.get_width() * 0.01f, (float)mViewport_framebuffer.get_height() * 0.01f }
					});
				renderer->render();
				renderer->clear();
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

		// Cleanup GLFW
		glfwDestroyWindow(mWindow);
		glfwTerminate();
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
		glfwMakeContextCurrent(mWindow);
		glfwGetFramebufferSize(mWindow, &display_w, &display_h);
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

		glfwMakeContextCurrent(mWindow);
		glfwSwapBuffers(mWindow);
	}

	void init_glfw()
	{
		glfwInit();

		// OpenGL 3.3 minimum
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Make Mac happy
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // No old OpenGL

		// Create our window and initialize the opengl context
		mWindow = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
		glfwMakeContextCurrent(mWindow);
		// Load extensions
		glewInit();

		// Match the monitors refresh rate (VSync)
		glfwSwapInterval(1);

		// Enable blending (for transparancy)
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDebugMessageCallback(opengl_message_callback, 0);
	}

	void init_imgui()
	{
		ImGui::CreateContext();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		ImGui::GetIO().ConfigDockingWithShift = true;
		ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
		ImGui_ImplOpenGL3_Init("#version 150");
	}

	void init_game_context()
	{
		mAsset_manager.add_loader("texture", std::make_shared<graphics::texture_asset_loader>());
		mAsset_manager.add_loader("scene", std::make_shared<core::config_asset_loader>());
		mAsset_manager.set_root_directory(".");
		mAsset_manager.load_assets();

		auto layer = mGame_context.create_layer();
		layer->set_name("Layer1");
		layer->add_system<graphics::renderer>();

		auto renderer = layer->get_system<graphics::renderer>();
		renderer->set_pixel_size(0.01);

		auto obj = layer->create_object();
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
				ImGui::ImageButton(tex, { 150, 150 });
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
					core::asset_uid id = *(core::asset_uid*)payload->Data;
					auto asset = mAsset_manager.get_asset<graphics::texture>(id);
					sprite->set_texture(asset);
				}
				ImGui::EndDragDropTarget();
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
			ImGui::Columns(3, "_AssetColumns");
			ImGui::SetColumnWidth(0, 100);

			ImGui::TextUnformatted("Type:");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Path:");
			ImGui::NextColumn();
			ImGui::TextUnformatted("UID:");
			ImGui::NextColumn();

			for (auto& i : mAsset_manager.get_asset_list())
			{
				ImGui::PushID(i->get_id());
				const bool asset_is_selected = mContext.get_selection<selection_type::asset>() == i;
				if (ImGui::Selectable(i->get_type().c_str(), asset_is_selected, ImGuiSelectableFlags_SpanAllColumns))
					mContext.set_selection(i);
				if (ImGui::BeginDragDropSource())
				{
					core::asset_uid id = i->get_id();
					ImGui::SetDragDropPayload((i->get_type() + "Asset").c_str(), &id, sizeof(core::asset_uid));
					ImGui::Text("Asset: %s", i->get_path().string().c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::NextColumn();
				ImGui::TextUnformatted(i->get_path().string().c_str());
				ImGui::NextColumn();
				ImGui::TextUnformatted(std::to_string(i->get_id()).c_str());
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
			auto comp = pObj.get_component_index(comp_idx);
			if (!comp->has_aabb())
				continue;
			if (!has_aabb)
			{
				pAABB = comp->get_screen_aabb();
				has_aabb = true;
			}
			else
				pAABB.merge(comp->get_screen_aabb());
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
		if (ImGui::Begin("Viewport"))
		{
			//mygameinput.set_enabled(ImGui::IsWindowFocused());

			float width = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2;
			float height = ImGui::GetWindowHeight() - ImGui::GetCursorPos().y - ImGui::GetStyle().WindowPadding.y;

			if (mViewport_framebuffer.get_width() != width
				|| mViewport_framebuffer.get_height() != height)
				mViewport_framebuffer.resize(width, height);

			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImGui::Image(mViewport_framebuffer, ImVec2(width, height));

			visual_editor::begin_editor("_SceneEditor", { cursor.x, cursor.y }, { 1, 1 });
			{
				ImDrawList* dl = ImGui::GetWindowDrawList();

				const math::vec2 mouse(ImGui::GetMousePos().x - cursor.x, ImGui::GetMousePos().y - cursor.y);
				for (auto layer : mGame_context.get_layer_container())
				{
					graphics::renderer* renderer = layer->get_system<graphics::renderer>();
					if (!renderer)
						continue;
					// Make sure we are working with the viewports framebuffer
					renderer->set_framebuffer(&mViewport_framebuffer);
					
					const math::vec2 render_view_scale = renderer->get_render_view_scale();
					for (std::size_t i = 0; i < layer->get_object_count(); i++)
					{
						auto obj = layer->get_object(i);
						auto transform = obj.get_component<core::transform_component>();

						// Check for selection
						auto selection = mContext.get_selection<selection_type::game_object>();
						const bool is_object_selected = selection && obj == *selection;

						// Draw center point
						if (is_object_selected)
						{
							math::vec2 center = transform->get_absolute_position() / render_view_scale;
							dl->AddCircle({ center.x + cursor.x, center.y + cursor.y },
								5, ImGui::GetColorU32({ 1, 1, 1, 0.6f }), 12, 3);
						}

						math::aabb aabb;
						if (create_aabb_from_object(obj, aabb))
						{
							aabb.min /= render_view_scale;
							aabb.max /= render_view_scale;

							if (is_object_selected)
							{
								const math::rect rect(aabb.min, aabb.max - aabb.min);
								math::vec2 delta;
								if (visual_editor::drag_rect("_SelectionResize", rect, &delta))
								{
									transform->set_position(transform->get_position() + delta * render_view_scale);
								}
							}

							if (aabb.intersect(mouse))
							{
								if (ImGui::IsItemClicked())
									mContext.set_selection(obj);
								dl->AddRect(
									{ aabb.min.x + cursor.x, aabb.min.y + cursor.y },
									{ aabb.max.x + cursor.x, aabb.max.y + cursor.y },
									ImGui::GetColorU32({ 1, 1, 1, 1 }), 0, 15, 3);
							}
						}
					}
				}
			}
			visual_editor::end_editor();
		}
		ImGui::End();
	}

	void show_layer_objects(const core::layer::ptr& pLayer)
	{
		for (std::size_t i = 0; i < pLayer->get_object_count(); i++)
		{
			core::game_object obj = pLayer->get_object(i);
			ImGui::PushID(obj.get_instance_id().get_value());

			const auto selection = mContext.get_selection<selection_type::game_object>();
			const bool is_selected = selection && selection->get_instance_id() == obj.get_instance_id();
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | (is_selected ? ImGuiTreeNodeFlags_Selected : 0);
			if (ImGui::TreeNodeEx((obj.get_name() + "###GameObject").c_str(), flags))
			{
				if (ImGui::IsItemClicked())
					mContext.set_selection(obj);
				ImGui::TreePop();
			}

			ImGui::PopID();
		}
	}

	void show_layers()
	{
		for (auto& i : mGame_context.get_layer_container())
		{
			ImGui::PushID(&(*i));

			bool is_selected = mContext.get_selection<selection_type::layer>() == i;
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ImGuiTreeNodeFlags_OpenOnArrow | (is_selected ? ImGuiTreeNodeFlags_Selected : 0);
			bool open = ImGui::TreeNodeEx((i->get_name() + "###Layer").c_str(), flags);
			if (ImGui::IsItemClicked())
				mContext.set_selection(i);
			if (open)
			{
				show_layer_objects(i);
				ImGui::TreePop();
			}

			ImGui::PopID();
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
						mGame_context.create_layer();
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
							auto obj = layer->create_object();
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
	
	void show_asset_inspector(core::asset::ptr pAsset)
	{
		std::string path = pAsset->get_path().string();
		ImGui::InputText("Name", &path, ImGuiInputTextFlags_ReadOnly);
		ImGui::Text(("Asset Id: " + std::to_string(pAsset->get_id())).c_str());
		ImGui::Separator();
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
			core::component* comp = pObj.get_component_index(i);
			ImGui::PushID(comp);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
			ImGui::BeginChild(ImGui::GetID("Actions"),
				ImVec2(0, (ImGui::GetStyle().WindowPadding.y * 2
					+ ImGui::GetStyle().FramePadding.y * 2
					+ ImGui::GetFontSize()) * 2), true);

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

			ImGui::EndChild();
			ImGui::PopStyleVar();

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
			//if (ImGui::Selectable("Script"))
			//	mSelected_node->add_component<script_component>();
			ImGui::EndCombo();
		}
	}

private:
	GLFWwindow* mWindow;

	context mContext;

	bool mDragging{ false };
	math::vec2 mDrag_offset;

	std::vector<std::unique_ptr<editor>> mEditors;
	
	graphics::framebuffer mViewport_framebuffer;

	core::context mGame_context;
	filesystem::path mGame_path;
	core::asset_manager mAsset_manager;

	component_inspector mInspectors;
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
