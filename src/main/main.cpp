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
	mRenderer.set_pixel_scale(3);

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
		mGame.tick(mControls);
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

	using key_type = engine::renderer::key_type;
	using control = rpg::controls::control;

	mControls.reset();

	if (mRenderer.is_key_pressed(key_type::Z) ||
		mRenderer.is_key_pressed(key_type::Return))
		mControls.trigger(control::activate);

	if (mRenderer.is_key_down(key_type::Left))
		mControls.trigger(control::left);

	if (mRenderer.is_key_down(key_type::Right))
		mControls.trigger(control::right);

	if (mRenderer.is_key_down(key_type::Up))
		mControls.trigger(control::up);

	if (mRenderer.is_key_down(key_type::Down))
		mControls.trigger(control::down);

	if (mRenderer.is_key_pressed(key_type::Left))
		mControls.trigger(control::select_previous);

	if (mRenderer.is_key_pressed(key_type::Right))
		mControls.trigger(control::select_next);

	if (mRenderer.is_key_down(key_type::LControl))
	{
		if (mRenderer.is_key_pressed(key_type::R))
			mControls.trigger(control::reset);
		if (mRenderer.is_key_pressed(key_type::Num1))
			mControls.trigger(control::editor_1);
		if (mRenderer.is_key_pressed(key_type::Num2))
			mControls.trigger(control::editor_2);
	}

	if (mRenderer.is_key_down(key_type::Escape))
		mRunning = false;

	if (mRenderer.is_key_pressed(key_type::T))
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