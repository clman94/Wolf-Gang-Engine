#ifndef RPG_HPP
#define RPG_HPP

#include "../../tinyxml2/tinyxml2.h"

#include <engine/renderer.hpp>
#include <engine/utility.hpp>
#include <engine/particle_engine.hpp>
#include <engine/your_soul.hpp>
#include <engine/pathfinding.hpp>
#include <engine/filesystem.hpp>
#include <engine/audio.hpp>
#include <engine/terminal.hpp>
#include <engine/controls.hpp>

#include <rpg/rpg_config.hpp>
#include <rpg/tilemap_manipulator.hpp>

#ifndef LOCKED_RELEASE_MODE
#include <rpg/editor.hpp>
#endif

#include <rpg/scene_loader.hpp>
#include <rpg/script_system.hpp>
#include <rpg/collision_system.hpp>
#include <rpg/flag_container.hpp>
#include <rpg/panning_node.hpp>
#include <rpg/entity.hpp>
#include <rpg/sprite_entity.hpp>
#include <rpg/character_entity.hpp>
#include <rpg/player_character.hpp>
#include <rpg/rpg_resource_directories.hpp>
#include <rpg/script_context.hpp>

#include <set>
#include <list>
#include <string>
#include <array>
#include <functional>
#include <map>
#include <fstream>
#include <memory>

namespace rpg
{
#ifndef LOCKED_RELEASE_MODE
class terminal_gui
{
public:
	terminal_gui();
	void set_terminal_system(engine::terminal_system& pTerminal_system);
	void load_gui(engine::renderer& pR);
	void update(engine::renderer& pR);

private:
	size_t mCurrent_history_entry;
	std::vector<std::string> mHistory;
	tgui::EditBox::Ptr mEb_input;
};
#endif

// Resource management of expression animations
class expression_manager
{
public:
	struct expression
	{
		std::shared_ptr<engine::texture> texture;
		std::shared_ptr<const engine::animation> animation;
	};

	util::optional_pointer<const expression> find_expression(const std::string& mName);
	int load_expressions_xml(tinyxml2::XMLElement * pRoot, engine::resource_manager& pResource_manager);

private:

	std::map<std::string, expression> mExpressions;
};


class text_entity :
	public entity
{
public:
	text_entity();

	int draw(engine::renderer & pR);

	virtual type get_type() const
	{ return type::text; }

	engine::formatted_text_node mText;
};

class dialog_text_entity :
	public text_entity
{
public:
	dialog_text_entity();

	void clear();

	int draw(engine::renderer& pR);

	bool is_revealing();
	void reveal(const std::string& pText, bool pAppend);
	void skip_reveal();

	void set_interval(float pMilliseconds);

	// Returns whether or not the text has revealed a
	// new letter in this frame
	bool has_revealed_character();

	void set_wordwrap(size_t pLength);
	void set_max_lines(size_t pLines);

private:
	void adjust_text();

	size_t mWord_wrap;
	size_t mMax_lines;
	bool        mNew_character;
	bool        mRevealing;
	size_t      mCount;
	engine::text_format mFull_text;
	engine::counter_clock mTimer;

	void do_reveal();
};

class entity_manager :
	public engine::render_proxy
{
public:
	entity_manager();

	void clean();

	void load_script_interface(script_system& pScript);
	void set_resource_manager(engine::resource_manager& pResource_manager);

	void set_root_node(engine::node& pNode);

private:

	template<typename T>
	T* create_entity()
	{
		if (mEntities.size() >= 1024)
		{
			util::error("Reached upper limit of characters.");
			return nullptr;
		}
		auto new_entity = new T();
		mEntities.push_back(std::unique_ptr<entity>(dynamic_cast<entity*>(new_entity)));
		mRoot_node->add_child(*new_entity);
		get_renderer()->add_object(*new_entity);
		return new_entity;
	}

	engine::node* mRoot_node;

	void register_entity_type(script_system& pScript);

	engine::resource_manager*  mResource_manager;
	script_system* mScript_system;

	std::vector<std::unique_ptr<entity>> mEntities;

	bool check_entity(entity_reference& e);

	// General
	void             script_remove_entity(entity_reference& e);
	void             script_set_position(entity_reference& e, const engine::fvector& pos);
	engine::fvector  script_get_position(entity_reference& e);
	void             script_set_depth_direct(entity_reference& e, float pDepth);
	void             script_set_depth(entity_reference& e, float pDepth);
	void             script_set_depth_fixed(entity_reference& e, bool pFixed);
	void             script_set_anchor(entity_reference& e, int pAnchor);
	void             script_set_visible(entity_reference& e, bool pIs_visible);
	void             script_add_child(entity_reference& e1, entity_reference& e2);
	void             script_set_parent(entity_reference& e1, entity_reference& e2);
	void             script_detach_children(entity_reference& e);
	void             script_detach_parent(entity_reference& e);
	void             script_make_gui(entity_reference& e, float pOffset);
	void             script_set_z(entity_reference& e, float pZ);
	float            script_get_z(entity_reference& e);
	bool             script_is_character(entity_reference& e);
	void             script_set_parallax(entity_reference& e, float pParallax);

	// Text-based
	entity_reference script_add_text();
	void             script_set_text(entity_reference& e, const std::string& pText);
	void             script_set_font(entity_reference& e, const std::string& pName);
	
	// Sprite-based
	entity_reference script_add_entity(const std::string& tex);
	entity_reference script_add_entity_atlas(const std::string& tex, const std::string& atlas);
	engine::fvector  script_get_size(entity_reference& e);
	void             script_start_animation(entity_reference& e);
	void             script_stop_animation(entity_reference& e);
	void             script_pause_animation(entity_reference& e);
	void             script_set_animation_speed(entity_reference& e, float pSpeed);
	float            script_get_animation_speed(entity_reference& e);
	bool             script_is_animation_playing(entity_reference& e);
	void             script_set_atlas(entity_reference& e, const std::string& name);
	void             script_set_rotation(entity_reference& e, float pRotation);
	float            script_get_rotation(entity_reference& e);
	void             script_set_color(entity_reference& e, int r, int g, int b, int a);
	void             script_set_texture(entity_reference & e, const std::string & name);
	void             script_set_scale(entity_reference& e, const engine::fvector& pScale);
	engine::fvector  script_get_scale(entity_reference& e);

	// Character
	entity_reference script_add_character(const std::string& tex);
	void             script_set_direction(entity_reference& e, int dir);
	int              script_get_direction(entity_reference& e);
	void             script_set_cycle(entity_reference& e, const std::string& name);

	// Dialog Text
	entity_reference script_add_dialog_text();
	void             script_reveal(entity_reference& e, const std::string& pText, bool pAppend);
	bool             script_is_revealing(entity_reference& e);
	void             script_skip_reveal(entity_reference& e);
	void             script_set_interval(entity_reference& e, float pMilli);
	bool             script_has_displayed_new_character(entity_reference& e);
	void             script_dialog_set_wordwrap(entity_reference& e, unsigned int pLength);
	void             script_dialog_set_max_lines(entity_reference& e, unsigned int pLines);

};

class background_music
{
public:
	background_music();
	void load_script_interface(script_system& pScript);
	void clean();
	void set_root_directory(const std::string& pPath);
	void set_resource_pack(engine::pack_stream_factory* pPack);
	void pause_music();

private:

