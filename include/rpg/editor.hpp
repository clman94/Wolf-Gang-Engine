#ifndef RPG_EDITOR_HPP
#define RPG_EDITOR_HPP

#include <engine/renderer.hpp>
#include <engine/rect.hpp>
#include <engine/terminal.hpp>

#include <rpg/tilemap_manipulator.hpp>
#include <rpg/tilemap_display.hpp>
#include <rpg/scene_loader.hpp>
#include <rpg/collision_box.hpp>
#include <rpg/scene.hpp>
#include <rpg/rpg.hpp>

#include <vector>
#include <string>
#include <list>
#include <functional>
#include <tuple>


class command
{
public:
	virtual bool execute() = 0;
	virtual bool undo() = 0;
	virtual bool redo() { return execute(); };
};

class command_manager
{
public:
	bool execute(std::shared_ptr<command> pCommand);
	bool add(std::shared_ptr<command> pCommand);

	void start(std::shared_ptr<command> pCommand);
	template<typename T>
	std::shared_ptr<T> current()
	{
		return std::dynamic_pointer_cast<T>(mCurrent);
	}
	void complete();

	bool undo();
	bool redo();
	void clear();

private:
	std::shared_ptr<command> mCurrent;
	std::vector<std::shared_ptr<command>> mUndo;
	std::vector<std::shared_ptr<command>> mRedo;
};

