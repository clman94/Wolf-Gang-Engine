#pragma once

#include <wge/logging/log.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/filesystem/exception.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/game_object.hpp>
#include <wge/core/scene.hpp>
#include <wge/core/component.hpp>
#include <wge/core/system.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/graphics/camera.hpp>

#include <sol/sol.hpp>

#include <variant>
#include <vector>
#include <memory>
#include <regex>

namespace wge::physics
{

class physics_world;

}

namespace wge::scripting
{

struct error_info
{
	std::string source;
	std::string message;
	int line = 0;

	bool operator==(const error_info& pOther) const noexcept
	{
		return source == pOther.source &&
			message == pOther.message && line == pOther.line;
	}

	bool operator!=(const error_info& pOther) const noexcept
	{
		return !operator==(pOther);
	}
};


class script :
	public core::resource
{
public:
	using handle = core::resource_handle<script>;

	std::string source;
	sol::protected_function function;
	std::optional<error_info> error;
	std::vector<std::pair<int, std::string>> function_list;

	bool has_errors() const noexcept
	{
		return error.has_value();
	}

	bool is_compiled() const noexcept
	{
		return function.valid() && !has_errors();
	}

	void parse_function_list()
	{
		function_list.clear();

		std::regex func_reg("function\\s+(.+)\\s*\\(");
		auto begin_iter = std::sregex_iterator(source.begin(), source.end(), func_reg);
		auto end_iter = std::sregex_iterator();
		int line = 1;
		for (auto i = begin_iter; i != end_iter; ++i)
		{
			for (auto c = i->prefix().first; c != i->prefix().second; c++)
				if (*c == '\n')
					++line;
			function_list.push_back({ line, (*i)[1] });
		}
	}

	virtual void load() override
	{
		const auto filepath = get_location().get_autonamed_file(".lua");
		try
		{
			std::ifstream stream(filepath.string().c_str());
			std::stringstream sstr;
			sstr << stream.rdbuf();
			source = sstr.str();
		}
		catch (const filesystem::io_error& e)
		{
			log::error("Couldn't load resource from path \"{}\"", filepath.string());
			log::error("Exception: {}", e.what());
		}
	}

	virtual void save() override
	{
		const auto filepath = get_location().get_autonamed_file(".lua");
		try
		{
			filesystem::file_stream stream;
			stream.open(filepath, filesystem::stream_access::write);
			stream.write(source);
		}
		catch (const filesystem::io_error& e)
		{
			log::error("Couldn't save resource to path \"{}\"", filepath.string());
			log::error("Exception: {}", e.what());
		}
	}
};

class event_component;

class lua_engine
{
public:
	// Create a new environment for individual objects.
	sol::environment create_object_environment(core::object pObj);
	// Update the delta. Do this before each layer.
	void update_delta(float pSeconds);

	bool compile_script(script& pScript);

	// Prepare state for new scene.
	void cleanup();

	sol::state state;

	void register_core_api();
	void register_asset_api(core::asset_manager& pAsset_manager);
	void register_layer_api(core::asset_manager& pAsset_manager);
	void register_graphics_api(graphics::camera& pDefault_camera);
	void register_math_api();
	void register_physics_api(physics::physics_world& pPhysics, core::scene& pScene);

	void update_layer(core::layer& pLayer, float pDelta);
	void draw_layer(core::layer& pLayer, float pDelta);

private:
	void run_script(event_component& pSource, const sol::environment& pEnv, const std::string& pEvent_name);
};

class event_state_component
{
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

class event_component
{
public:
	script::handle source_script;
	const std::string& get_source() const
	{
		WGE_ASSERT(source_script.is_valid());
		return source_script->source;
	}
};

namespace event_selector
{

using create = core::bselect<event_component, 0>;
using preupdate = core::bselect<event_component, 1>;
using update = core::bselect<event_component, 2>;
using postupdate = core::bselect<event_component, 3>;
using draw = core::bselect<event_component, 4>;

} // namespace event_selectors

std::string make_valid_identifier(std::string_view pStr, std::string_view pDefault = "Blank");

} // namespace wge::scripting
