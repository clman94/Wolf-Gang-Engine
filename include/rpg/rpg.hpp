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

#include <angelscript.h> // AS_USE_NAMESPACE will need to be defined
#include <angelscript/add_on/contextmgr/contextmgr.h>
#include <angelscript/add_on/scriptbuilder/scriptbuilder.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>

namespace AS = AngelScript;

namespace rpg{

class script_system;

// A node that acts as a sophisticated camera that can focus on a point.
class panning_node :
	public engine::node
{
public:
	panning_node();

	// Set the region in which the camera will always stay within
	void set_boundary(engine::frect pBoundary);
	engine::frect get_boundary();

	// Set the camera's resolution
	void set_viewport(engine::fvector pViewport);

	// Set the focal point in which the camera will center on
	void set_focus(engine::fvector pFocus);
	engine::fvector get_focus();

	void set_boundary_enable(bool pEnable);

private:
	engine::frect mBoundary;     ///< Region that the viewport will stay within
	engine::fvector mViewport;   ///< Size of the viewport
	engine::fvector mFocus;      ///< Point of focus (typically in the center of the screen)
	bool mBoundary_enabled;      ///< 
};

// A list template that connects all nodes contained to a central node
template<typename T>
class node_list :
	public engine::node
{
public:

	void clear()
	{
		mItems.clear();
	}

	template<class... T_ARG>
	auto& create_item(T_ARG&&... pArg)
	{
		mItems.emplace_back(std::forward<T_ARG>(pArg)...);
		add_child(mItems.back());
		return mItems.back();
	}

	auto& create_item()
	{
		mItems.emplace_back();
		add_child(mItems.back());
		return mItems.back();
	}

	bool remove_item(T* pPtr)
	{
		for (auto i = mItems.begin(); i != mItems.end(); i++)
		{
			if (&(*i) == pPtr)
			{
				mItems.erase(i);
				return true;
			}
		}
		return false;
	}

	size_t size()
	{
		return mItems.size();
	}

	auto begin() { return mItems.begin(); }
	auto end()   { return mItems.end();   }
	auto back()  { return mItems.back();  }

private:
	std::list<T> mItems;
};

// Contains all flags and an interface to them
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
		reset,
		menu,
		editor_1,
		editor_2,
	};
	controls();
	void trigger(control pControl);
	bool is_triggered(control pControl);
	void reset();

private:
	std::array<bool, 11> mControls;
};

// The basic dynamic object in game.
class entity :
	public engine::render_client,
	public engine::node,
	public util::named,
	public util::tracked_owner
{
public:

	entity();
	void play_animation();
	void stop_animation();
	void tick_animation();
	bool set_animation(const std::string& pName, bool pSwap = false);
	int draw(engine::renderer &pR);
	int load_entity(std::string pName, texture_manager& pTexture_manager);
	void set_dynamic_depth(bool a);
	void set_anchor(engine::anchor pAnchor);
	void set_color(engine::color pColor);
	void set_rotation(float pRotation);

	engine::fvector get_size()
	{ return mSprite.get_size(); }
	bool is_playing();

private:
	engine::texture*         mTexture;
	engine::animation_node   mSprite;

	bool dynamic_depth;
};
typedef util::tracking_ptr<entity> entity_reference;

