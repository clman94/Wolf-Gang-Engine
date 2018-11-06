#include <wge/editor/application.hpp>

#include <wge/logging/log.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/core/context.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world_component.hpp>
#include <wge/graphics/sprite_component.hpp>
#include <wge/scripting/script.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/graphics/texture_asset_loader.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/exception.hpp>

#include "editor.hpp"
#include "history.hpp"
#include "context.hpp"
#include "component_inspector.hpp"

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stl.h>

#include <functional>

namespace ImGui
{

// Draws a framebuffer as a regular image
inline void Image(const wge::graphics::framebuffer& mFramebuffer, const ImVec2& pSize = ImVec2(0, 0))
{
	ImGui::Image((void*)mFramebuffer.get_gl_texture(), pSize,
		ImVec2(0, 1), ImVec2(1, 0)); // Y-axis needs to be flipped
}

}

namespace wge::editor
{

class scene_editor :
	public editor
{
public:
	scene_editor(editor_context& pContext)
	{
		mContext = &pContext;
	}

	virtual void on_gui(float pDelta) override
	{
		mContext->renderer.set_framebuffer(&mFramebuffer);
		mContext->renderer.render();
	}

private:
	graphics::framebuffer mFramebuffer;
	editor_context* mContext;
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
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
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

inline void show_node_tree(util::ref<core::object_node> pNode, util::ref<core::object_node>& pSelected, bool pIs_collection = false)
{
	ImGui::PushID(pNode.get());

	bool* open = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("_IsOpen"));

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	// Don't show the arrow if there are no children nodes
	if (pNode->get_child_count() > 0)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		collapsing_arrow("NodeUnfold", open);
		ImGui::PopStyleVar();
		ImGui::SameLine();
	}

	std::string label;
	if (pIs_collection) label = "Collection - " + pNode->get_name();
	else label = pNode->get_name();
	if (ImGui::Selectable(label.c_str(), pSelected == pNode))
		pSelected = pNode;


	if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
		*open = !*open; // Toggle open flag
	if (!pIs_collection && ImGui::BeginDragDropSource())
	{
		core::object_node* ptr = pNode.get();
		ImGui::SetDragDropPayload("MoveNodeInTree", &ptr, sizeof(void*));

		ImGui::Text(pNode->get_name().c_str());

		ImGui::EndDragDropSource();
	}

