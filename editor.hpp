
#include "renderer.hpp"
#include "tilemap_loader.hpp"
#include "rpg_managers.hpp"
#include "scene_loader.hpp"
#include <vector>
#include <string>
#include <list>

#include "tinyxml2\xmlshortcuts.hpp"

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
	{
		mCamera_offset = pPosition;
	}

protected:
	std::shared_ptr<tgui_list_layout> get_layout()
	{
		return mEditor_layout;
	}

	virtual void tick(engine::renderer& pR, engine::fvector pCamera_offset) {}

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

class scroll_control_node :
	public engine::node
{
public:
	void movement(engine::renderer& pR);
};

class editor
{
public:
	void set_editor_gui(editor_gui& pEditor_gui)
	{
		mEditor_gui = &pEditor_gui;
		setup_editor(pEditor_gui);
	}
private:
	editor_gui* mEditor_gui;
protected:
	virtual void setup_editor(editor_gui& pEditor_gui){}
	editor_gui* get_editor_gui()
	{ return mEditor_gui; }
};


class tilemap_editor :
	public engine::render_client,
	public editor
{
public:
	tilemap_editor();
	int open_scene(const std::string& pPath);
	int draw(engine::renderer& pR);
	void set_texture_manager(rpg::texture_manager& pTexture_manager);

private:
	size_t mCurrent_tile;
	int    mRotation;
	int    mLayer;
	bool   mIs_highlight;

	std::string mPath;

	std::vector<std::string> mTile_list;

	rpg::scene_loader mLoader;

	engine::texture* mTexture;

	engine::sprite_node mPreview;

	engine::rectangle_node mBlackout;
	engine::rectangle_node mLines[4];

	scroll_control_node mRoot_node;

	rpg::tilemap_loader    mTilemap_loader;
	rpg::tilemap_display   mTilemap_display;
	rpg::texture_manager*  mTexture_manager;

	tgui::Label::Ptr mLb_tile;
	tgui::Label::Ptr mLb_layer;
	tgui::Label::Ptr mLb_rotation;

	void setup_editor(editor_gui& pEditor_gui);
	void setup_lines();

	void update_labels();
	void update_preview();
	void update_highlight();
	void update_lines(engine::fvector pBoundary);

	void tick_highlight(engine::renderer & pR);

	void save();
};


class collisionbox_editor :
	public engine::render_client,
	public editor
{
public:
	collisionbox_editor();

	int open_scene(const std::string& pPath);
	int draw(engine::renderer& pR);

private:
	scroll_control_node mRoot_node;

	engine::rectangle_node mWall_display;

	std::vector<engine::frect> mWalls;

	engine::rectangle_node mBlackout;
	rpg::tilemap_loader    mTilemap_loader;
	rpg::tilemap_display   mTilemap_display;
	rpg::texture_manager*  mTexture_manager;
	rpg::scene_loader      mLoader;
};


}
