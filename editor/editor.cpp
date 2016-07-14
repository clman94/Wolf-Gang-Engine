#include "editor.hpp"

using namespace editor;

// ###########
// editor_mode
// ###########

int
editor_mode::initualize_renderer()
{
	r.initualize(640, 512, 30);
	r.set_pixel_scale(2);
	if (!font.load("data/font.ttf"))
		std::cout << "nooo";
	return 0;
}

int
editor_mode::start(std::function<int()> game)
{
	initualize_renderer();

	menus.reset(new game_editor());
	menus->begin(r, font);

	bool working = true;
	while (working)
	{
		if (r.update_events())
		{
			working = false;
			break;
		}

		if (menus)
			menus->update(r);

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

// ###########
// game_editor
// ###########

int
game_editor::begin(engine::renderer &r, engine::font& font)
{
	b1.set_instance(instance);
	b1.set_size({ 30, 50 });
	b1.set_depth(-1000);
	r.add_client(&b1);

	b2.set_instance(instance);
	b2.set_position({ 0, 100 });
	b2.set_enable(false);
	b2.set_size({ 30, 50 });
	b2.set_depth(-1000);
	r.add_client(&b2);

	auto& text_node = tb1.get_text_node();
	text_node.set_font(font);
	default_text_format(text_node);

	tb1.set_instance(instance);
	tb1.set_size({ 100, 20 });
	tb1.set_position({ 100, 13 });
	tb1.set_message("Thing");
	tb1.set_text(100.32f);
	r.add_client(&tb1);
	return 0;
}

int
game_editor::update(engine::renderer &r)
{
	return 0;
}