// An entity that has a specific role as a character.
// Provides walk cycles.
class character :
	public entity
{
public:

	enum struct e_cycle
	{
		def, // "default" is apparently not allowed in gcc....
		left,
		right,
		up,
		down,
		idle
	};

	character();
	void set_cycle_group(std::string name);
	void set_cycle(const std::string& name);
	void set_cycle(e_cycle type);

	void  set_speed(float f);
	float get_speed();

private:
	std::string mCyclegroup;
	std::string mCycle;
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
	AS::asIScriptEngine *as_engine;
	AS::asIScriptFunction *func;
	AS::CContextMgr *ctx;
	AS::asIScriptContext *func_ctx;
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
	collision_box* wall_collision(const engine::frect& r);
	door*          door_collision(const engine::fvector& pPosition);
	trigger*       trigger_collision(const engine::fvector& pPosition);
	trigger*       button_collision(const engine::fvector& pPosition);

	engine::fvector get_door_entry(std::string pName);

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
class script_system
{
public:
	script_system();
	~script_system();

	// Load the scene angelscript file
	int load_scene_script(const std::string& pPath);

	// Register a member function
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr, void* pInstance);
	
	// Register a non-member/static function
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr);

	// Create custom triggers/buttons for trigger/button functions (data contained in metadata)
	void setup_triggers(collision_system& pCollision_system);

	void about_all();

	// Call all functions that contain the specific metadata
	// TODO: Cut off leading and trailing whitespace in all metadata before comparing
	void start_all_with_tag(const std::string& tag);

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
	AS::asIScriptEngine *mEngine;
	AS::CContextMgr      mCtxmgr;
	AS::asIScriptModule *mScene_module;
	AS::CScriptBuilder   mBuilder;
	std::ofstream        mLog_file;
	bool                 mExecuting;

	engine::timer mTimer;
	
	void dprint(std::string &msg);
	void register_vector_type();
	void message_callback(const AS::asSMessageInfo * msg);
	std::string get_metadata_type(const std::string &pMetadata);
	void script_abort();
	void script_create_thread(AS::asIScriptFunction *func, AS::CScriptDictionary *arg);
	void script_create_thread_noargs(AS::asIScriptFunction *func);
};

// Resource management of expression animations
class expression_manager
{
public:
	const engine::animation* find_animation(const std::string& mName);
	int load_expressions_xml(tinyxml2::XMLElement * pRoot, texture_manager& pTexture_manager);
	
private:
	std::map<std::string, const engine::animation*> mAnimations;
};

// The dialog object with text reveal
// TODO: Make more flexible with the ability to only have the text displayed,
//       move the text to any location, (possibly) automatically wrap text
//       without cutting off words, and lots more that might be useful.
class narrative_dialog :
	public engine::render_client
{
public:
	enum class position
	{
		top,
		bottom
	};

	narrative_dialog();

	void set_box_position(position pPosition);

	bool is_revealing();

	// Set the text to begin revealing
	void reveal_text(const std::string& pText, bool pAppend = false);

	// Set text without reveal
	void instant_text(std::string pText, bool pAppend = false);

	// Finish reveal before it is finished
	void skip_reveal();

	// Show the dialog box
	void show_box();

	// Hide the dialog box
	void hide_box();
	
	// Cleanup the current session
	void end_narrative();

	bool is_box_open();

	//void set_style_profile(const std::string& path);

	// Set duration between each character during reveal
	void set_interval(float ms);


	// The selection is just a simple text object, nothing really special.
	// TODO: Make special
	void show_selection();
	void hide_selection();
	void set_selection(const std::string& pText);

	void set_expression(const std::string& pName);

	int load_narrative_xml(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager);

	void load_script_interface(script_system& pScript);

	int draw(engine::renderer &pR);

protected:
	void refresh_renderer(engine::renderer& r);

private:
	engine::sprite_node    mBox;
	engine::sprite_node    mCursor;
	engine::text_node      mText;
	engine::text_node      mSelection;
	engine::font           mFont;
	engine::clock          mTimer;

	expression_manager     mExpression_manager;
	engine::animation_node mExpression;

	bool        mNew_character;
	bool        mRevealing;
	size_t      mCount;
	std::string mFull_text;
	float       mInterval;

	int load_box(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager);
	int load_font(tinyxml2::XMLElement* pEle);

	void show_expression();
	void reset_positions();

	bool script_has_displayed_new_character();
};

