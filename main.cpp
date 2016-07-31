#include "node.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "rpg.hpp"
#include "time.hpp"
#include "ui.hpp"
#include "rpg_config.hpp"
#include "editor\editor.hpp"
#include "parsers.hpp"

#include "particle_engine.hpp"

// http://www.grinninglizard.com/tinyxmldocs/tutorial0.html

int game()
{
	
	engine::renderer r;
	r.initualize(rpg::defs::SCREEN_SIZE, 120); // Tiles fit nicely in this space
	r.set_pixel_scale(2); // Set the pixels to be 2x2
	
	engine::clock load_clock;

	// The humungous class that is the rpg game engine :D
	rpg::game game;
	game.set_renderer(r);

	// Load default location
	game.load_game("data/game.xml");

	std::cout << "Size of the freakin game: " << sizeof(game) << "\n";
	std::cout << "Size of the sprite_node: " << sizeof(engine::sprite_node) << "\n";

	std::cout << "Load time : " << load_clock.get_elapse().s() << " seconds\n";

	// This clock will calculate the framerate
	engine::clock fpsclock;
	unsigned int frames_clocked = 0;

	// TEST particle system

	/*engine::particle_system p1;
	p1.set_region({ 10,10 });
	p1.set_life(4);
	p1.set_acceleration({ 0, 5 });
	p1.set_depth(-100);
	p1.set_velocity({ 0.2f,0.2f });
	p1.set_rate(1);
	p1.set_texture(*game.get_texture_manager().get_texture("somedude1"));
	p1.set_texture_rect({ 0,0,32,32 });

	r.add_client(&p1);*/

	r.start_text_record(true);

	// TEST parser

	{
		auto v = parsers::parse_vector<int>("2, 5");
		std::cout << v.x << " " << v.y << "\n";
	}

	rpg::controls controls;

	bool working = true;
	while (working)
	{
		if (r.update_events())
		{
			working = false;
			break;
		}
		if (r.is_key_pressed(engine::events::key_type::Z) ||
			r.is_key_pressed(engine::events::key_type::Return))
			controls.trigger(rpg::controls::control::activate);

		if (r.is_key_down(engine::events::key_type::Left))
			controls.trigger(rpg::controls::control::left);

		if (r.is_key_down(engine::events::key_type::Right))
			controls.trigger(rpg::controls::control::right);

		if (r.is_key_down(engine::events::key_type::Up))
			controls.trigger(rpg::controls::control::up);

		if (r.is_key_down(engine::events::key_type::Down))
			controls.trigger(rpg::controls::control::down);

		if (r.is_key_pressed(engine::events::key_type::Left))
			controls.trigger(rpg::controls::control::select_previous);

		if (r.is_key_pressed(engine::events::key_type::Right))
			controls.trigger(rpg::controls::control::select_next);

		if (r.is_key_down(engine::events::key_type::Escape))
			working = false;

		game.tick(controls);
		r.draw();

		++frames_clocked;
		if (fpsclock.get_elapse().s() >= 1)
		{
			std::cout << frames_clocked / fpsclock.get_elapse().s() << "\n";
			frames_clocked = 0;
			fpsclock.restart();
		}
		controls.reset();
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc == 2)
	{
			editors::editor_mode e;
			return e.start(game);
	}
	return game();
}
