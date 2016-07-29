#include "editor.hpp"

using namespace editors;

// ###########
// editor_mode
// ###########

int
editor_mode::initualize()
{
	r.initualize({ 640, 512 }, 30);
	r.set_pixel_scale(2);
	r.set_bg_color({ 255, 204, 153 });
	if (!font.load("editorfont.ttf"))
		std::cout << "nooo";
	return 0;
}

int
editor_mode::start_game(std::function<int()> game)
{
	assert(game != nullptr);

	std::cout << "########## Starting game\n";
	r.set_visible(false);

	int status = game();

	std::cout << "########## Returning to editor\n"
		<< "Game exited with errorcode: " << status << "\n";
	r.set_visible(true);

	return status;
}

int
editor_mode::start(std::function<int()> game)
{
	assert(game != nullptr);

	initualize();

	c_editor.reset(new tile_editor());
	c_editor->set_renderer(r);
	c_editor->begin(font);

	bool working = true;
	while (working)
	{
		if (r.update_events())
		{
			working = false;
			break;
		}
		
		if (c_editor)
			c_editor->update();

		if ((r.is_key_down(engine::renderer::key_type::LControl) ||
			r.is_key_down(engine::renderer::key_type::RControl)) &&
			r.is_key_pressed(engine::renderer::key_type::P))
		{
			start_game(game);
		}
		
		r.draw();
	}
	return 0;
}

// ###########
// game_editor
// ###########

int
game_editor::begin(engine::font& font)
{
	default_input_box(inp_start_scene, font, { 0, 0 });
	inp_start_scene.set_instance(instance);
	inp_start_scene.set_message("Start Scene");
	inp_start_scene.set_text("data/scene.xml");
	get_renderer()->add_client(&inp_start_scene);

	default_input_box(inp_textures, font, { 0, 20 });
	inp_textures.set_instance(instance);
	inp_textures.set_message("Texture File");
	inp_textures.set_text("data/textures/textures.xml");
	get_renderer()->add_client(&inp_textures);

	default_input_box(inp_dialog_sound, font, { 0, 40 });
	inp_dialog_sound.set_instance(instance);
	inp_dialog_sound.set_message("Dialog Sound");
	inp_dialog_sound.set_text("data/sound/dialog.ogg");
	get_renderer()->add_client(&inp_dialog_sound);
	return 0;
}

int
game_editor::update()
{
	return 0;
}

int
tile_editor::begin(engine::font& font)
{
	default_input_box(inp_tile_name, font, { 0, 20 });
	inp_tile_name.set_instance(instance);
	inp_tile_name.set_message("Tile");
	inp_tile_name.set_text("None");
	return 0;
}

int
tile_editor::update()
{
	return 0;
}

void
tile_editor::refresh_renderer(engine::renderer& r)
{
	r.add_client(&inp_tile_name);
	r.add_client(&sprite_preview);
	r.add_client(&tilemap_preview);
}

int
editor::default_text_format(engine::text_node & text_node)
{
	text_node.set_color({ 51, 26, 0 });
	text_node.set_character_size(20);
	text_node.set_scale(0.5f);
	return 0;
}

void
editor::default_input_box(engine::ui::input_box & ib, engine::font & font, engine::fvector pos)
{
	auto& text_node = ib.get_text_node();
	text_node.set_font(font);
	default_text_format(text_node);
	ib.set_size({ 100, 20 });
	ib.set_position(pos);
}
