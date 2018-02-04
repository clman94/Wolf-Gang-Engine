#include <iostream>
#include <exception>

#include <engine/renderer.hpp>
#include <engine/time.hpp>
#include <engine/logger.hpp>

#include <rpg/rpg.hpp>

#include <rpg/editor.hpp>

/*class wolf_gang_engine_editor
{
public:
	wolf_gang_engine_editor();

	int initualize(const std::string& pCustom_location);
	bool run();

private:
	void update_events();
	bool update_editor();

	engine::renderer mRenderer;
	engine::display_window mWindow;

	rpg::terminal_gui mTerminal_gui;
	engine::terminal_system mTerminal_system;


	rpg::game        mGame;
	engine::controls mControls;

	bool mRunning;
};

wolf_gang_engine_editor::wolf_gang_engine_editor()
{
	mRunning = true;

	mEditor_manager.set_world_node(mGame.get_scene().get_world_node());
	mEditor_manager.set_resource_manager(mGame.get_resource_manager());
	mEditor_manager.set_scene(mGame.get_scene());

	mGame.load_terminal_interface(mTerminal_system);
	mTerminal_gui.set_terminal_system(mTerminal_system);
	mEditor_manager.load_terminal_interface(mTerminal_system);
}

int wolf_gang_engine_editor::initualize(const std::string& pCustom_location = std::string())
{
	engine::clock load_clock;

	logger::info("Loading renderer...");
	logger::start_sub_routine();

	mWindow.initualize("Game", rpg::defs::SCREEN_SIZE);
	mRenderer.set_target_size(rpg::defs::DISPLAY_SIZE);
	mRenderer.set_window(mWindow);
	mGame.set_renderer(mRenderer);
	mTerminal_gui.load_gui(mRenderer);
	mEditor_manager.set_renderer(mRenderer);
	logger::info("Renderer loaded");


#ifndef LOCKED_RELEASE_MODE
	if (pCustom_location.empty())
		mGame.load("./data");
	else
		mGame.load(pCustom_location);
#else
	if (!mGame.load_settings("./data.pack"))
	{
		logger::error("Failed to load settings");
	}
#endif
	float load_time = load_clock.get_elapse().seconds();

	logger::end_sub_routine();

	logger::info("Load time : " + std::to_string(load_time) + " seconds");
	return 0;
}

bool wolf_gang_engine_editor::run()
{
	while (mRunning)
	{
		update_events();

		if (mRenderer.is_key_pressed(engine::renderer::key_type::F11))
		{
			mWindow.toggle_mode();
			mRenderer.refresh();
		}

		mTerminal_gui.update(mRenderer);
		update_editor();

		if (mGame.tick())
			return true;

		mRenderer.draw();
	}
	return false;
}

void wolf_gang_engine_editor::update_events()
{
	if (mWindow.poll_events())
	{
		logger::info("Closing with window event");
		mRunning = false;
		return;
	}

	mRenderer.update_events();

	if (mRenderer.is_key_down(engine::renderer::key_type::Escape))
	{
		logger::info("Closing with 'Escape'");
		mRunning = false;
	}
}

bool wolf_gang_engine_editor::update_editor()
{
	const bool lshift = mRenderer.is_key_down(engine::renderer::key_type::LShift);
	const bool lctrl = mRenderer.is_key_down(engine::renderer::key_type::LControl);

	if (lctrl
		&& lshift
		&& mRenderer.is_key_pressed(engine::renderer::key_type::R))
	{
		mEditor_manager.close_editor();
		mGame.restart_game();
	}

	if (lctrl
		&& !lshift)
	{
		if (mRenderer.is_key_pressed(engine::renderer::key_type::R))
		{
			mEditor_manager.close_editor();
			logger::info("Reloading scene...");

			mGame.get_resource_manager().reload_directories();
			mGame.get_scene().reload_scene();
			logger::info("Scene reloaded");
		}	
		if (mRenderer.is_key_pressed(engine::renderer::key_type::Num1))
		{
			mEditor_manager.close_editor();
			mEditor_manager.open_tilemap_editor((mGame.get_source_path() / rpg::defs::DEFAULT_SCENES_PATH / mGame.get_scene().get_path()).string());
			mGame.clear_scene();
		}

		if (mRenderer.is_key_pressed(engine::renderer::key_type::Num2))
		{
			mEditor_manager.close_editor();
			mEditor_manager.open_collisionbox_editor((mGame.get_source_path() / rpg::defs::DEFAULT_SCENES_PATH / mGame.get_scene().get_path()).string());
			mGame.clear_scene();
		}
		if (mRenderer.is_key_pressed(engine::renderer::key_type::Num3))
		{
			mEditor_manager.close_editor();
			mEditor_manager.open_atlas_editor();
			mGame.clear_scene();
		}
	}
	return false;
}*/

// Entry point of application
int main(int argc, char* argv[])
{
	logger::initialize("./log.txt");
	try
	{
		editors::WGE_editor editor;

		editor.initualize("./data");
		editor.run();
	}
	catch (std::exception& e)
	{
		logger::error("A main exception occurred: " + std::string(e.what()) + "\n");
		std::getchar();
	}
	
	return 0;
}