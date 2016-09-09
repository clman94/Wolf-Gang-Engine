#ifndef RPG_HPP
#define RPG_HPP

#include "renderer.hpp"
#include "utility.hpp"
#include "rpg_managers.hpp"
#include "rpg_config.hpp"
#include "your_soul.hpp"
#include "tinyxml2\tinyxml2.h"
#include "tilemap_loader.hpp"
#include "particle_engine.hpp"
#include "editor.hpp"

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

namespace AS = AngelScript;

namespace rpg{

class script_system;

class panning_node :
	public engine::node
{
public:
	void set_boundary(engine::fvector pBoundary);
	void set_viewport(engine::fvector pViewport);
	void set_focus(engine::fvector pFocus);
	engine::fvector get_focus();
private:
	engine::fvector mBoundary, mViewport, mFocus;
};

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
	auto& add_item(T_ARG&&... pArg)
	{
		mItems.emplace_back(std::forward<T_ARG>(pArg)...);
		add_child(mItems.back());
		return mItems.back();
	}

	auto& add_item()
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

	auto begin() { return mItems.begin(); }
	auto end()   { return mItems.end();   }
	auto back()  { return mItems.back();  }

private:
	std::list<T> mItems;
};

class flag_container
{
public:
	bool set_flag(const std::string& pName);
	bool unset_flag(const std::string& pName);
	bool has_flag(const std::string& pName);
	void load_script_interface(script_system& pScript);

	auto begin()
	{ return flags.begin(); }

	auto end()
	{ return flags.end(); }

private:
	std::set<std::string> flags;
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
		menu
	};
	controls();
	void trigger(control pControl);
	bool is_triggered(control pControl);
	void reset();

private:
	std::array<bool, 9> mControls;
};

class entity :
	public engine::render_client,
	public engine::node,
	public   util::named
{
public:

	enum e_type
	{
		movement,
		constant,
		speech,
		user
	};
	entity();
	void play_withtype(e_type pType);
	void stop_withtype(e_type pType);
	void tick_withtype(e_type pType);
	bool set_animation(const std::string& pName, bool pSwap = false);
	int draw(engine::renderer &pR);
	util::error load_entity_xml(std::string path, texture_manager& tm);
	void set_dynamic_depth(bool a);
	void set_anchor(engine::anchor pAnchor);
	void set_color(engine::color pColor);
	void set_rotation(float pRotation);

	engine::fvector get_size()
	{ return mSprite.get_size(); }
	bool is_playing();

protected:
	util::error load_animations(tinyxml2::XMLElement* e, texture_manager& tm);

private:
	struct entity_animation :
		public util::named
	{
		const engine::animation* animation;
		int type;
	};

	typedef std::unordered_map<std::string, entity_animation> animation_map_t;

	engine::animation_node mSprite;
	animation_map_t mAnimations;
	animation_map_t::iterator mAnimation;

	bool dynamic_depth;
};

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
	std::string cyclegroup;
	std::string cycle;
	float move_speed;
};

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

struct collision_box
	: engine::frect
{
public:
	collision_box();
	bool is_valid();
	void validate(flag_container & pFlags);
	void load_xml(tinyxml2::XMLElement* e);

protected:
	std::string mInvalid_on_flag;
	std::string mSpawn_flag;
	bool valid;
};

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
	util::error load_collision_boxes(tinyxml2::XMLElement* pEle, flag_container& pFlags);

private:
	std::list<collision_box> mWalls;
	std::list<door> mDoors;
	std::list<trigger> mTriggers;
	std::list<trigger> mButtons;
};

// Excuse this mess -_-
class script_system
{
public:
	script_system();
	~script_system();
	util::error load_scene_script(const std::string& pPath);
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr, void* pInstance);
	void add_function(const char* pDeclaration, const AS::asSFuncPtr & pPtr);
	void add_pointer_type(const char* pName);
	void call_event_function(const std::string& pName);
	void setup_triggers(collision_system& pCollision_system);
	void about_all();
	void start_all_with_tag(const std::string& tag);
	int tick();

	bool is_executing();

private:
	AS::asIScriptEngine *mEngine;
	AS::CContextMgr      mCtxmgr;
	AS::asIScriptModule *mScene_module;
	AS::CScriptBuilder   mBuilder;
	std::ofstream        mLog_file;
	bool                 mExecuting;

	engine::timer mTimer;

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
	
	void dprint(std::string &msg);
	void register_vector_type();
	void message_callback(const AS::asSMessageInfo * msg);
	std::string get_metadata_type(const std::string &pMetadata);
	void script_abort();
};

