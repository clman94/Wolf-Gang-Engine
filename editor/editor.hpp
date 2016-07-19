#include "../node.hpp"
#include "../renderer.hpp"
#include "../texture.hpp"
#include "../rpg.hpp"
#include "../time.hpp"
#include "../ui.hpp"
#include <memory>
#include <functional>

namespace editor
{

static int default_text_format(engine::text_node& text_node)
{
	text_node.set_color({ 51, 26, 0 });
	text_node.set_character_size(20);
	text_node.set_scale(0.5f);
	return 0;
}
static void default_input_box(engine::ui::input_box& ib, engine::font& font, engine::fvector pos)
{
	auto& text_node = ib.get_text_node();
	text_node.set_font(font);
	default_text_format(text_node);
	ib.set_size({ 100, 20 });
	ib.set_position(pos);
}

class editor
{
public:
	virtual int begin(engine::renderer &r, engine::font& font) = 0;
	virtual int update(engine::renderer &r) = 0;
protected:
	engine::ui::ui_instance instance;
};

class game_editor
	: public editor
{
	engine::ui::input_box inp_start_scene;
	engine::ui::input_box inp_textures;
	engine::ui::input_box inp_dialog_sound;
public:
	int begin(engine::renderer &r, engine::font& font);
	int update(engine::renderer &r);
};

class tile_editor
	: public editor
{
	engine::ui::input_box inp_tile_name;
public:
	int begin(engine::renderer &r, engine::font& font);
	int update(engine::renderer &r);
};

class editor_mode
{
	engine::renderer r;
	std::unique_ptr<editor> c_editor;
	engine::font font;
	int initualize();
public:
	int start(std::function<int()> game);
};

}
