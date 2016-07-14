#include "node.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "rpg.hpp"
#include "time.hpp"
#include "ui.hpp"

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

class editor_mode
{
	std::string path;
	engine::renderer r;
	
	engine::ui::ui_instance instance;
	
	int initualize_renderer()
	{
		r.initualize(640, 512, 20);
		r.set_pixel_scale(2);
		return 0;
	}

public:
	void set_path(std::string str)
	{
		path = str;
	}
	int start()
	{
		initualize_renderer();

		engine::ui::button_simple b1;
		b1.set_instance(instance);
		b1.set_size({ 30, 50 });
		b1.set_depth(-1000);
		r.add_client(&b1);

		engine::ui::button_simple b2;
		b2.set_instance(instance);
		b2.set_position({ 0, 100 });
		b2.set_enable(false);
		b2.set_size({ 30, 50 });
		b2.set_depth(-1000);
		r.add_client(&b2);

		engine::font font;
		if (!font.load("data/font.ttf"))
			std::cout << "nooo";

		engine::ui::input_box tb1;
		auto& text_node = tb1.get_text_node();
		text_node.set_font(font);
		text_node.set_color({ 255, 255, 255 });
		text_node.set_scale(0.5f);
		tb1.set_instance(instance);
		tb1.set_size({ 100, 20 });
		tb1.set_position({ 100, 13 });
		tb1.set_message("Thing");
		tb1.set_text(100.32f);
		r.add_client(&tb1);

		bool working = true;
		while (working)
		{
			if (r.update_events())
			{
				working = false;
				break;
			}
			
			if ((r.is_key_down(engine::renderer::key_type::LControl) ||
				r.is_key_down(engine::renderer::key_type::RControl)) &&
				r.is_key_pressed(engine::renderer::key_type::P))
			{
				std::cout << "########## Starting game\n";
				r.set_visible(false);
				int status = game();

				std::cout << "########## Returning to editor\n"
					      << "Game exited with errorcode: " << status << "\n";
				r.set_visible(true);
			}

			r.draw();
		}
		return 0;
	}
};

int main(int argc, char* argv[])
{
	if (argc == 2)
	{
			editor_mode e;
			return e.start();
	}
	return game();
}