	// Drop node as child of this node. Inserted at end.
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MoveNodeInTree"))
		{
			util::ref<core::object_node> node = *static_cast<core::object_node**>(payload->Data);
			if (!pNode->is_child_of(node)) // Do not move parent into its own child!
				pNode->add_child(node);
		}
		ImGui::EndDragDropTarget();
	}

	// Drop node as first child of this node or as previous node if this node is collapsed
	ImGui::InvisibleButton("__DragBetween", ImVec2(-1, 3));
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MoveNodeInTree"))
		{
			util::ref<core::object_node> node = *static_cast<core::object_node**>(payload->Data);
			if (!pNode->is_child_of(node)) // Do not move parent into its own child!
			{
				if (pNode->get_child_count() && *open)
					pNode->add_child(node, 0);
				else if (auto parent = pNode->get_parent())
					parent->add_child(node, parent->get_child_index(pNode));
			}
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);

	if (*open)
	{
		ImGui::TreePush();
		// Show the children nodes and their components
		for (std::size_t i = 0; i < pNode->get_child_count(); i++)
			show_node_tree(pNode->get_child(i), pSelected);
		ImGui::TreePop();

		// Drop node as next node
		if (pNode->get_child_count() > 0 && *open)
		{
			ImGui::InvisibleButton("__DragAfterChildrenInParent", ImVec2(-1, 3));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MoveNodeInTree"))
				{
					util::ref<core::object_node> node = *static_cast<core::object_node**>(payload->Data);
					if (!pNode->is_child_of(node)) // Do not move parent into its own child!
						if (auto parent = pNode->get_parent())
							parent->add_child(node, parent->get_child_index(pNode) + 1);
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
		}
	}

	ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

	ImGui::PopID();
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
	int run()
	{
		init_glfw();
		init_imgui();
		mContext.init();
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

			show_asset_manager();
			show_viewport();
			show_objects();
			show_component_inspector();

			for (auto i : mContext.game_context.get_collection_container())
				i->send_down("on_render", &mContext.renderer);

			mContext.renderer.set_framebuffer(&mViewport_framebuffer);
			mContext.renderer.render();
			mContext.renderer.clear();

			end_frame();
		}
		shutdown();
	}

	void shutdown()
	{
		mContext.renderer.clear();

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

			for (auto& i : mContext.asset_manager.get_asset_list())
			{
				ImGui::PushID(i->get_id());
				ImGui::Selectable(i->get_type().c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
				if (ImGui::BeginDragDropSource())
				{
					core::asset_uid id = i->get_id();
					ImGui::SetDragDropPayload((i->get_type() + "Asset").c_str(), &id, sizeof(core::asset_uid));
					ImGui::Text("Asset");
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

			ImGui::Image(mViewport_framebuffer, ImVec2(width, height));
		}
		ImGui::End();
	}

	void show_objects()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, ImGui::GetStyle().WindowPadding.y));
		if (ImGui::Begin("Objects", NULL, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Scene"))
				{
					if (ImGui::MenuItem("Load"))
					{
						if (auto scene = mContext.asset_manager.find_asset("myscene.asset"))
						{
							mContext.game_context.deserialize(scene->get_config()->get_metadata());
						}
						else
						{
							log::error() << "myscene asset missing" << log::endm;
						}
					}
					if (ImGui::MenuItem("Save"))
					{
						try
						{
							if (auto scene = mContext.asset_manager.find_asset("myscene.asset"))
							{
								scene->get_config()->set_metadata(mContext.game_context.serialize());
								scene->get_config()->save();
							}
							else
							{
								auto config = std::make_shared<core::asset_config>();
								config->set_metadata(mContext.game_context.serialize());
								config->set_path(mContext.asset_manager.get_root_directory() / "myscene.asset");
								config->set_type("scene");
								config->generate_id();

								config->save();

								core::asset_loader* loader = mContext.asset_manager.find_loader("scene");
								mContext.asset_manager.add_asset(loader->create_asset(config, mContext.asset_manager.get_root_directory()));
							}
						}
						catch (const std::exception& e)
						{
							log::error() << e.what() << log::endm;
							log::error() << "Could not save scene" << log::endm;
						}
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Add"))
				{
					if (ImGui::MenuItem("Collection"))
					{
						mSelected_node = mContext.game_context.create_collection();
					}
					if (ImGui::MenuItem("Object 2D"))
					{
						auto node = core::object_node::create(&mContext.game_context);
						node->set_name("New 2D Object");
						node->add_component<core::transform_component>();
						if (mSelected_node)
							mSelected_node->add_child(node);
						else
							log::error() << "Could not create object" << log::endm;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			for (auto& i : mContext.game_context.get_collection_container())
				show_node_tree(i, mSelected_node, true);
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void show_component_inspector()
	{
		ImGui::Begin("Component Inspector");
		if (mSelected_node)
		{
			std::string name = mSelected_node->get_name();
			if (ImGui::InputText("Name", &name))
				mSelected_node->set_name(name);

			for (std::size_t i = 0; i < mSelected_node->get_component_count(); i++)
			{
				// If set to true, the component will be deleted at the end of this loop
				bool delete_component = false;

				core::component* comp = mSelected_node->get_component_index(i);
				ImGui::PushID(comp);

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
				ImGui::BeginChild(ImGui::GetID("Actions"),
					ImVec2(0, (ImGui::GetStyle().WindowPadding.y * 2
						+ ImGui::GetStyle().FramePadding.y * 2
						+ ImGui::GetFontSize()) * 2), true);

				bool open = collapsing_arrow("CollapsingArrow");

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
				delete_component = ImGui::Button("Delete");

				ImGui::EndChild();
				ImGui::PopStyleVar();

				if (open)
					mContext.inspectors.on_gui(comp);

				ImGui::PopID();
				if (delete_component)
					mSelected_node->remove_component(i--);
			}

			ImGui::Separator();
			if (ImGui::BeginCombo("###Add Component", "Add Component"))
			{
				if (ImGui::Selectable("Transform 2D"))
					mSelected_node->add_component<core::transform_component>();
				if (ImGui::Selectable("Physics World"))
					mSelected_node->add_component<physics::physics_world_component>();
				if (ImGui::Selectable("Physics"))
					mSelected_node->add_component<physics::physics_component>();
				if (ImGui::Selectable("Box Collider"))
					mSelected_node->add_component<physics::box_collider_component>();
				if (ImGui::Selectable("Sprite"))
					mSelected_node->add_component<graphics::sprite_component>();
				//if (ImGui::Selectable("Script"))
				//	mSelected_node->add_component<script_component>();
				ImGui::EndCombo();
			}
		}
		ImGui::End();

	}

private:
	GLFWwindow* mWindow;
	core::object_node::ref mSelected_node;
	std::vector<std::unique_ptr<editor>> mEditors;
	editor_context mContext;
	graphics::framebuffer mViewport_framebuffer;
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
