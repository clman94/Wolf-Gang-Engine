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

namespace ImGui
{
struct RendererData;
}

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
	void add_new(Targs&&... pArgs)
	{
		mUndo.push_back(std::make_shared<T>(std::forward<Targs>(pArgs)...));
	}

	template<typename T, typename... Targs>
	bool execute(Targs&&... pArgs)
	{
		assert(!mCurrent);
		mRedo.clear();
		add_new<T>(std::forward<Targs>(pArgs)...);
		return mUndo.back()->execute();
	}

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

	void cancel();
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

enum class message_type
{
	unknown,

};

struct message
{
	message_type type;
};

struct message_scene_changed
	: public message
{
	rpg::game *game;

};

class editor_tool
{
public:
	virtual void process_message(message* pMessage) = 0;
	virtual void update() = 0;
};

class log
	: public editor_tool
{
public:
	virtual void process_message(message* pMessage) {}
	virtual void update();

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

class atlas_imgui_editor
{
public:
	atlas_imgui_editor();
	~atlas_imgui_editor();

	void request_open_texture(const std::string pName); // The editor will ask the player to save before opening the new texture
	void update();

	void set_resource_manager(engine::resource_manager& pRes_mgr);

	void set_window_open_handler(bool* mBool);

	bool is_modified() const;
	void save();
	bool is_confirming_save() const;
	void confirm_save();

private:
	void open_requested();

	void center_subtexture_preview();

	int mCurrent_frame;
	bool mIs_playing;
	engine::clock mAnim_clock;
	void play_animation();
	void stop_animation();
	void update_animation();

	std::string mNew_entry_name;

	bool* mWindow_open;

	void new_entry_popup();

	ImGui::RendererData* mFull_texture_renderdata;
	ImGui::RendererData* mSubtexture_renderdata;

	std::string mReq_texture_name;

	engine::node mTexture_node;

	std::shared_ptr<engine::texture> mTexture;
	engine::subtexture::ptr mSubtexture;
	engine::frect mOriginal_rect, mChange_rect;

	bool mModified;

	engine::resource_manager* mResource_manager;
};

class WGE_imgui_editor
{
public:
	WGE_imgui_editor();
	~WGE_imgui_editor();
	void run();

private:
	enum
	{
		snapping_none = 0,
		snapping_pixel,
		snapping_4x4,
		snapping_2x2,
		snapping_1x1,
	};

	enum
	{
		editor_tilemap,
		editor_collision
	};

	// Game Settings
	int mTile_size;

	bool mIs_game_view_window_focused; // Used to determine if game recieves events or not

	// Tilemap Editor
	size_t mSelected_tile;
	int mTile_rotation;

	std::vector<std::string> mScene_list;

	engine::display_window mWindow;

	sf::RenderTexture mGame_render_target;
	rpg::game mGame;
	engine::renderer mGame_renderer;

	rpg::tilemap_manipulator mTilemap_manipulator;
	rpg::tilemap_display mTilemap_display;
	std::shared_ptr<engine::texture> mTilemap_texture;
	engine::subtexture::ptr mCurrent_tile_atlas;
	std::size_t mCurrent_layer;
	command_manager mCommand_manager;
	int mTilemap_current_snapping;

	rpg::collision_box_container mColl_container;
	rpg::collision_box::ptr mSelected_collbox;
	engine::frect mBox_change;
	engine::frect mOriginal_box;

	bool mShow_grid;
	engine::color mGrid_color;

	// Tilemap Commands
	void place_tile(engine::fvector pos);
	void remove_tile(engine::fvector pos);

	// Collision editor commands
	void new_box(rpg::collision_box::ptr pBox, engine::fvector pPos);
	void duplicate_box();
	void delete_box();

	editor_settings_loader mSettings;

	rpg::scene_loader mScene_loader;

	bool mIs_scene_modified; // If true, the user will be asked to save the scene

	// This enables the info being displayed on the top left corner.
	// Toggles with F1.
	bool mShow_debug_info;

	enum window_open_
	{
		window_open_game_window = 0,
		window_open_scene_properties,
		window_open_scene_editor,
		window_open_atlas_editor,
		window_open_log,
		window_open_count,
	};
	std::array<bool, window_open_count> mWindow_open;

	std::string mNew_scene_name;
	std::string mNew_scene_texture_name;
	std::string mChange_scene_texture_name;

	atlas_imgui_editor mAtlas_editor;

	ImGui::RendererData* mScene_editor_rendererdata;

	int mCurrent_scene_editor;

	engine::fvector mLast_tile_position; // Prevents a tile being placed/remove continuously in one spot

	bool mRunning, mIs_closing;

private:
	bool is_ready_to_close();

	void prepare_scene(engine::fs::path pPath, const std::string& pName);
	void save_scene();

	void draw_scene_window();
	void draw_game_window();
	void draw_game_view_window();

	void draw_scene_editor_window();

	void collision_editor_update();
	void draw_collision_editor_settings();

	void tilemap_editor_update();
	void draw_tilemap_editor_settings();
	void draw_tile_group(float from_bottom = 300);
	void draw_tilemap_layers_group();
	void center_tilemap();

	void draw_log_window();

	void handle_undo_redo();
	void handle_save_confirmations();

	void new_scene_popup();
	void change_texture_popup();

	static engine::fvector calc_snapping(int pSnapping, int pTile_size);

};

}

#endif