// The main player character
class player_character :
	public character
{
public:
	enum class direction
	{
		other,
		up,
		down,
		left,
		right
	};

	player_character();
	void set_locked(bool pLocked);
	bool is_locked();

	// Get Collision box in a specific durection based on the size of the frame
	engine::frect get_collision(direction pDirection);

	// Do movement with collision detection
	void movement(controls &pControls, collision_system& pCollision_system, float pDelta);
	
	// Get point in front of player
	engine::fvector get_activation_point(float pDistance = 18);

private:
	bool mLocked;
	direction mFacing_direction;
	void set_move_direction(engine::fvector pVec);
};

// Manager of entities in world
class entity_manager :
	public engine::render_proxy,
	public engine::node
{
public:
	entity_manager();

	character* find_character(const std::string& pName);
	entity*    find_entity(const std::string& pName);

	void clean();

	void load_script_interface(script_system& pScript);

	void set_texture_manager(texture_manager& pTexture_manager);

	int load_entities(tinyxml2::XMLElement* e);
	int load_characters(tinyxml2::XMLElement* e);

	bool is_character(entity* pEntity);

private:
	void register_entity_type(script_system& pScript);

	texture_manager*  mTexture_manager;

	script_system* mScript_system;

	node_list<character> mCharacters;
	node_list<entity>    mEntities;

	bool check_entity(entity_reference& e);

	entity_reference script_add_entity(const std::string& tex);
	entity_reference script_add_entity_atlas(const std::string& tex, const std::string& atlas);
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
	void             script_set_visibility(entity_reference& e, bool pIs_visible);

	void             script_add_child(entity_reference& e1, entity_reference& e2);
	void             script_set_parent(entity_reference& e1, entity_reference& e2);
	void             script_detach_children(entity_reference& e);
	void             script_detach_parent(entity_reference& e);
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

	bool script_find_path(AS::CScriptArray* pScript_path, engine::fvector pStart, engine::fvector pDestination);
	bool script_find_path_partial(AS::CScriptArray* pScript_path, engine::fvector pStart, engine::fvector pDestination, int pCount);
};

class scene :
	public engine::render_proxy,
	public panning_node
{
public:
	scene();
	~scene();
	collision_system& get_collision_system();

	// Cleanups the scene for a new scene.
	// Does not stop background music by default 
	// so it can be continued in the next scene.
	void clean_scene(bool pFull = false);

	// Load scene xml file which loads the scene script
	int load_scene(std::string pPath);

	// Reload the currently loaded scene.
	int reload_scene();

	const std::string& get_path()
	{ return mScene_path; }

	const std::string& get_name()
	{ return mScene_name; }

	void load_script_interface(script_system& pScript);
	void set_texture_manager(texture_manager& pTexture_manager);
	void load_game_xml(tinyxml2::XMLElement* ele_root);

	player_character& get_player()
	{ return mPlayer; }

	void tick(controls &pControls);

	void focus_player(bool pFocus);

private:
	texture_manager*  mTexture_manager;
	script_system*    mScript;

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

// TODO: Think of something better
class save_value_manager
{
public:
	void set_value(const std::string& pName, const std::string& pVal);
	void set_value(const std::string& pName, int pVal);
	void set_value(const std::string& pName, float pVal);

	const std::string& get_value_string(const std::string& pName);
	int                get_value_int(const std::string& pName);
	float              get_value_float(const std::string& pName);

	bool remove_value(const std::string& pName);

	void load_script_interface(script_system& pScript);

private:
	enum struct value_type
	{
		string,
		floating_point,
		integer
	};

	struct data_value
	{
		std::string val_string;
		float val_floating_point;
		int val_integer;
		value_type type;
	};
	std::map<std::string, data_value> mValues;
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

// The main game
class game :
	public engine::render_proxy,
	public util::nocopy
{
public:
	game();

	// Load the xml game settings
	int load_game_xml(std::string pPath);

	void tick(controls& pControls);

protected:
	void refresh_renderer(engine::renderer& r);

private:
	scene            mScene;
	texture_manager  mTexture_manager;
	flag_container   mFlags;
	script_system    mScript;
	controls         mControls;
	size_t           mSlot;

	save_value_manager mValue_manager;

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