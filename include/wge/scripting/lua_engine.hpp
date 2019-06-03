#pragma once

#include <wge/logging/log.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/filesystem/exception.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/game_object.hpp>
#include <wge/core/layer.hpp>
#include <wge/core/component.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/core/system.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>

#include <sol/sol.hpp>

#include <variant>
#include <vector>
#include <memory>

namespace wge::scripting
{

class script :
	public core::resource
{
public:
	using handle = core::resource_handle<script>;

	std::string source;
	sol::protected_function function;

	bool is_compiled() const noexcept
	{
		return function.valid();
	}

	virtual void load(const filesystem::path& pDirectory, const std::string& pName) override
	{
		auto path = pDirectory / (pName + ".lua");
		try
		{
			std::ifstream stream(path.string().c_str());
			std::stringstream sstr;
			sstr << stream.rdbuf();
			source = sstr.str();
			mFile_path = path;
		}
		catch (const filesystem::io_error& e)
		{
			log::error() << e.what() << log::endm;
			log::error() << "Couldn't load resource" << log::endm;
		}
	}

	virtual void save() override
	{
		filesystem::file_stream stream;
		try
		{
			stream.open(mFile_path, filesystem::stream_access::write);
			stream.write(source);
		}
		catch (const filesystem::io_error& e)
		{
			log::error() << e.what() << log::endm;
			log::error() << "Couldn't save resource" << log::endm;
		}
	}

	void save(const filesystem::path& pPath)
	{
		mFile_path = pPath;
		save();
	}

	const filesystem::path& get_source_path() const noexcept
	{
		return mFile_path;
	}

	virtual void update_source_path(const filesystem::path& pDirectory, const std::string& pName) override
	{
		auto old_path = pDirectory / mFile_path.filename();
		auto new_path = pDirectory / (pName + ".lua");
		system_fs::rename(old_path, new_path);
		mFile_path = std::move(new_path);
	}

private:
	filesystem::path mFile_path;
};

class lua_engine
{
public:
	lua_engine();
	~lua_engine();

	// Clear all global variables and then execute all global scripts.
	void execute_global_scripts(core::asset_manager& mAsset_manager);
	// Create a new environment for individual objects.
	sol::environment create_object_environment(const core::game_object& pObj);
	// Update the delta. Do this before each layer.
	void update_delta(float pSeconds);

	bool compile_script(script& pScript);

	// This environment stores all global data from global scripts
	sol::environment global_environment;

	sol::state state;

private:
	void register_api();
	void register_math_api();
	void register_physics_api();
};

class event_state_component :
	public core::component
{
	WGE_COMPONENT("Event State", 9233);
public:
	using property_value = std::variant<int, float, math::vec2, std::string>;
	struct property
	{
		std::string name;
		property_value value{ 0 };
	};
	std::vector<property> properties;
	sol::environment environment;
};

class event_component :
	public core::component
{
public:
	script::handle source_script;
	std::string source;
};

namespace event_components
{

class on_update :
	public event_component
{
	WGE_COMPONENT("Event: Update", 9234);
};

class on_create :
	public event_component
{
	WGE_COMPONENT("Event: Create", 9235);
public:
	bool first_time{ true };
};

} // namespace event_components

class script_system :
	public core::system
{
	WGE_SYSTEM("Script", 8423);
public:
	script_system(core::layer& pLayer, lua_engine& pLua_engine) :
		core::system(pLayer),
		mLua_engine(pLua_engine)
	{}

	void update(float pDelta) override;

private:
	lua_engine& mLua_engine;
};

std::string make_valid_identifier(const std::string_view& pStr);

} // namespace wge::scripting
