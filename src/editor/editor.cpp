#include <wge/editor/editor.hpp>
#include <wge/logging/log.hpp>

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stl.h>

#include <functional>

namespace wge::editor
{

class editor_context;

class history
{
public:
	class command
	{
	public:
		using ptr = std::shared_ptr<command>;

		virtual const char* get_name() const = 0;
		virtual void undo() = 0;
		virtual void redo() = 0;
	};

	class functional_command :
		public command
	{
	public:
		functional_command(std::string pName,
			std::function<void()> pUndo,
			std::function<void()> pRedo) :
			mName(pName), mUndo(pUndo), mRedo(pRedo)
		{}

		virtual const char* get_name() const
		{
			return mName.c_str();
		}

		virtual void undo()
		{
			mUndo();
		}

		virtual void redo()
		{
			mRedo();
		}

	private:
		std::string mName;
		std::function<void()> mUndo, mRedo;
	};

public:
	void add(const std::string pName, std::function<void()> pUndo, std::function<void()> pRedo)
	{
		auto ptr = std::make_shared<functional_command>(pName, pUndo, pRedo);
	}
};

class document
{
public:
	using ptr = std::shared_ptr<document>;

public:
	virtual ~document() {}
	virtual void save() {}

	bool is_dirty() const
	{
		return mDirty;
	}

	void mark_dirty()
	{
		mDirty = true;
	}

private:
	bool mDirty{ false };
};

// Base class for document editors
class editor
{
public:
	virtual ~editor() {}

	virtual document::ptr get_document() { return{}; }

	// Normally called before on_gui. Update any logic here.
	virtual void on_update(float pDelta) {}
	// Called when the gui needs to be drawn. Do gui stuff here.
	virtual void on_gui(float pDelta) {}
	// Called when some outside source wants to give focus to this editor
	virtual void on_request_focus() {}
	// Closes this editor
	virtual void on_close() {}
};

/*class editor_context
{

};*/

class scene_editor :
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
		mainloop();
		return 0;
	}

private:
	void mainloop()
	{
		while (!glfwWindowShouldClose(mWindow))
		{
			new_frame();
			main_viewport_dock();

			for (auto& i : mEditors)
			{
				i->on_update(1);
				i->on_gui(1);
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

private:
	GLFWwindow* mWindow;
	std::vector<std::unique_ptr<editor>> mEditors;
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

}