#ifndef RPG_SCENE_HPP
#define RPG_SCENE_HPP

#include <engine/renderer.hpp>
#include <engine/audio.hpp>
#include <engine/pathfinding.hpp>
#include <engine/terminal.hpp>

#include <rpg/scene_loader.hpp>
#include <rpg/panning_node.hpp>
#include <rpg/collision_system.hpp>
#include <rpg/script_system.hpp>
#include <rpg/tilemap_manipulator.hpp>
#include <rpg/entity_manager.hpp>
#include <rpg/game_settings_loader.hpp>

namespace rpg {

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

	bool script_find_path(AS_array<engine::fvector>& pScript_path, engine::fvector pStart, engine::fvector pDestination);
	bool script_find_path_partial(AS_array<engine::fvector>& pScript_path, engine::fvector pStart, engine::fvector pDestination, int pCount);
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

class background_music
{
public:
	background_music();
	void load_script_interface(script_system& pScript);
	void clean();
	void set_resource_manager(engine::resource_manager& pResource_manager);
	void pause_music();
	void set_mixer(engine::mixer& pMixer);

private:

	engine::resource_manager* mResource_manager;

	std::unique_ptr<engine::sound> mStream;
	std::unique_ptr<engine::sound> mOverlap_stream;

	engine::fs::path mPath;
	engine::fs::path mOverlay_path;

	bool script_music_open(const std::string& pName);
	bool script_music_swap(const std::string& pName);
	int script_music_start_transition_play(const std::string& pName);
	void script_music_stop_transition_play();
	void script_music_set_second_volume(float pVolume);
};

class scene;

class scene_visualizer :
	public engine::render_object
{
public:
	scene_visualizer();
	void set_scene(scene& pScene);
	void visualize_entities(bool pVisualize);
	void visualize_collision(bool pVisualize);

	void load_terminal_interface(engine::terminal_system& pTerminal_system);

	int draw(engine::renderer& pR);
private:
	std::shared_ptr<engine::terminal_command_group> mGroup;

	void visualize_entities(engine::renderer& pR);
	void visualize_collision(engine::renderer& pR);

	bool mVisualize_collision;
	bool mVisualize_entities;

	engine::rectangle_node mEntity_center_visualize;
	engine::rectangle_node mEntity_visualize;
	engine::rectangle_node mBox_visualize;

	util::optional_pointer<scene> mScene;
};

class scene :
	public engine::render_proxy
{
public:

	scene();
	~scene();

	panning_node& get_world_node();
	engine::node& get_scene_node();

	collision_system& get_collision_system();

	// Cleans up the scene for a new scene.
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

	void set_resource_pack(engine::resource_pack* pPack);

	scene_visualizer& get_visualizer();

	bool is_ready() const;

	engine::mixer& get_mixer();

private:
	bool mIs_ready;

	std::vector<std::shared_ptr<script_function>> mEnd_functions;

	std::map<std::string, scene_script_context> pScript_contexts;

	panning_node mWorld_node;
	engine::node mScene_node;

	engine::resource_pack* mPack;
	engine::resource_manager* mResource_manager;
	script_system*            mScript;

	tilemap_display       mTilemap_display;
	tilemap_manipulator   mTilemap_manipulator;
	collision_system      mCollision_system;
	entity_manager        mEntity_manager;
	player_character      mPlayer;
	colored_overlay       mColored_overlay;
	pathfinding_system    mPathfinding_system;

	engine::mixer         mMixer;
	background_music      mBackground_music;
	engine::sound_spawner mSound_FX;

#ifndef LOCKED_RELEASE_MODE
	std::shared_ptr<engine::terminal_command_group> mTerminal_cmd_group;
	scene_visualizer mVisualizer;
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

	friend class scene_visualizer;
};


}

#endif // !RPG_SCENE_HPP
