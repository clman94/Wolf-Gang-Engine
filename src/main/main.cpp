#include <iostream>
#include <exception>

#include <engine/renderer.hpp>
#include <engine/time.hpp>

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

	util::info("Loading renderer...");
#ifndef LOCKED_RELEASE_MODE
	mRenderer.initualize(rpg::defs::SCREEN_SIZE);
#else
	mRenderer.initualize(rpg::defs::SCREEN_SIZE, 60);
#endif
	mRenderer.set_target_size(rpg::defs::DISPLAY_SIZE);
	util::info("Renderer loaded");

	mGame.set_renderer(mRenderer);
	
#ifndef LOCKED_RELEASE_MODE
	if (pCustom_location.empty())
		mGame.load_settings("./data");
	else
		mGame.load_settings(pCustom_location);
#else
	if (!mGame.load_settings("./data.pack"))
	{
		util::error("Failed to load settings");
	}
#endif
	float load_time = load_clock.get_elapse().s();
	util::info("Load time : " + std::to_string(load_time) + " seconds");
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
		util::info("Closing with window event");
		mRunning = false;
		return;
	}

	if (mRenderer.is_key_down(engine::renderer::key_type::Escape))
	{
		util::info("Closing with 'Escape'");
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
		util::error("A main exception occurred: " + std::string(e.what()) + "\n");
		std::getchar();
	}
	
	return 0;
}