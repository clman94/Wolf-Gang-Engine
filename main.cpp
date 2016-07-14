#include "node.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "rpg.hpp"
#include "time.hpp"
#include "ui.hpp"

#include "editor\editor.hpp"

// http://www.grinninglizard.com/tinyxmldocs/tutorial0.html

int game()
{
	engine::renderer r;
	r.initualize(640, 512, 120); // Tiles fit nicely in this space
	r.set_pixel_scale(2); // Set the pixels to be 2x2

	engine::clock load_clock;

	// The humungous class that is the rpg game engine :D
	rpg::game game;
	game.set_renderer(r);

	// Load default location
	game.load_game("data/game.xml");

	std::cout << "Size of the freakin game: " << sizeof(game) << "\n";
	std::cout << "Size of the animated_sprite_node: " << sizeof(engine::animated_sprite_node) << "\n";
	std::cout << "Size of the sprite_node: " << sizeof(engine::sprite_node) << "\n";

	std::cout << "Load time : " << load_clock.get_elapse().s() << " seconds\n";

	// This clock will calculate own framerate
	engine::clock fpsclock;
	unsigned int frames_clocked = 0;

	r.start_text_record(true);

	bool working = true;
	while (working)
	{
		if (r.update_events())
		{
			working = false;
			break;
		}
		if (r.is_key_pressed(engine::renderer::key_type::Z) ||
			r.is_key_pressed(engine::renderer::key_type::Return))
			game.trigger_control(game.ACTIVATE);
		if (r.is_key_down(engine::renderer::key_type::Left))
			game.trigger_control(game.LEFT);
		if (r.is_key_down(engine::renderer::key_type::Right))
			game.trigger_control(game.RIGHT);
		if (r.is_key_down(engine::renderer::key_type::Up))
			game.trigger_control(game.UP);
		if (r.is_key_down(engine::renderer::key_type::Down))
			game.trigger_control(game.DOWN);
		if (r.is_key_pressed(engine::renderer::key_type::Left))
			game.trigger_control(game.SELECT_PREV);
		if (r.is_key_pressed(engine::renderer::key_type::Right))
			game.trigger_control(game.SELECT_NEXT);
		if (r.is_key_down(engine::renderer::key_type::Escape))
			working = false;

		game.tick(r);
		r.draw();

		++frames_clocked;
		if (fpsclock.get_elapse().s() >= 1)
		{
			std::cout << frames_clocked / fpsclock.get_elapse().s() << "\n";
			frames_clocked = 0;
			fpsclock.restart();
		}
	}
	return 0;
}


int main(int argc, char* argv[])
{
	//if (argc == 2)
	//{
			editor::editor_mode e;
			return e.start(game);
	//}
	return game();
}

