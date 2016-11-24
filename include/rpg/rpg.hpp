#ifndef RPG_HPP
#define RPG_HPP

#include "../../tinyxml2/tinyxml2.h"

#include <engine/renderer.hpp>
#include <engine/utility.hpp>
#include <engine/particle_engine.hpp>
#include <engine/your_soul.hpp>
#include <engine/pathfinding.hpp>

#include <rpg/rpg_managers.hpp>
#include <rpg/rpg_config.hpp>
#include <rpg/tilemap_loader.hpp>
#include <rpg/editor.hpp>
#include <rpg/scene_loader.hpp>

#include <set>
#include <list>
#include <string>
#include <array>
#include <functional>
#include <map>
#include <fstream>
#include <memory>

#include <angelscript.h> // AS_USE_NAMESPACE will need to be defined
#include <angelscript/add_on/contextmgr/contextmgr.h>
#include <angelscript/add_on/scriptbuilder/scriptbuilder.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>
#include <angelscript/add_on/scripthandle/scripthandle.h>

namespace AS = AngelScript;

namespace rpg{


class script_system;

/// A node that acts as a sophisticated camera that can focus on a point.
class panning_node :
	public engine::node
{
public:
	panning_node();

	/// Set the region in which the camera will always stay within
	void set_boundary(engine::frect pBoundary);
	engine::frect get_boundary();

	/// Set the camera's resolution
	void set_viewport(engine::fvector pViewport);

	/// Set the focal point in which the camera will center on
	void set_focus(engine::fvector pFocus);
	engine::fvector get_focus();

	void set_boundary_enable(bool pEnable);

private:
	engine::frect mBoundary;
	engine::fvector mViewport;
	engine::fvector mFocus;
	bool mBoundary_enabled;
};

/// Contains all flags and an interface to them
class flag_container
{
public:
	bool set_flag(const std::string& pName);
	bool unset_flag(const std::string& pName);
	bool has_flag(const std::string& pName);
	void load_script_interface(script_system& pScript);
	void clean();

	auto begin()
	{ return mFlags.begin(); }

	auto end()
	{ return mFlags.end(); }

private:
	std::set<std::string> mFlags;
};

class controls
{
public:
	enum class control
	{
		activate,
		left,
		right,
		up,
		down,
		select_next,
		select_previous,
		select_up,
		select_down,
		back,
		reset,
		menu,
		editor_1,
		editor_2,
	};
	controls();
	void trigger(control pControl);
	bool is_triggered(control pControl);
	void reset();

	void update(engine::renderer& pR);

private:
	std::array<bool, 14> mControls;
};

/// An object that represents a graphical object in the game.
class entity :
	public engine::render_object,
	public engine::node,
	public util::tracked_owner
{
public:
	entity() :
		dynamic_depth(false)
	{}
	virtual ~entity(){}

	enum class entity_type
	{
		other,
		sprite,
		text
	};

	virtual entity_type get_entity_type()
	{ return entity_type::other; }

	/// Set the dynamically changing depth according to its
	/// Y position.
	void set_dynamic_depth(bool pIs_dynamic);

	void set_name(const std::string& pName);
	const std::string& get_name();

protected:
	/// Updates the depth of the entity to its Y position.
	/// Should be called if draw() is overridden by a subclass.
	void update_depth();

private:
	bool dynamic_depth;
	std::string mName;
};
/// For referencing entities in scripts.
typedef util::tracking_ptr<entity> entity_reference;

class sprite_entity :
	public entity
{
public:
	sprite_entity();
	void play_animation();
	void stop_animation();
	void tick_animation();
	bool set_animation(const std::string& pName, bool pSwap = false);
	int draw(engine::renderer &pR);
	int set_texture(std::string pName, texture_manager& pTexture_manager);
	void set_anchor(engine::anchor pAnchor);
	void set_color(engine::color pColor);
	void set_rotation(float pRotation);

	engine::fvector get_size() const;
	bool is_playing() const;

	virtual entity_type get_entity_type()
	{ return entity_type::sprite; }

private:
	util::optional_pointer<engine::texture> mTexture;
	engine::animation_node   mSprite;
};

// An sprite_entity that has a specific role as a character_entity.
// Provides walk cycles.
class character_entity :
	public sprite_entity
{
public:

	enum class cycle
	{
		def, // "default" is apparently not allowed in gcc....
		left,
		right,
		up,
		down,
		idle,
		idle_left,
		idle_right,
		idle_up,
		idle_down
	};

	enum class direction
	{
		other,
		left,
		right,
		up,
		down,
	};

	character_entity();
	void set_cycle_group(const std::string& name);
	void set_cycle(const std::string& name);
	void set_cycle(cycle type);

	void set_direction(direction pDirection);
	direction get_direction();

	void set_idle(bool pIs_idle);
	bool is_idle();

	void  set_speed(float f);
	float get_speed();

private:
	std::string mCyclegroup;
	std::string mCycle;
	direction mDirection;
	bool mIs_idle;
	float mMove_speed;
};

// A reference to a script function.
class script_function
{
public:
	script_function();
	~script_function();
	bool is_running();
	void set_engine(AS::asIScriptEngine * e);
	void set_function(AS::asIScriptFunction * f);
	void set_context_manager(AS::CContextMgr * cm);
	void set_arg(unsigned int index, void* ptr);
	bool call();

private:
	util::optional_pointer<AS::asIScriptEngine> as_engine;
	util::optional_pointer<AS::asIScriptFunction> func;
	util::optional_pointer<AS::CContextMgr> ctx;
	util::optional_pointer<AS::asIScriptContext> func_ctx;
	void return_context();
};

// A basic collision box
struct collision_box
{
public:
	collision_box();
	bool is_valid();
	void validate(flag_container & pFlags);
	void load_xml(tinyxml2::XMLElement* e);
	engine::frect get_region();
	void set_region(engine::frect pRegion);

protected:
	std::string mInvalid_on_flag;
	std::string mSpawn_flag;
	bool valid;
	engine::frect mRegion;
};

// A collisionbox that is activated once the player has walked over it.
struct trigger : public collision_box
{
public:
	script_function& get_function();
	void parse_function_metadata(const std::string& pMetadata);

private:
	script_function mFunc;
};

struct door : public collision_box
{
	std::string name;
	std::string scene_path;
	std::string destination;
	engine::fvector offset;
};

// A simple static collision system for world interactivity
class collision_system
{
public:
	util::optional_pointer<collision_box> wall_collision(const engine::frect& r);
	util::optional_pointer<door>          door_collision(const engine::fvector& pPosition);
	util::optional_pointer<trigger>       trigger_collision(const engine::fvector& pPosition);
	util::optional_pointer<trigger>       button_collision(const engine::fvector& pPosition);

