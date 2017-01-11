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

#include <rpg/rpg_config.hpp>
#include <rpg/tilemap_loader.hpp>
#include <rpg/editor.hpp>
#include <rpg/scene_loader.hpp>
#include <rpg/script_system.hpp>
#include <rpg/collision_system.hpp>
#include <rpg/flag_container.hpp>
#include <rpg/panning_node.hpp>
#include <rpg/controls.hpp>
#include <rpg/entity.hpp>
#include <rpg/sprite_entity.hpp>
#include <rpg/character_entity.hpp>
#include <rpg/player_character.hpp>
#include <rpg/rpg_resource_directories.hpp>

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

// Resource management of expression animations
class expression_manager
{
public:
	util::optional_pointer<const engine::animation> find_animation(const std::string& mName);
	int load_expressions_xml(tinyxml2::XMLElement * pRoot, engine::resource_manager& pResource_manager);

private:
	std::map<std::string, const engine::animation*> mAnimations;
};

class text_format_profile
{
public:
	// Load xml settings:
	// <font path="" size="" scale=""/>
	int load_settings(tinyxml2::XMLElement* pEle);
	int get_character_size() const;
	float get_scale() const;
	const engine::font& get_font() const;
	void apply_to(engine::text_node& pText) const;

private:
	engine::font mFont;
	int mCharacter_size;
	float mScale;
	//engine::color mColor; // TODO: Implement color
};

class text_entity :
	public entity
{
public:
	text_entity();
	void apply_format(const text_format_profile& pFormat);

	void set_text(const std::string& pText);
	void set_color(engine::color pColor);
	void set_anchor(engine::anchor pAnchor);

	int draw(engine::renderer & pR);

	
	virtual entity_type get_entity_type()
	{ return entity_type::text; }

protected:
	engine::text_node mText;
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
	// new character_entity in this frame
	bool has_revealed_character();

private:
	bool        mNew_character;
	bool        mRevealing;
	size_t      mCount;
	std::string mFull_text;
	engine::counter_clock mTimer;

	void do_reveal();
};

// The dialog object with text reveal
// TODO: Make more flexible with the ability to only have the text displayed,
//       move the text to any location, (possibly) automatically wrap text
//       without cutting off words, and lots more that might be useful.
// POSSIBLY: COMPLETELY REMOVE (or cut down) and use a script implementation 
//             with dialog_text_entity to help
class narrative_dialog :
	public engine::render_object
{
public:
	enum class position
	{
		top,
		bottom
	};

	narrative_dialog();

	void set_box_position(position pPosition);

	// Show the dialog box
	void show_box();

	// Hide the dialog box
	void hide_box();
	
	// Cleanup the current session
	void end_narrative();

	bool is_box_open();

	// The selection is just a simple text object, nothing really special.
	// TODO: Make special
	void show_selection();
	void hide_selection();
	void set_selection(const std::string& pText);

	void set_expression(const std::string& pName);

	int load_narrative_xml(tinyxml2::XMLElement* pEle, engine::resource_manager& pResource_manager);

	void load_script_interface(script_system& pScript);

	int draw(engine::renderer &pR);

	void set_text_format(const text_format_profile& pFormat);

protected:
	void refresh_renderer(engine::renderer& r);

private:
	sprite_entity          mBox;
	engine::sprite_node    mCursor;
	dialog_text_entity     mText;
	engine::text_node      mSelection;

	expression_manager     mExpression_manager;
	engine::animation_node mExpression;

	int load_box(tinyxml2::XMLElement* pEle, engine::resource_manager& pResource_manager);

	void show_expression();
	void reset_positions();

	entity_reference script_get_narrative_box();
	entity_reference script_get_narrative_text();
};


class entity_manager :
	public engine::render_proxy
{
public:
	entity_manager();

	util::optional_pointer<entity> find_entity(const std::string& pName);

	void clean();

	void load_script_interface(script_system& pScript);
	void set_resource_manager(engine::resource_manager& pResource_manager);
	bool is_character(sprite_entity* pEntity);

	template<typename T>
	T* create_entity()
	{
		if (mEntities.size() >= 256)
		{
			util::error("Reached upper limit of characters.");
			return nullptr;
		}
		auto new_entity = new T();
		assert(dynamic_cast<entity*>(new_entity) != nullptr);
		mEntities.push_back(std::unique_ptr<entity>(dynamic_cast<entity*>(new_entity)));
		mRoot_node->add_child(*new_entity);
		get_renderer()->add_object(*new_entity);
		return new_entity;
	}

	void set_text_format(const text_format_profile& pFormat);

	void set_root_node(engine::node& pNode);

private:

	engine::node* mRoot_node;

	void register_entity_type(script_system& pScript);

	engine::resource_manager*  mResource_manager;
	script_system* mScript_system;

	/// Used for the text-based entities
	const text_format_profile* mText_format;

	std::vector<std::unique_ptr<entity>> mEntities;

	bool check_entity(entity_reference& e);

	entity_reference script_add_entity(const std::string& tex);
	entity_reference script_add_entity_atlas(const std::string& tex, const std::string& atlas);
	entity_reference script_add_text();
	void             script_set_text(entity_reference& e, const std::string& pText);
	void             script_remove_entity(entity_reference& e);
	entity_reference script_add_character(const std::string& tex);
	void             script_set_name(entity_reference& e, const std::string& pName);
	void             script_set_position(entity_reference& e, const engine::fvector& pos);
	engine::fvector  script_get_position(entity_reference& e);
	void             script_set_direction(entity_reference& e, int dir);
	void             script_set_cycle(entity_reference& e, const std::string& name);
	void             script_set_depth(entity_reference& e, float pDepth);
	void             script_set_depth_fixed(entity_reference& e, bool pFixed);
	void             script_start_animation(entity_reference& e);
	void             script_stop_animation(entity_reference& e);
	void             script_set_animation(entity_reference& e, const std::string& name);
	void             script_set_anchor(entity_reference& e, int pAnchor);
	void             script_set_rotation(entity_reference& e, float pRotation);
	void             script_set_color(entity_reference& e, int r, int g, int b, int a);
	void             script_set_visible(entity_reference& e, bool pIs_visible);
	void             script_set_texture(entity_reference& e, const std::string& name);

	void             script_add_child(entity_reference& e1, entity_reference& e2);
	void             script_set_parent(entity_reference& e1, entity_reference& e2);
	void             script_detach_children(entity_reference& e);
	void             script_detach_parent(entity_reference& e);

	void             script_make_gui(entity_reference& e, float pOffset);
};

