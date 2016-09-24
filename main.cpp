#include "node.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "rpg.hpp"
#include "time.hpp"
#include "rpg_config.hpp"
#include "parsers.hpp"

#include "particle_engine.hpp"

// http://www.grinninglizard.com/tinyxmldocs/tutorial0.html

int game()
{
	engine::renderer r;
	r.initualize(rpg::defs::SCREEN_SIZE);
	r.set_pixel_scale(3);
	
	engine::clock load_clock;

	// The humungous class that is the rpg game engine :D
	rpg::game game;
	game.set_renderer(r);

	// Load default location
	game.load_game_xml("data/game.xml");

	std::cout << "Size of the freakin game: " << sizeof(game) << "\n";
	std::cout << "Size of the sprite_node: " << sizeof(engine::sprite_node) << "\n";

	std::cout << "Load time : " << load_clock.get_elapse().s() << " seconds\n";

	// TEST particle system

	/*engine::particle_emitter p1;
	p1.set_region({ 10,10 });
	p1.set_life(4);
	p1.set_acceleration({ 0, 5 });
	p1.set_depth(-100);
	p1.set_velocity({ 0.2f,0.2f });
	p1.set_rate(1);
	p1.set_texture(*game.get_texture_manager().get_texture("somedude1"));
	p1.set_texture_rect({ 0,0,32,32 });

	r.add_client(&p1);*/

	rpg::controls controls;

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
			controls.trigger(rpg::controls::control::activate);

		if (r.is_key_down(engine::renderer::key_type::Left))
			controls.trigger(rpg::controls::control::left);

		if (r.is_key_down(engine::renderer::key_type::Right))
			controls.trigger(rpg::controls::control::right);

		if (r.is_key_down(engine::renderer::key_type::Up))
			controls.trigger(rpg::controls::control::up);

		if (r.is_key_down(engine::renderer::key_type::Down))
			controls.trigger(rpg::controls::control::down);

		if (r.is_key_pressed(engine::renderer::key_type::Left))
			controls.trigger(rpg::controls::control::select_previous);

		if (r.is_key_pressed(engine::renderer::key_type::Right))
			controls.trigger(rpg::controls::control::select_next);

		if (r.is_key_down(engine::renderer::key_type::LControl))
		{
			if (r.is_key_pressed(engine::renderer::key_type::R))
				controls.trigger(rpg::controls::control::reset);
			if (r.is_key_pressed(engine::renderer::key_type::Num1))
				controls.trigger(rpg::controls::control::editor_1);
			if (r.is_key_pressed(engine::renderer::key_type::Num2))
				controls.trigger(rpg::controls::control::editor_2);
		}

		if (r.is_key_down(engine::renderer::key_type::Escape))
			working = false;

		if (r.is_key_pressed(engine::renderer::key_type::T))
			std::cout << "FPS: " << r.get_fps() << "\n";

		game.tick(controls);
		r.draw();

		controls.reset();
	}
	return 0;
}

int main(int argc, char* argv[])
{

	/*rpg::tilemap_loader tl;
	tl.load_tilemap("testmap.txt");
	tl.condense_tiles();
	tl.generate_tilemap("testmapnew.txt");*/

	return game();
}