	util::optional<engine::fvector> get_door_entry(std::string pName);

	void validate_all(flag_container& pFlags);

	void add_wall(engine::frect r);
	void add_trigger(trigger& t);
	void add_button(trigger& t);
	void clean();

	int load_collision_boxes(tinyxml2::XMLElement* pEle);

private:
	std::list<collision_box> mWalls;
	std::list<door> mDoors;
	std::list<trigger> mTriggers;
	std::list<trigger> mButtons;
};

// Angelscript wrapper
// Excuse this mess, please -_-
// TODO: Cleanup

class script_context;


class script_system
{
public:
	script_system();
	~script_system();

	void load_context(script_context& pContext);

	// Register a member function, will require the pointer to the instance
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr, void* pInstance);
	
	// Register a non-member/static function
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr);
	
	void about_all();

	// Call all functions that contain the specific metadata
	void start_all_with_tag(const std::string& pTag);

	// Execute all scripts
	int tick();

	int get_current_line();

	AS::asIScriptEngine& get_engine();

	bool is_executing();

	template<typename T>
	static void script_default_constructor(void *pMemory)
	{ new(pMemory) T(); }

	template<typename T, typename Targ1>
	static void script_constructor(Targ1 pArg1, void *pMemory)
	{ new(pMemory) T(pArg1); }

	template<typename T, typename Targ1, typename Targ2>
	static void script_constructor(Targ1 pArg1, Targ2 pArg2, void *pMemory)
	{ new(pMemory) T(pArg1, pArg2); }

	template<typename T>
	static void script_default_deconstructor(void *pMemory)
	{ ((T*)pMemory)->~T(); }

private:
	util::optional_pointer<AS::asIScriptEngine> mEngine;
	AS::CContextMgr      mCtxmgr;
	util::optional_pointer<script_context> mContext;
	std::ofstream        mLog_file;
	bool                 mExecuting;

	engine::timer mTimer;

	enum class log_entry_type
	{
		error,
		info,
		warning,
		debug
	};
	
	void log_print(const std::string& pFile, int pLine, int pCol
		, log_entry_type pType, const std::string& pMessage);

	void debug_print(std::string &pMessage);
	void error_print(std::string &pMessage);
	void register_vector_type();
	void message_callback(const AS::asSMessageInfo * msg);
	void script_abort();
	void script_create_thread(AS::asIScriptFunction *func, AS::CScriptDictionary *arg);
	void script_create_thread_noargs(AS::asIScriptFunction *func);

	std::map<std::string, AS::CScriptHandle> mShared_handles;

	void script_make_shared(AS::CScriptHandle pHandle, const std::string& pName);
	AS::CScriptHandle script_get_value(const std::string& pName);

	void load_script_interface();

	friend class script_context;
};