class background_music
{
public:
	background_music();
	void load_script_interface(script_system& pScript);
	void clean();
	void set_root_directory(const std::string& pPath);

private:
	engine::sound_stream mStream;
	engine::fs::path mRoot_directory;
	engine::fs::path mPath;

	int script_music_open(const std::string& pName);
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
	// pPath is not a reference so cleanup doesn't cause issues.
	int load_scene(std::string pName);

	// Reload the currently loaded scene.
	int reload_scene();

	const std::string& get_path();
	const std::string& get_name();

	void load_script_interface(script_system& pScript);

	void set_resource_manager(engine::resource_manager& pResource_manager);

	// Loads global scene settings from game.xml file.
	void load_game_xml(tinyxml2::XMLElement* ele_root);

	player_character& get_player();

	void tick(controls &pControls);

	void focus_player(bool pFocus);

	void set_text_format(const text_format_profile& pFormat);

private:
	std::map<std::string, script_context> pScript_contexts;

	panning_node mWorld_node;

	engine::resource_manager* mResource_manager;
	script_system*     mScript;

	tilemap_display       mTilemap_display;
	tilemap_loader        mTilemap_loader;
	collision_system      mCollision_system;
	entity_manager        mEntity_manager;
	background_music      mBackground_music;
	narrative_dialog      mNarrative;
	engine::sound_spawner mSound_FX;
	player_character      mPlayer;
	colored_overlay       mColored_overlay;
	pathfinding_system    mPathfinding_system;

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

	void refresh_renderer(engine::renderer& _r);
	void update_focus();
	void update_collision_interaction(controls &pControls);
};

// A basic save system.
// Saves player position, flags, and current scene path.
class save_system
{
public:
	save_system();

	bool open_save(const std::string& pPath);
	void load_flags(flag_container& pFlags);
	void load_player(player_character& pPlayer);
	std::string get_scene_path();
	std::string get_scene_name();

	void new_save();
	void save(const std::string& pPath);
	void save_flags(flag_container& pFlags);
	void save_scene(scene& pScene);
	void save_player(player_character& pPlayer);

private:
	tinyxml2::XMLDocument mDocument;
	tinyxml2::XMLElement *mEle_root;
};

class game_service
{
public:
	const text_format_profile& get_font_format() const;
	engine::fvector get_tile_size() const;

	int load_xml(const std::string& pPath);

private:
	text_format_profile mFont_format;
	engine::fvector mTile_size;
};

class game_settings_loader
{
public:
	bool load(const std::string& pPath);

	const std::string& get_start_scene() const;
	const std::string& get_textures_path() const;
	const std::string& get_sounds_path() const;
	const std::string& get_music_path() const;
	const text_format_profile& get_font_format() const;
	const std::string& get_player_texture() const;

private:
	std::string mStart_scene;
	std::string mTextures_path;
	std::string mSounds_path;
	std::string mMusic_path;
	text_format_profile mFont_format;

	std::string mPlayer_texture;

};


class scene_load_request
{
public:
	scene_load_request();

	void request_load(const std::string& pScene_name);
	bool is_requested() const;
	const std::string& get_scene_name() const;
	void complete();
private:
	bool mRequested;
	std::string mScene_name;
};

// The main game
class game :
	public engine::render_proxy
{
public:
	game();

	// Load the xml game settings
	int load_game_xml(std::string pPath);

	void tick();

protected:
	void refresh_renderer(engine::renderer& r);

private:
	scene            mScene;
	engine::resource_manager mResource_manager;
	flag_container   mFlags;
	script_system    mScript;
	controls         mControls;
	size_t           mSlot;

	text_format_profile mDefault_format; //TODO: create manager of formats

	editors::editor_manager mEditor_manager;

	scene_load_request mScene_load_request;

	engine::fs::path get_slot_path(size_t pSlot);
	void save_game();
	void open_game();
	bool is_slot_used(size_t pSlot);
	void set_slot(size_t pSlot);
	size_t get_slot();

	void script_load_scene(const std::string& pName);
	void load_script_interface();

	float get_delta();
};

}

#endif