class expression_manager
{
public:
	const engine::animation* find_animation(const std::string& mName);
	int load_expressions_xml(tinyxml2::XMLElement * pRoot, texture_manager& pTexture_manager);
	
private:
	std::map<std::string, const engine::animation*> mAnimations;
};

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

	void reveal_text(const std::string& pText, bool pAppend = false);
	void instant_text(std::string pText, bool pAppend = false);

	void show_box();
	void hide_box();
	bool is_box_open();

	//void set_style_profile(const std::string& path);

	void set_interval(float ms);

	void show_selection();
	void hide_selection();
	void set_selection(const std::string& pText);

	void set_expression(const std::string& pName);

	util::error load_narrative_xml(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager);

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

	bool        mRevealing;
	size_t      mCount;
	std::string mFull_text;
	float       mInterval;

	util::error load_box(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager);
	util::error load_font(tinyxml2::XMLElement* pEle);

	void show_expression();
	void reset_positions();
};

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
	engine::frect get_collision(direction pDirection);
	void movement(controls &pControls, collision_system& pCollision_system, float pDelta);
	engine::fvector get_activation_point(float pDistance = 18);

private:
	bool mLocked;
	direction mFacing_direction;
	void set_move_direction(engine::fvector pVec);
};

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

	util::error load_entities(tinyxml2::XMLElement* e);
	util::error load_characters(tinyxml2::XMLElement* e);

	bool is_character(entity* pEntity);

private:
	texture_manager*  mTexture_manager;

	node_list<character> mCharacters;
	node_list<entity>    mEntities;

	entity*         script_add_entity(const std::string& path);
	void            script_remove_entity(entity* e);
	entity*         script_add_character(const std::string& path);
	void            script_set_name(entity* e, const std::string& pName);
	void            script_set_position(entity* e, const engine::fvector& pos);
	engine::fvector script_get_position(entity* e);
	void            script_set_direction(entity* e, int dir);
	void            script_set_cycle(entity* e, const std::string& name);
	void            script_set_depth(entity* e, float pDepth);
	void            script_set_depth_fixed(entity* e, bool pFixed);
	void            script_start_animation(entity* e, int type);
	void            script_stop_animation(entity* e, int type);
	void            script_set_animation(entity* e, const std::string& name);
	void            script_set_anchor(entity* e, engine::anchor pAnchor);
	void            script_set_rotation(entity* e, float pRotation);
	void            script_set_color(entity* e, int r, int g, int b, int a);
	bool            script_validate_entity(entity* e);
};

class battle_system
{
	// TODO
};

// TODO
class particle_manager :
	public engine::render_proxy,
	public engine::node
{
public:
	void load_scene_emitters_xml(tinyxml2::XMLElement * pEle) {}
	void load_emitter_xml(tinyxml2::XMLElement * pEle);
	void clean() {}

private:
	std::list<engine::particle_emitter> mEmitters;
};

class background_music
{
public:
	void load_script_interface(script_system& pScript);
	void clean();
private:
	engine::sound_stream mStream;
};

class scene :
	public engine::render_proxy,
	public engine::node
{
public:
	scene();
	collision_system& get_collision_system();

	void clean_scene();
	util::error load_scene_xml(std::string pPath, script_system& pScript, flag_container& pFlags);
	util::error reload_scene(script_system& pScript, flag_container& pFlags);

	const std::string& get_path()
	{ return mScene_path; }

	const std::string& get_name()
	{ return mScene_name; }

	const engine::fvector& get_boundary()
	{ return mBoundary; }

	void load_script_interface(script_system& pScript);

	void set_texture_manager(texture_manager& pTexture_manager);

private:
	tilemap_display   mTilemap_display;
	tilemap_loader    mTilemap_loader;
	collision_system  mCollision_system;
	texture_manager*  mTexture_manager;
	script_system*    mScript;
	entity_manager    mEntity_manager;
	particle_manager  mParticle_system;
	background_music  mBackground_music;

	void script_set_tile(const std::string& pAtlas
		, engine::fvector pPosition, int pLayer, int pRotation);
	void script_remove_tile(engine::fvector pPosition, int pLayer);

	engine::fvector mBoundary;

	std::string mScene_path;
	std::string mScene_name;

protected:
	void refresh_renderer(engine::renderer& _r);
};

class save_system
{
	tinyxml2::XMLDocument mDocument;
	tinyxml2::XMLElement *mEle_root;
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
};

class game :
	public engine::render_proxy,
	public util::nocopy
{	
public:
	game();
	util::error load_game_xml(std::string pPath);
	void tick(controls& pControls);

protected:
	void refresh_renderer(engine::renderer& r);

private:
	panning_node     mRoot_node;
	scene            mScene;
	player_character mPlayer;
	texture_manager  mTexture_manager;
	flag_container   mFlags;
	narrative_dialog mNarrative;
	sound_manager    mSound_FX;
	script_system    mScript;
	controls         mControls;
	size_t           mSlot;

	bool        mRequest_load;
	std::string mNew_scene_path;
	
	editor::editor_gui mTest_gui;

	std::string get_slot_path(size_t pSlot);
	void save_game(size_t pSlot);
	void open_game(size_t pSlot);

	void player_scene_interact();

	entity* script_get_player();
	void load_script_interface();

	float get_delta();
};

}

#endif