class script_context
{
public:
	script_context();

	void set_script_system(script_system& pScript);

	bool build_script(const std::string& pPath);

	bool is_valid();

	void clean();

	// Constructs all triggers/buttons defined by functions
	// with metadata "trigger" and "button"
	// TODO: Stablize parsing of metadata
	void setup_triggers(collision_system& pCollision_system);

private:
	util::optional_pointer<script_system> mScript;
	util::optional_pointer<AS::asIScriptModule> mScene_module;
	AS::CScriptBuilder   mBuilder;

	static std::string get_metadata_type(const std::string &pMetadata);

	friend class script_system;
};

// Resource management of expression animations
class expression_manager
{
public:
	util::optional_pointer<const engine::animation> find_animation(const std::string& mName);
	int load_expressions_xml(tinyxml2::XMLElement * pRoot, texture_manager& pTexture_manager);
	
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

	int load_narrative_xml(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager);

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

	int load_box(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager);

	void show_expression();
	void reset_positions();

	entity_reference script_get_narrative_box();
	entity_reference script_get_narrative_text();
};

// The main player character_entity
class player_character :
	public character_entity
{
public:

	player_character();
	void set_locked(bool pLocked);
	bool is_locked();

	// Do movement with collision detection
	void movement(controls &pControls, collision_system& pCollision_system, float pDelta);
	
	// Get point in front of player
	engine::fvector get_activation_point(float pDistance = 19);

private:
	bool mLocked;
	void set_move_direction(engine::fvector pVec);
};

class entity_manager :
	public engine::render_proxy,
	public engine::node
{
public:
	entity_manager();

	util::optional_pointer<entity>    find_entity(const std::string& pName);

	void clean();

	void load_script_interface(script_system& pScript);
	void set_texture_manager(texture_manager& pTexture_manager);
	bool is_character(sprite_entity* pEntity);

	template<typename T>
	T* construct_entity()
	{
		auto e = new T();
		assert(dynamic_cast<entity*>(e) != nullptr);
		mEntities.push_back(std::unique_ptr<entity>(dynamic_cast<entity*>(e)));
		add_child(*e);
		return e;
	}

	void set_text_format(const text_format_profile& pFormat);

private:
	void register_entity_type(script_system& pScript);

	texture_manager*  mTexture_manager;
	script_system* mScript_system;

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
	void load_script_interface(script_system& pScript);
	void clean();
private:
	engine::sound_stream mStream;
	std::string mPath;

	int script_music_open(const std::string& pPath);
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
	void clean_scene(bool pFull = false);

	// Load scene xml file which loads the scene script.
	// pPath is not a reference so cleanup doesn't cause issues.
	int load_scene(std::string pPath);

	// Reload the currently loaded scene.
	int reload_scene();

	const std::string& get_path();
	const std::string& get_name();

	void load_script_interface(script_system& pScript);

	void set_texture_manager(texture_manager& pTexture_manager);

	// Loads global scene settings from game.xml file.
	void load_game_xml(tinyxml2::XMLElement* ele_root);

	player_character& get_player();

	void tick(controls &pControls);

	void focus_player(bool pFocus);

	void set_text_format(const text_format_profile& pFormat);

private:
	std::map<std::string, script_context> pScript_contexts;

	panning_node mWorld_node;

	texture_manager*   mTexture_manager;
	script_system*     mScript;

	tilemap_display    mTilemap_display;
	tilemap_loader     mTilemap_loader;
	collision_system   mCollision_system;
	entity_manager     mEntity_manager;
	background_music   mBackground_music;
	narrative_dialog   mNarrative;
	sound_manager      mSound_FX;
	player_character   mPlayer;
	colored_overlay    mColored_overlay;
	pathfinding_system mPathfinding_system;

	scene_loader mLoader;

	bool              mFocus_player;

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

	std::string mScene_path;
	std::string mScene_name;

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
	texture_manager  mTexture_manager;
	flag_container   mFlags;
	script_system    mScript;
	controls         mControls;
	size_t           mSlot;

	text_format_profile mDefault_format; //TODO: create manager of formats

	bool        mRequest_load;
	std::string mNew_scene_path;
	
	editors::editor_manager mEditor_manager;

	std::string get_slot_path(size_t pSlot);
	void save_game();
	void open_game();
	bool is_slot_used(size_t pSlot);
	void set_slot(size_t pSlot);
	size_t get_slot();

	void script_load_scene(const std::string& pPath);
	void load_script_interface();

	float get_delta();
};

}

#endif