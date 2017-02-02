#ifndef RPG_EDITOR_HPP
#define RPG_EDITOR_HPP

#include <engine/renderer.hpp>
#include <engine/rect.hpp>

#include <rpg/tilemap_manipulator.hpp>
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
	editor_gui();

	void clear();

	tgui::Label::Ptr add_label(const std::string& text, tgui::Container::Ptr pContainer = nullptr);
	tgui::TextBox::Ptr add_textbox(tgui::Container::Ptr pContainer = nullptr);
	tgui::ComboBox::Ptr add_combobox(tgui::Container::Ptr pContainer = nullptr);
	tgui::Button::Ptr add_button(const std::string& text, tgui::Container::Ptr pContainer = nullptr);
	std::shared_ptr<tgui_list_layout> add_sub_container(tgui::Container::Ptr pContainer = nullptr);

	void update_camera_position(engine::fvector pPosition);

	int draw(engine::renderer& pR);

private:

	float mUpdate_timer;
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

class editor_boundary_visualization :
	public engine::render_object
{
public:
	editor_boundary_visualization();
	void set_boundary(engine::frect pBoundary);

	virtual int draw(engine::renderer &pR);
private:
	std::array<engine::rectangle_node, 4> mLines;
	void setup_lines();
};

class editor :
	public engine::render_object
{
public:
	editor();
	bool open_scene(std::string pPath);
	void set_editor_gui(editor_gui& pEditor_gui);
	void set_resource_manager(engine::resource_manager& pResource_manager);

	virtual int save() = 0;

protected:
	virtual bool editor_open() = 0;

	engine::rectangle_node mBlackout;

	rpg::tilemap_manipulator    mTilemap_loader;
	rpg::tilemap_display   mTilemap_display;

	rpg::scene_loader mLoader;

	editor_gui* mEditor_gui;
	engine::resource_manager* mResource_manager;

	editor_boundary_visualization mBoundary_visualization;

	virtual void setup_editor(editor_gui& pEditor_gui){}
};

class tilemap_editor :
	public editor
{
public:
	tilemap_editor();
	int draw(engine::renderer& pR);
	int save();

	void clean();

protected:
	virtual bool editor_open();
	
private:
	size_t mCurrent_tile;
	int    mRotation;
	int    mLayer;
	bool   mIs_highlight;

	engine::fvector last_tile;

	std::vector<std::string> mTile_list;

	std::shared_ptr<engine::texture> mTexture;
	std::string mCurrent_texture_name;

	engine::sprite_node mPreview;

	tgui::ComboBox::Ptr mCb_tile;
	tgui::Label::Ptr mLb_layer;
	tgui::Label::Ptr mLb_rotation;
	tgui::TextBox::Ptr mTb_texture;

	void setup_editor(editor_gui& pEditor_gui);

	void update_tile_combobox_list();
	void update_tile_combobox_selected();
	void update_labels();
	void update_preview();
	void update_highlight();

	void tick_highlight(engine::renderer& pR);

	void apply_texture();

};

class collisionbox_editor :
	public editor
{
public:
	collisionbox_editor();

	int draw(engine::renderer& pR);
	int save();

protected:
	virtual bool editor_open();

private:

	// Siply extends the frect with a group name string
	struct wall :
		public engine::frect
	{
		wall() {};
		wall(engine::frect r) : engine::frect(r) {};
		std::string group;
	};

	std::shared_ptr<rpg::collision_box> mSelection;

	enum class state
	{
		normal,
		size_mode
	};

	state mState;
	engine::fvector mDrag_from;

	tgui::ComboBox::Ptr mCb_type;
	tgui::Label::Ptr    mLb_tilesize;
	tgui::TextBox::Ptr  mTb_wallgroup;

	std::shared_ptr<tgui_list_layout> mLo_door;
	tgui::TextBox::Ptr mTb_door_name;
	tgui::TextBox::Ptr mTb_door_destination;
	tgui::TextBox::Ptr mTb_door_scene;
	tgui::TextBox::Ptr mTb_door_offsetx;
	tgui::TextBox::Ptr mTb_door_offsety;

	rpg::collision_box::type     mCurrent_type;
	rpg::collision_box_container mContainer;

	engine::rectangle_node mTile_preview;
	engine::rectangle_node mWall_display;
	engine::rectangle_node mResize_display;

	void setup_editor(editor_gui& pEditor_gui);

	void apply_wall_settings();

	bool tile_selection(engine::fvector pCursor, bool pCycle = true);
	void update_labels();
	void update_door_settings_labels();
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