namespace editors
{

const engine::color default_gui_bg_color(30, 30, 30, 255);


class tgui_list_layout :
	public tgui::BoxLayout
{
private:
	void updateWidgetPositions();
};

// This is the side used in all the editors.
// It provides methods to conveniently building it.
class editor_sidebar :
	public tgui_list_layout
{
public:
	editor_sidebar();

	void add_group(const std::string& pText);

	tgui::EditBox::Ptr add_value_int(const std::string& pLabel, std::function<void(int)> pCallback, bool pNeg = true);
	tgui::EditBox::Ptr add_value_int(const std::string& pLabel, int& pValue, bool pNeg = true);

	tgui::EditBox::Ptr add_value_string(const std::string& pLabel, std::function<void(std::string)> pCallback);
	tgui::EditBox::Ptr add_value_string(const std::string& pLabel, std::string& pValue);

	tgui::EditBox::Ptr add_value_float(const std::string& pLabel, std::function<void(float)> pCallback, bool pNeg = true);
	tgui::EditBox::Ptr add_value_float(const std::string& pLabel, float& pValue, bool pNeg = true);

	tgui::ComboBox::Ptr add_value_enum(const std::string& pLabel, std::function<void(size_t)> pCallback, const std::vector<std::string>& pValues, size_t pDefault = 0, bool pBig_mode = false);
	tgui::ComboBox::Ptr add_value_enum(const std::string& pLabel, size_t& pSelection, const std::vector<std::string>& pValues, size_t pDefault = 0, bool pBig_mode = false);

	void add_horizontal_buttons(const std::vector<std::tuple<std::string, std::function<void()>>> pName_callbacks);
	void add_button(const std::string& pLabel, std::function<void()> pCallback);

	tgui::Label::Ptr add_label(const std::string& text, tgui::Container::Ptr pContainer = nullptr);
	tgui::Label::Ptr add_small_label(const std::string& text, tgui::Container::Ptr pContainer = nullptr);
	tgui::TextBox::Ptr add_textbox(tgui::Container::Ptr pContainer = nullptr);
	tgui::ComboBox::Ptr add_combobox(tgui::Container::Ptr pContainer = nullptr);
	tgui::CheckBox::Ptr add_checkbox(const std::string& pName);
	tgui::Button::Ptr add_button(const std::string& text, tgui::Container::Ptr pContainer = nullptr);
	std::shared_ptr<tgui_list_layout> add_sub_container(tgui::Container::Ptr pContainer = nullptr);

private:
	tgui::HorizontalLayout::Ptr create_value_line();
	tgui::HorizontalLayout::Ptr create_value_line(const std::string& pText);
};

class scroll_control_node :
	public engine::node
{
public:
	void movement(engine::renderer& pR);

private:
	//engine::fvector mOffset;
};

class editor_boundary_visualization :
	public engine::render_object
{
public:
	editor_boundary_visualization();
	void set_boundary(engine::frect pBoundary);

	virtual int draw(engine::renderer &pR);
private:
	engine::rectangle_node mLines;
};

class editor :
	public engine::render_object
{
public:
	editor();
	std::shared_ptr<editor_sidebar> get_sidebar();

	void set_game(rpg::game& pGame);

	virtual bool open_editor() { return true;  }

	bool is_changed() const;

	virtual bool save() { mIs_changed = false; return true; };

protected:
	rpg::game* mGame;

	void editor_changed();
	std::shared_ptr<editor_sidebar> mSidebar;

private:
	bool mIs_changed;
};

class scene_editor :
	public editor
{
public:
	scene_editor();
	bool open_scene(std::string pName); // Should be called before open_editor()

private:
	tgui::ComboBox::Ptr mCb_scene;

protected:
	float mZoom;
	void update_zoom(engine::renderer& pR);

	rpg::scene_loader mLoader;
	editor_boundary_visualization mBoundary_visualization;
	rpg::tilemap_manipulator mTilemap_manipulator;
	rpg::tilemap_display     mTilemap_display;
	scroll_control_node mMain_scroll;
};

class game_editor :
	public editor
{
public:
	game_editor();

	bool save() { return 0; }

	void set_subwindow(engine::frect pRect);

	int draw(engine::renderer& pR);

	rpg::game& get_game();

	void load_game(const std::string& pSource);

protected:
	virtual void refresh_renderer(engine::renderer& pR);

private:
	void update_scene_list();
	void setup_gui();
	engine::renderer mRenderer;
	rpg::game mGame;

	engine::timer mInfo_update_timer;
	tgui::ComboBox::Ptr mCb_scene;
	tgui::Label::Ptr mLb_mouse;
	tgui::Label::Ptr mLb_fps;
};

class tilemap_layer_list :
	public tgui::VerticalLayout
{
public:
	typedef std::function<void(size_t)> selection_callback;

	tilemap_layer_list();

	void set_tilemap_manipulator(rpg::tilemap_manipulator& pTm_man);
	void set_tilemap_display(rpg::tilemap_display& pTm_displ);

	void refresh_list();

	void set_selected_layer(size_t pIndex);
	void set_selection_callback(selection_callback pCallback);

private:
	void create_item(rpg::tilemap_layer& pLayer, size_t pIndex);
	rpg::tilemap_manipulator* mTilemap_manipulator;
	rpg::tilemap_display* mTilemap_display;

	tgui::VerticalLayout::Ptr mLo_list;

	size_t mSelected_index;
	selection_callback mSelection_callback;
};

class tilemap_editor :
	public scene_editor
{
public:
	tilemap_editor();
	virtual bool open_editor();
	int draw(engine::renderer& pR);
	void load_terminal_interface(engine::terminal_system& pTerminal);

	bool save() override;

	void clean();

private:
	std::shared_ptr<engine::terminal_command_group> mTilemap_group;

	enum class state
	{
		none,
		drawing,
		drawing_region,
		erasing,
	} mState;

	size_t mCurrent_tile; // Index of mTile_list
	int    mRotation;
	int    mLayer;
	bool   mIs_highlight;

	engine::fvector mLast_tile;

	std::vector<std::string> mTile_list;

	std::shared_ptr<engine::texture> mTexture;
	std::string mCurrent_texture_name;

	engine::sprite_node mPreview;

	tgui::ComboBox::Ptr mCb_tile;
	tgui::Label::Ptr mLb_layer;
	tgui::Label::Ptr mLb_rotation;
	tgui::EditBox::Ptr mTb_texture;
	tgui::CheckBox::Ptr mCb_half_grid;
	std::shared_ptr<tilemap_layer_list> mLayer_list;

	engine::grid mGrid;

	void setup_gui();

	command_manager mCommand_manager;

	// User Actions

	void copy_tile_type_at(engine::fvector pAt);
	void draw_tile_at(engine::fvector pAt);
	void erase_tile_at(engine::fvector pAt);
	void next_tile();
	void previous_tile();
	void layer_up();
	void layer_down();
	void rotate_clockwise();

	void update_tile_combobox_list();
	void update_tile_combobox_selected();
	void update_labels();
	void update_preview();
	void update_highlight();
	void update_tilemap();

	void tick_highlight(engine::renderer& pR);

	void apply_texture();
};

class collisionbox_editor :
	public scene_editor
{
public:
	collisionbox_editor();

	virtual bool open_editor();

	int draw(engine::renderer& pR);
	void load_terminal_interface(engine::terminal_system& pTerminal);

	bool save();
	
private:
	std::shared_ptr<engine::terminal_command_group> mCollision_editor_group;

	command_manager mCommand_manager;

	std::shared_ptr<rpg::collision_box> mSelection;


	enum class state
	{
		normal,
		size_mode,
		move_mode,
		resize_mode
	};

	state mState;
	engine::fvector mDrag_from;

	enum class grid_snap
	{
		none,
		pixel,
		eighth,
		quarter,
		full
	} mGrid_snap;

	engine::frect mResize_mask;
	engine::frect mOriginal_rect; // For resize_mode

	tgui::ComboBox::Ptr mCb_type;
	tgui::EditBox::Ptr  mTb_wallgroup;
	tgui::EditBox::Ptr  mTb_box_x;
	tgui::EditBox::Ptr  mTb_box_y;
	tgui::EditBox::Ptr  mTb_box_width;
	tgui::EditBox::Ptr  mTb_box_height;

	tgui::EditBox::Ptr  mTb_door_name;
	tgui::EditBox::Ptr  mTb_door_destination;
	tgui::ComboBox::Ptr mTb_door_scene;
	tgui::EditBox::Ptr  mTb_door_offsetx;
	tgui::EditBox::Ptr  mTb_door_offsety;

	rpg::collision_box::type     mCurrent_type;
	rpg::collision_box_container mContainer;

	engine::rectangle_node mWall_display;
	engine::rectangle_node mResize_display;

	engine::grid mGrid;

	void setup_gui();

	bool tile_selection(engine::fvector pCursor, bool pCycle = true);
	void update_labels();
};

class atlas_editor :
	public editor
{
public:
	atlas_editor();

	virtual bool open_editor();

	int draw(engine::renderer& pR);

	bool save();

private:
	void get_textures(const std::string& pPath);
	void setup_for_texture(const engine::encoded_path& pPath);

	enum class state
	{
		normal,
		size_mode,
		move_mode,
		resize_mode,
	} mState;

	bool mAtlas_changed;
	engine::encoded_path mLoaded_texture;
	std::vector<engine::encoded_path> mTexture_list;
	std::shared_ptr<engine::texture> mTexture;

	engine::texture_atlas mAtlas;
	engine::subtexture::ptr mSelection;

	void new_entry();
	void remove_selected();

	engine::fvector mDrag_offset;

	float mZoom;

	engine::rectangle_node mBackground;
	engine::sprite_node mSprite;
	engine::rectangle_node mPreview_bg;
	engine::animation_node mPreview;
	engine::rectangle_node mFull_animation;
	engine::rectangle_node mSelected_firstframe;

	tgui::ComboBox::Ptr mCb_texture_select;
	tgui::ComboBox::Ptr mCb_entry_select;
	tgui::EditBox::Ptr  mTb_name;
	tgui::EditBox::Ptr  mTb_frames;
	tgui::EditBox::Ptr  mTb_default_frame;
	tgui::EditBox::Ptr  mTb_interval;
	tgui::ComboBox::Ptr mCb_loop;
	tgui::EditBox::Ptr  mTb_size_x;
	tgui::EditBox::Ptr  mTb_size_y;
	tgui::EditBox::Ptr  mTb_size_w;
	tgui::EditBox::Ptr  mTb_size_h;
	tgui::ComboBox::Ptr mCb_bg_color;

	void atlas_selection(engine::fvector pPosition);

	void setup_gui();

	void black_background();
	void white_background();

	void update_entry_list();
	void update_settings();
	void update_preview();
	void clear_gui();
};

class WGE_editor
{
public:
	WGE_editor();

	void initualize(const std::string& pCustom_location);

	void run();

private:
	tgui::Tab::Ptr mTabs;

	std::shared_ptr<tgui::VerticalLayout> mGui_base;
	std::shared_ptr<tgui_list_layout> mSidebar;
	std::shared_ptr<tgui_list_layout> mVisualizations_layout;
	tgui::Panel::Ptr mRender_container;
	tgui::Label::Ptr mBottom_text;

	tgui::Panel::Ptr mSave_background;
	tgui::Panel::Ptr mSave_accept;

	tgui::Theme::Ptr mTheme;

	engine::renderer mRenderer;
	engine::display_window mWindow;

	game_editor         mGame_editor;
	tilemap_editor      mTilemap_editor;
	collisionbox_editor mCollisionbox_editor;
	atlas_editor        mAtlas_editor;

	editor* mCurrent_editor;

	void setup_gui();

	void set_current_editor(editor& pEditor);
};

}

#endif