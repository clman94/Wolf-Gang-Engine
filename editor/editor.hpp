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
	text_node.set_color({ 255, 255, 255 });
	text_node.set_scale(0.5f);
	return 0;
}

class menu
{
public:
	virtual int begin(engine::renderer &r, engine::font& font) = 0;
	virtual int update(engine::renderer &r) = 0;
protected:
	engine::ui::ui_instance instance;
};

class game_editor
	: public menu
{
	engine::ui::button_simple b1, b2;
	engine::ui::input_box tb1;
public:
	int begin(engine::renderer &r, engine::font& font);
	int update(engine::renderer &r);
};

class editor_mode
{
	engine::renderer r;
	std::unique_ptr<menu> menus;
	engine::font font;
	int initualize_renderer();
public:
	int start(std::function<int()> game);
};

}
