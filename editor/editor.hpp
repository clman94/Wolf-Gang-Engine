#include "../node.hpp"
#include "../renderer.hpp"
#include "../texture.hpp"
#include "../rpg.hpp"
#include "../time.hpp"
#include "../ui.hpp"
#include <memory>
#include <functional>

namespace editors
{

class editor :
	public engine::render_proxy
{
public:
	virtual int begin(engine::font& font) = 0;
	virtual int update() = 0;
protected:
	int default_text_format(engine::text_node& text_node);
	void default_input_box(engine::ui::input_box& ib, engine::font& font, engine::fvector pos);

	engine::ui::ui_instance instance;
};

class game_editor :
	public editor
{
	engine::ui::input_box inp_start_scene;
	engine::ui::input_box inp_textures;
	engine::ui::input_box inp_dialog_sound;
public:
	int begin(engine::font& font);
	int update();
};


class tile_editor :
	public editor
{
	engine::ui::input_box inp_tile_name;
	engine::sprite_node sprite_preview;
	engine::tile_node tilemap_preview;
	
	struct tile
	{
		std::string name;
		engine::ivector pos;
	};
	std::vector<tile> tiles;
public:
	int begin(engine::font& font);
	int update();

protected:
	void refresh_renderer(engine::renderer& _r);
};

class editor_mode
{
	engine::renderer r;
	std::unique_ptr<editor> c_editor;
	engine::font font;
	int initualize();
	int start_game(std::function<int()> game);
public:
	int start(std::function<int()> game);
};

}
