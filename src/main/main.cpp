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

	engine::renderer mRenderer;
	engine::display_window mWindow;

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

#ifndef LOCKED_RELEASE_MODE
	mWindow.initualize("A Friendly Venture", rpg::defs::SCREEN_SIZE);
#else
	mWindow.initualize("A Friendly Venture", rpg::defs::SCREEN_SIZE);
#endif
	mRenderer.set_target_size(rpg::defs::DISPLAY_SIZE);
	mRenderer.set_window(mWindow);
	logger::info("Renderer loaded");

	mGame.set_renderer(mRenderer);
	
#ifndef LOCKED_RELEASE_MODE
	if (pCustom_location.empty())
		mGame.load_settings("./data");
	else
		mGame.load_settings(pCustom_location);
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

// Entry point of application
int main(int argc, char* argv[])
{
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