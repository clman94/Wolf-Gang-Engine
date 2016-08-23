
#include "renderer.hpp"
#include "tilemap_loader.hpp"

namespace editor
{

class tgui_list_layout :
	public tgui::BoxLayout
{
private:
	void updateWidgetPositions();
};

class editor_gui :
	public engine::render_client
{
public:
	void initualize();

	void clear();

	tgui::Label::Ptr add_label(const std::string& text);

	void update_camera_position(engine::fvector pPosition)
	{ mCamera_offset = pPosition; }

protected:
	std::shared_ptr<tgui_list_layout> get_layout()
	{ return mEditor_layout; }

	virtual void tick(engine::renderer& pR, engine::fvector pCamera_offset){}

private:
	float mUpdate_timer;
	tgui::Label::Ptr mLb_mode;
	tgui::Label::Ptr mLb_mouse;
	tgui::Label::Ptr mLb_fps;
	std::shared_ptr<tgui_list_layout> mLayout;
	std::shared_ptr<tgui_list_layout> mEditor_layout;
	tgui::Gui mTgui;

	engine::fvector mCamera_offset;

	void refresh_renderer(engine::renderer& pR);
	int draw(engine::renderer& pR);
};


}