	engine::pack_stream_factory* mPack;

	std::unique_ptr<engine::sound_stream> mStream;
	std::unique_ptr<engine::sound_stream> mOverlap_stream;

	engine::fs::path mRoot_directory;
	engine::fs::path mPath;
	engine::fs::path mOverlay_path;

	bool script_music_open(const std::string& pName);
	bool script_music_swap(const std::string& pName);
	int script_music_start_transition_play(const std::string& pName);
	void script_music_stop_transition_play();
	void script_music_set_second_volume(float pVolume);

};

// A colored rectangle overlay of the entire screen for
// fade effects
class colored_overlay :
	public engine::render_proxy
{
public:
	colored_overlay();
	void load_script_interface(script_system& pScript);
	void clean();

private:
	void refresh_renderer(engine::renderer& pR);

	engine::rectangle_node mOverlay;

	void script_set_overlay_color(int r, int g, int b);
	void script_set_overlay_opacity(int a);
};

// The main pathfinding system for tilemap based pathfinding
class pathfinding_system
{
public:
	pathfinding_system();

	// Pathfinding uses the walls in the collsiion system for obsticle checking
	void set_collision_system(collision_system& pCollision_system);

	void load_script_interface(script_system& pScript);

private:
	collision_system* mCollision_system;
	engine::pathfinder mPathfinder;

	bool script_find_path(AS::CScriptArray& pScript_path, engine::fvector pStart, engine::fvector pDestination);
	bool script_find_path_partial(AS::CScriptArray& pScript_path, engine::fvector pStart, engine::fvector pDestination, int pCount);
};

class scenes_directory :
	public engine::resource_directory
{
public:
	scenes_directory();
	bool load(engine::resource_manager& pResource_manager);
	void set_path(const std::string& pFilepath);
	void set_script_system(script_system& pScript);
private:
	std::string mPath;
	script_system* mScript;
};

class game_settings_loader
{
public:
	bool load(const std::string& pPath, const std::string& pPrefix_path = std::string());
	bool load_memory(const char* pData, size_t pSize, const std::string& pPrefix_path = std::string());

