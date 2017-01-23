#ifndef RPG_EDITOR_HPP
#define RPG_EDITOR_HPP

#include <engine/renderer.hpp>
#include <engine/rect.hpp>

#include <rpg/tilemap_loader.hpp>
#include <rpg/scene_loader.hpp>
#include <rpg/collision_box.hpp>

#include <vector>
#include <string>
#include <list>

#include "../tinyxml2/xmlshortcuts.hpp"

namespace editors
{

class tgui_list_layout :
	public tgui::BoxLayout
{
public:
	void collapse_size();
private:
	void updateWidgetPositions();
};


class editor_gui :
	public engine::render_object
{
public:
	void initualize();

	void clear();

	tgui::Label::Ptr add_label(const std::string& text);

	tgui::TextBox::Ptr add_textbox();

	void update_camera_position(engine::fvector pPosition);

	int draw(engine::renderer& pR);

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
};

class scroll_control_node :
	public engine::node
{
public:
	void movement(engine::renderer& pR);
};

class editor :
	public engine::render_object
{
public:
	virtual bool open_scene(std::string pPath) = 0;

	void set_editor_gui(editor_gui& pEditor_gui);

	void set_resource_manager(engine::resource_manager& pResource_manager);

	virtual int save() = 0;

protected:
	editor_gui* mEditor_gui;
	engine::resource_manager* mResource_manager;

	virtual void setup_editor(editor_gui& pEditor_gui){}
};

class tilemap_editor :
	public editor
{
public:
	tilemap_editor();
	bool open_scene(std::string pPath);
	int draw(engine::renderer& pR);
	int save();
	
private:
	size_t mCurrent_tile;
	int    mRotation;
	int    mLayer;
	bool   mIs_highlight;

	engine::fvector last_tile;

	std::vector<std::string> mTile_list;

	rpg::scene_loader mLoader;

	std::shared_ptr<engine::texture> mTexture;

	engine::sprite_node mPreview;

	engine::rectangle_node mBlackout;
	engine::rectangle_node mLines[4];

	rpg::tilemap_loader    mTilemap_loader;
	rpg::tilemap_display   mTilemap_display;

	tgui::Label::Ptr mLb_tile;
	tgui::Label::Ptr mLb_layer;
	tgui::Label::Ptr mLb_rotation;

	void setup_editor(editor_gui& pEditor_gui);
	void setup_lines();

	void update_labels();
	void update_preview();
	void update_highlight();
	void update_lines(engine::frect pBoundary);

	void tick_highlight(engine::renderer& pR);

};

class collisionbox_editor :
	public editor
{
public:
	collisionbox_editor();

	bool open_scene(std::string pPath);
	int draw(engine::renderer& pR);
	int save();

private:

	// Siply extends the frect with a group name string
	struct wall :
		public engine::frect
	{
		wall() {};
		wall(engine::frect r) : engine::frect(r) {};
		std::string group;
	};

	size_t mSelection;

	bool mSize_mode;
	engine::fvector mDrag_from;

	tgui::Label::Ptr   mLb_tilesize;
	tgui::TextBox::Ptr mTb_wallgroup;


	std::vector<wall> mWalls;

	engine::rectangle_node mTile_preview;
	engine::rectangle_node mWall_display;
	engine::rectangle_node mResize_display;

	engine::rectangle_node mBlackout;
	rpg::tilemap_loader    mTilemap_loader;
	rpg::tilemap_display   mTilemap_display;
	rpg::scene_loader      mLoader;

	void setup_editor(editor_gui& pEditor_gui);

	bool tile_selection(engine::fvector pCursor);
	void update_labels();
};

class editor_manager :
	public engine::render_object
{
public:
	editor_manager();

	bool is_editor_open();
	void open_tilemap_editor(std::string pScene_path);
	void open_collisionbox_editor(std::string pScene_path);
	void close_editor();

	void update_camera_position(engine::fvector pPosition);

	void set_resource_manager(engine::resource_manager& pResource_manager);

	int draw(engine::renderer& pR);

private:
	void refresh_renderer(engine::renderer& pR);

	editor* mCurrent_editor;

	editor_gui          mEditor_gui;
	scroll_control_node mRoot_node;

	tilemap_editor      mTilemap_editor;
	collisionbox_editor mCollisionbox_editor;
};


}

#endif