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
#include <memory>
#include <utility>

#include <imgui.h>
#include <imgui-SFML.h>

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

	template<typename T, typename... Targs>
	bool execute(Targs&&... pArgs)
	{
		assert(!mCurrent);
		mRedo.clear();
		auto cmd = std::make_shared<T>(std::forward<Targs>(pArgs)...);
		mUndo.push_back(cmd);
		return cmd->execute();
	}

	bool add(std::shared_ptr<command> pCommand);

	// Begin a command.
	template<typename T, typename... Targs>
	std::shared_ptr<T> start(Targs&&... pArgs)
	{
		std::shared_ptr<T> ptr = std::make_shared<T>(std::forward<Targs>(pArgs)...);
		mCurrent = ptr;
		return ptr;
	}
	template<typename T>
	std::shared_ptr<T> current()
	{
		return std::dynamic_pointer_cast<T>(mCurrent);
	}
	void complete(); // Archives the current command in the undo list without executing
	bool execute_and_complete();

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

class editor :
	public engine::render_object
{
public:
	editor();

	void set_game(rpg::game& pGame);

	virtual bool open_editor() { return true;  }

	bool is_changed() const;

	virtual bool save() { mIs_changed = false; return true; };

protected:
	rpg::game* mGame;

	void editor_changed();

private:
	bool mIs_changed;
};

class scene_editor :
	public editor
{
public:
	scene_editor();
	bool open_scene(std::string pName); // Should be called before open_editor()

protected:
	float mZoom;
	void update_zoom(engine::renderer& pR);

	rpg::scene_loader mLoader;
	rpg::tilemap_manipulator mTilemap_manipulator;
	rpg::tilemap_display     mTilemap_display;
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

	rpg::collision_box::type     mCurrent_type;
	rpg::collision_box_container mContainer;

	engine::rectangle_node mWall_display;
	engine::rectangle_node mResize_display;

	engine::grid mGrid;

	bool tile_selection(engine::fvector pCursor, bool pCycle = true);
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
	void setup_for_texture(const engine::generic_path& pPath);

	enum class state
	{
		normal,
		size_mode,
		move_mode,
		resize_mode,
	} mState;

	bool mAtlas_changed;
	engine::generic_path mLoaded_texture;
	std::vector<engine::generic_path> mTexture_list;
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

	void atlas_selection(engine::fvector pPosition);

	void black_background();
	void white_background();

	void update_preview();
};

class editor_settings_loader
{
public:
	bool load(const engine::fs::path& pPath);

	std::string generate_open_cmd(const std::string& pFilepath) const;
	std::string generate_opento_cmd(const std::string& pFilepath, size_t pRow, size_t pCol);

private:
	std::string mPath;
	std::string mOpen_param;
	std::string mOpento_param;
};

class WGE_imgui_editor
{
public:
	WGE_imgui_editor();
	void run();

private:

	enum
	{
		snapping_none = 0,
		snapping_pixel,
		snapping_eight,
		snapping_quarter,
		snapping_full,
	};

	// Game Settings
	int mTile_size;

	bool mIs_game_view_window_focused; // Used to determine if game recieves events or not

	// Tilemap Editor
	size_t mSelected_tile;
	int mTile_rotation;

	std::vector<std::string> mScene_list;

	sf::RenderWindow mWindow;

	sf::RenderTexture mGame_render_target;
	rpg::game mGame;
	engine::renderer mGame_renderer;

	// this all will all be refactored soon, simply prototyping
	sf::RenderTexture mTilemap_render_target;
	rpg::tilemap_manipulator mTilemap_manipulator;
	rpg::tilemap_display mTilemap_display;
	engine::renderer mTilemap_renderer;
	std::shared_ptr<engine::texture> mTilemap_texture;
	engine::subtexture::ptr mCurrent_tile_atlas;
	engine::node mTilemap_center_node; // Never changes position but scaling will cause zooming in and out.
	float mTilemap_scale;
	std::size_t mCurrent_layer;
	command_manager mCommand_manager;
	int mTilemap_current_snapping;
	bool mShow_grid;

	// Tilemap Commands
	void place_tile(engine::fvector pos);
	void remove_tile(engine::fvector pos);

	engine::primitive_builder mPrimitives;

	editor_settings_loader mSettings;

	rpg::scene_loader mScene_loader;

	bool mIs_scene_modified;

	bool mShow_debug_info;

	char mNew_scene_name_buf[256];
	std::string mNew_scene_texture_name;
	std::string mChange_scene_texture_name;

private:
	void prepare_scene(engine::fs::path pPath, const std::string& pName);
	void save_scene();

	void draw_scene_window();
	void draw_game_window();
	void draw_game_view_window();

	void draw_tilemap_editor_window();
	void draw_tile_group(float from_bottom = 300);
	void draw_tilemap_layers_group();
	void center_tilemap();

	void draw_collision_settings_window();

	void draw_log_window();

	void handle_undo_redo();
	void handle_scene_change();

	void new_scene_popup();
	void change_texture_popup();

	static engine::fvector calc_snapping(int pSnapping, int pTile_size);

};

}

#endif