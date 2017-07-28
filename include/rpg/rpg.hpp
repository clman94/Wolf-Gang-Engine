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

#include <rpg/script_system.hpp>
#include <rpg/collision_system.hpp>
#include <rpg/flag_container.hpp>
#include <rpg/panning_node.hpp>
#include <rpg/rpg_resource_directories.hpp>
#include <rpg/script_context.hpp>
#include <rpg/game_settings_loader.hpp>
#include <rpg/scene.hpp>

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
	bool load(engine::fs::path pData_dir);
	bool stop();
	void clear_scene();

	bool restart_game();

	bool tick();

	void load_terminal_interface(engine::terminal_system& pTerminal);

	scene& get_scene();
	engine::resource_manager& get_resource_manager();

	const engine::fs::path& get_source_path() const;

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

	float script_get_tile_size();

	void load_script_interface();

	void load_icon();
	void load_icon_pack();

	float get_delta();
};

}

#endif