	const std::string& get_start_scene() const;
	const std::string& get_textures_path() const;
	const std::string& get_sounds_path() const;
	const std::string& get_music_path() const;
	const std::string& get_fonts_path() const;
	const std::string& get_scenes_path() const;
	const std::string& get_player_texture() const;
	engine::fvector    get_screen_size() const;
	float get_unit_pixels() const;
	const engine::controls& get_key_bindings() const;

private:
	bool parse_settings(tinyxml2::XMLDocument& pDoc, const std::string& pPrefix_path);
	bool parse_key_bindings(tinyxml2::XMLElement* pEle);
	bool parse_binding_attributes(tinyxml2::XMLElement* pEle, const std::string& pName
		, const std::string& pPrefix, bool pAlternative);

	std::string mStart_scene;
	std::string mTextures_path;
	std::string mSounds_path;
	std::string mMusic_path;
	std::string mPlayer_texture;
	std::string mFonts_path;
	std::string mScenes_path;
	engine::fvector mScreen_size;
	engine::controls mKey_bindings;
	float pUnit_pixels;

	std::string load_setting_path(tinyxml2::XMLElement* pRoot, const std::string& pName, const std::string& pDefault);
};

class scene :
	public engine::render_proxy
{
public:

	scene();
	~scene();

	panning_node& get_world_node();

	collision_system& get_collision_system();

	// Cleanups the scene for a new scene.
	// Does not stop background music by default 
	// so it can be continued in the next scene.
	void clean(bool pFull = false);

	// Load scene xml file which loads the scene script.
	// The strings are not references so cleanup doesn't cause issues.
	bool load_scene(std::string pName);
	bool load_scene(std::string pName, std::string pDoor);

#ifndef LOCKED_RELEASE_MODE
	bool create_scene(const std::string& pName);
#endif

	// Reload the currently loaded scene.
	bool reload_scene();

	const std::string& get_path();
	const std::string& get_name();

	void load_script_interface(script_system& pScript);

#ifndef LOCKED_RELEASE_MODE
	void load_terminal_interface(engine::terminal_system& pTerminal);
#endif

	void set_resource_manager(engine::resource_manager& pResource_manager);

	// Loads global scene settings from game.xml file.
	bool load_settings(const game_settings_loader& pSettings);

	player_character& get_player();

	void tick(engine::controls &pControls);

	void focus_player(bool pFocus);

	void set_resource_pack(engine::pack_stream_factory* pPack);

private:
	std::vector<std::shared_ptr<script_function>> mEnd_functions;

	std::map<std::string, scene_script_context> pScript_contexts;

	panning_node mWorld_node;

	engine::pack_stream_factory* mPack;
	engine::resource_manager* mResource_manager;
	script_system*            mScript;

	tilemap_display       mTilemap_display;
	tilemap_manipulator   mTilemap_manipulator;
	collision_system      mCollision_system;
	entity_manager        mEntity_manager;
	background_music      mBackground_music;
	engine::sound_spawner mSound_FX;
	player_character      mPlayer;
	colored_overlay       mColored_overlay;
	pathfinding_system    mPathfinding_system;

#ifndef LOCKED_RELEASE_MODE
	std::shared_ptr<engine::terminal_command_group> mTerminal_cmd_group;
#endif

	std::string mCurrent_scene_name;
	scene_loader mLoader;

	bool mFocus_player;

	void             script_set_tile(const std::string& pAtlas
		                  , engine::fvector pPosition, int pLayer, int pRotation);
	void             script_remove_tile(engine::fvector pPosition, int pLayer);
	void             script_set_focus(engine::fvector pPosition);
	engine::fvector  script_get_focus();
	entity_reference script_get_player();
	engine::fvector  script_get_boundary_position();
	engine::fvector  script_get_boundary_size();
	void             script_set_boundary_position(engine::fvector pPosition);
	void             script_set_boundary_size(engine::fvector pSize);

	void             script_spawn_sound(const std::string& pName, float pVolume, float pPitch);

	engine::fvector  script_get_display_size();

	void refresh_renderer(engine::renderer& _r);
	void update_focus();
	void update_collision_interaction(engine::controls &pControls);
};

// A basic save system.
// Saves player position, flags, and current scene path.
class save_system
{
public:
	save_system();

	void clean();

	bool open_save(const std::string& pPath);
	void load_flags(flag_container& pFlags);
	engine::fvector get_player_position();
	std::string get_scene_path();
	std::string get_scene_name();

	util::optional<int> get_int_value(const engine::encoded_path& pPath) const;
	util::optional<float> get_float_value(const engine::encoded_path& pPath) const;
	util::optional<std::string> get_string_value(const engine::encoded_path& pPath) const;
	std::vector<std::string> get_directory_entries(const engine::encoded_path& pDirectory) const;

