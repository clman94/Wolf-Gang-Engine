#include "editor.hpp"

using namespace editor;

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
editor_mode::start(std::function<int()> game)
{
	initualize();

	c_editor.reset(new tile_editor());
	c_editor->begin(r, font);

	bool working = true;
	while (working)
	{
		if (r.update_events())
		{
			working = false;
			break;
		}
		
		if (c_editor)
			c_editor->update(r);

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
	default_input_box(inp_start_scene, font, { 0, 0 });
	inp_start_scene.set_instance(instance);
	inp_start_scene.set_message("Start Scene");
	inp_start_scene.set_text("data/scene.xml");
	r.add_client(&inp_start_scene);

	default_input_box(inp_textures, font, { 0, 20 });
	inp_textures.set_instance(instance);
	inp_textures.set_message("Texture File");
	inp_textures.set_text("data/textures/textures.xml");
	r.add_client(&inp_textures);

	default_input_box(inp_dialog_sound, font, { 0, 40 });
	inp_dialog_sound.set_instance(instance);
	inp_dialog_sound.set_message("Dialog Sound");
	inp_dialog_sound.set_text("data/sound/dialog.ogg");
	r.add_client(&inp_dialog_sound);
	return 0;
}

int
game_editor::update(engine::renderer &r)
{
	return 0;
}

int
tile_editor::begin(engine::renderer &r, engine::font& font)
{
	default_input_box(inp_tile_name, font, { 0, 20 });
	inp_tile_name.set_instance(instance);
	inp_tile_name.set_message("Tile");
	inp_tile_name.set_text("None");
	r.add_client(&inp_tile_name);
	return 0;
}


int
tile_editor::update(engine::renderer &r)
{
	return 0;
}