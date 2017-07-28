#include <iostream>
#include <exception>

#include <engine/renderer.hpp>
#include <engine/time.hpp>
#include <engine/log.hpp>

#include <rpg/rpg.hpp>


class wolf_gang_engine
{
public:
	wolf_gang_engine();

	int initualize(const std::string& pCustom_location);
	bool run();

private:
	void update_events();
	bool update_editor();

	engine::renderer mRenderer;
	engine::display_window mWindow;

	rpg::terminal_gui mTerminal_gui;
	engine::terminal_system mTerminal_system;

	editors::editor_manager mEditor_manager;

	rpg::game        mGame;
	engine::controls mControls;

	bool mRunning;
};

wolf_gang_engine::wolf_gang_engine()
{
	mRunning = true;
}

int wolf_gang_engine::initualize(const std::string& pCustom_location = std::string())
{
	engine::clock load_clock;

	logger::info("Loading renderer...");
	logger::start_sub_routine();

	mWindow.initualize("", rpg::defs::SCREEN_SIZE);
	mRenderer.set_target_size(rpg::defs::DISPLAY_SIZE);
	mRenderer.set_window(mWindow);
	logger::info("Renderer loaded");

	// Setup game
	mGame.set_renderer(mRenderer);
	mGame.load_terminal_interface(mTerminal_system);

	// Setup editors
	mEditor_manager.set_renderer(mRenderer);
	mEditor_manager.set_world_node(mGame.get_scene().get_world_node());
	mEditor_manager.set_scene(mGame.get_scene());
	mEditor_manager.load_terminal_interface(mTerminal_system);

	mTerminal_gui.set_terminal_system(mTerminal_system);
	mTerminal_gui.load_gui(mRenderer);

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

bool wolf_gang_engine::run()
{
	while (mRunning)
	{
		update_events();

		mTerminal_gui.update(mRenderer);
		update_editor();

		if (mGame.tick())
			return true;

		mRenderer.draw();
	}
	return false;
}

void wolf_gang_engine::update_events()
{
	if (mRenderer.update_events())
	{
		logger::info("Closing with window event");
		mRunning = false;
		return;
	}

	if (mRenderer.is_key_down(engine::renderer::key_type::Escape))
	{
		logger::info("Closing with 'Escape'");
		mRunning = false;
	}
}

bool wolf_gang_engine::update_editor()
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
		&& !lshift
		&& mRenderer.is_key_pressed(engine::renderer::key_type::R))
	{
		mEditor_manager.close_editor();
		logger::info("Reloading scene...");

		mGame.get_resource_manager().reload_directories();
		mGame.get_scene().reload_scene();
		logger::info("Scene reloaded");
	}


	if (lctrl
		&& mRenderer.is_key_pressed(engine::renderer::key_type::Num1))
	{
		mEditor_manager.close_editor();
		mEditor_manager.open_tilemap_editor((mGame.get_source_path() / rpg::defs::DEFAULT_SCENES_PATH / mGame.get_scene().get_path()).string());
		mGame.clear_scene();
	}

	if (lctrl
		&& mRenderer.is_key_pressed(engine::renderer::key_type::Num2))
	{
		mEditor_manager.close_editor();
		mEditor_manager.open_collisionbox_editor((mGame.get_source_path() / rpg::defs::DEFAULT_SCENES_PATH / mGame.get_scene().get_path()).string());
		mGame.clear_scene();
	}
	if (lctrl
		&& mRenderer.is_key_pressed(engine::renderer::key_type::Num3))
	{
		mEditor_manager.close_editor();
		mEditor_manager.open_atlas_editor();
		mGame.clear_scene();
	}
	return false;
}

// Entry point of application
int main(int argc, char* argv[])
{
	logger::initialize("./log.txt");
	try
	{
		wolf_gang_engine wge;

		if (argc > 2)
			wge.initualize(argv[1]);
		else
			wge.initualize();
		if (wge.run())
			return 0;
	}
	catch (std::exception& e)
	{
		logger::error("A main exception occurred: " + std::string(e.what()) + "\n");
		std::getchar();
	}
	
	return 0;
}