	bool set_value(const engine::encoded_path& pPath, int pValue);
	bool set_value(const engine::encoded_path& pPath, float pValue);
	bool set_value(const engine::encoded_path& pPath, const std::string& pValue);
	bool remove_value(const engine::encoded_path& pPath);

	bool has_value(const engine::encoded_path& pPath) const;

	void new_save();
	void save(const std::string& pPath);
	void save_flags(flag_container& pFlags);
	void save_scene(scene& pScene);

private:
	void value_factory(tinyxml2::XMLElement * pEle);
	void load_values();
	void save_values();

	struct value
	{
		engine::encoded_path mPath;
		virtual void save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const = 0;
		virtual void load(tinyxml2::XMLElement * pEle_value) = 0;
	};

	struct int_value : public value
	{
		int mValue;
		virtual void save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const;
		virtual void load(tinyxml2::XMLElement * pEle_value);
	};

	struct float_value : public value
	{
		float mValue;
		virtual void save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const;
		virtual void load(tinyxml2::XMLElement * pEle_value);
	};

	struct string_value : public value
	{
		std::string mValue;
		virtual void save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const;
		virtual void load(tinyxml2::XMLElement * pEle_value);
	};

	std::vector<std::unique_ptr<value>> mValues;
	value* find_value(const engine::encoded_path& pPath) const;

	template<typename T>
	T* ensure_existence(const engine::encoded_path& pPath)
	{
		value* val = find_value(pPath);

		// Create a new one
		if (!val)
		{
			auto new_value = new T;
			new_value->mPath = pPath;
			mValues.push_back(std::move(std::unique_ptr<value>(static_cast<value*>(new_value))));
			return new_value;
		}

		// Just cast and go
		auto cast = dynamic_cast<T*>(val);
		return cast;
	}

	tinyxml2::XMLDocument mDocument;
	tinyxml2::XMLElement *mEle_root;
	void save_player(player_character& pPlayer);
};

class scene_load_request
{
public:

	enum class to_position
	{
		none,
		door,
		position
	};

	scene_load_request();

	void request_load(const std::string& pScene_name);
	bool is_requested() const;
	const std::string& get_scene_name() const;
	void complete();

	void set_player_position(engine::fvector pPosition);
	void set_player_position(const std::string& pDoor);
	
	to_position get_player_position_type() const;

	const std::string& get_player_door() const;
	engine::fvector get_player_position() const;

private:
	bool mRequested;
	std::string mScene_name;

	to_position mTo_position;
	std::string mDoor;
	engine::fvector mPosition;
};

// The main game
class game :
	public engine::render_proxy
{
public:
	game();
	~game();

	// Load the xml game settings
	bool load_settings(engine::fs::path pData_dir);

	bool tick();

	bool restart_game();

protected:
	void refresh_renderer(engine::renderer& r);

private:

	// Everything is good and settings are loaded
	bool mIs_ready;

	bool mExit;

	scene            mScene;
	engine::resource_manager mResource_manager;
	engine::pack_stream_factory mPack;
	flag_container   mFlags;
	script_system    mScript;
	engine::controls mControls;
	size_t           mSlot;
	save_system      mSave_system;

	engine::fs::path mData_directory;

#ifndef LOCKED_RELEASE_MODE
	editors::editor_manager mEditor_manager;
	terminal_gui mTerminal_gui;
	engine::terminal_system mTerminal_system;
	void load_terminal_interface();

	std::shared_ptr<engine::terminal_command_group> mGroup_flags;
	std::shared_ptr<engine::terminal_command_group> mGroup_game;
	std::shared_ptr<engine::terminal_command_group> mGroup_global1;
	std::shared_ptr<engine::terminal_command_group> mGroup_slot;
#endif

	scene_load_request mScene_load_request;

	engine::fs::path get_slot_path(size_t pSlot);
	void save_game();
	void open_game();
	bool is_slot_used(size_t pSlot);
	void set_slot(size_t pSlot);
	size_t get_slot();

	void abort_game();

	void script_load_scene(const std::string& pName);
	void script_load_scene_to_door(const std::string& pName, const std::string& pDoor);
	void script_load_scene_to_position(const std::string& pName, engine::fvector pPosition);

	int script_get_int_value(const std::string& pPath) const;
	float script_get_float_value(const std::string& pPath) const;
	std::string script_get_string_value(const std::string& pPath) const;
	bool script_set_int_value(const std::string& pPath, int pValue);
	bool script_set_float_value(const std::string& pPath, float pValue);
	bool script_set_string_value(const std::string& pPath, const std::string& pValue);
	AS::CScriptArray* script_get_director_entries(const std::string& pPath);
	bool script_remove_value(const std::string& pPath);
	bool script_has_value(const std::string& pPath);

	void load_script_interface();

	void load_icon();
	void load_icon_pack();

	float get_delta();
};

}

#endif