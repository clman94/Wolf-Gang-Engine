#pragma once

#include <wge/logging/log.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/filesystem/exception.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/object.hpp>
#include <wge/core/scene.hpp>
#include <wge/core/handle.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/graphics/camera.hpp>

#include <wge/scripting/script.hpp>
#include <wge/scripting/error.hpp>
#include <wge/scripting/events.hpp>

#include <sol/sol.hpp>

#include <variant>
#include <vector>
#include <memory>
#include <regex>
#include <map>
#include <set>

namespace wge::physics
{

class physics_world;

}

namespace wge::scripting
{

class script_engine
{
public:
	// Create a new environment for individual objects.
	sol::environment create_object_environment(core::object pObj, sol::environment pExisting = {});
	// Update the delta. Do this before each layer.
	void update_delta(float pSeconds);

	bool compile_script(const script::handle& pScript, const std::string& pName);

	// Prepare state for new scene.
	void cleanup();

	sol::state state;

	void register_core_api();
	void register_asset_api(core::asset_manager& pAsset_manager);
	void register_layer_api(core::asset_manager& pAsset_manager);
	void register_graphics_api(graphics::camera& pDefault_camera);
	void register_math_api();
	void register_physics_api(physics::physics_world& pPhysics, core::scene& pScene);

	void event_create(core::layer& pLayer);
	void event_unique_create(core::layer& pLayer);
	void event_preupdate(core::layer& pLayer);
	void event_update(core::layer& pLayer);
	void event_postupdate(core::layer& pLayer);
	void event_draw(core::layer& pLayer, float pDelta);

	const error_info* get_script_error(const util::uuid& pId) const
	{
		auto iter = mCompile_errors.find(pId);
		if (iter != mCompile_errors.end())
			return &iter->second;
		else
			return nullptr;
	}

	bool has_compile_error(const util::uuid& pId) const
	{
		return mCompile_errors.find(pId) != mCompile_errors.end();
	}

	void prepare_recompile(const script::handle& pScript)
	{
		pScript->function = {};
		mCompile_errors.erase(pScript.get_id());
	}

	bool has_object_error(const core::object_id& pId) const noexcept
	{
		return mObject_errors.count(pId) != 0;
	}

	auto& get_erroneous_objects() const noexcept
	{
		return mObject_errors;
	}

	auto& get_runtime_errors() const noexcept
	{
		return mRuntime_errors;
	}

	auto& get_compile_errors() const noexcept
	{
		return mCompile_errors;
	}

	void reset_object(const core::object& pObj) noexcept;

	void clear_errors()
	{
		mObject_errors.clear();
		mCompile_errors.clear();
		mRuntime_errors.clear();
	}

private:
	// Set of objects that failed to run due to either a compile or runtime error.
	// This will block running/compiling of scripts in an object.
	std::set<core::object_id> mObject_errors;
	// Compile-time errors.
	std::map<util::uuid, error_info> mCompile_errors;
	// Errors that happened while running a script.
	// Run-time errors have an association with an object and may fail multiple times
	// depending on the amount of objects in the scene that run the script.
	std::map<core::object_id, runtime_error_info> mRuntime_errors;

private:
	void run_script(script::handle& pSource, const sol::environment& pEnv, const std::string& pEvent_name, const core::object_id& pId);
};



std::string make_valid_identifier(std::string_view pStr, std::string_view pDefault = "Blank");

} // namespace wge::scripting
