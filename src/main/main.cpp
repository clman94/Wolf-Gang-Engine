#include <iostream>
#include <exception>

#include <engine/renderer.hpp>
#include <engine/time.hpp>

#include <rpg/rpg.hpp>



class wolf_gang_engine
{
public:
	wolf_gang_engine();

	int initualize();
	int run();

private:
	void update_events();

	engine::renderer mRenderer;

	rpg::game        mGame;
	rpg::controls    mControls;

	bool mRunning;
};


wolf_gang_engine::wolf_gang_engine()
{
	mRunning = true;
}

int wolf_gang_engine::initualize()
{
	engine::clock load_clock;

	mRenderer.initualize(rpg::defs::SCREEN_SIZE);
	mRenderer.set_pixel_scale(2);

	mGame.set_renderer(mRenderer);
	mGame.load_game_xml("data/game.xml");
	
	float load_time = load_clock.get_elapse().s();
	std::cout << "Load time : " << load_time << " seconds\n";
	return 0;
}

int wolf_gang_engine::run()
{
	while (mRunning)
	{
		update_events();
		mGame.tick();
		mRenderer.draw();
	}
	return 0;
}

void wolf_gang_engine::update_events()
{
	if (mRenderer.update_events())
	{
		mRunning = false;
		return;
	}

	if (mRenderer.is_key_down(engine::renderer::key_type::Escape))
		mRunning = false;

	if (mRenderer.is_key_pressed(engine::renderer::key_type::T))
		std::cout << "FPS: " << mRenderer.get_fps() << "\n";
}

// Entry point of application
int main(int argc, char* argv[])
{
	try
	{
		wolf_gang_engine wge;
		wge.initualize();
		wge.run();
	}
	catch (std::exception& e)
	{
		std::cout << "A main exception occurred: " << e.what() << "\n";
		std::getchar();
	}

	return 0;
}