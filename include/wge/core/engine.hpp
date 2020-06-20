#pragma once

#include <wge/core/scene.hpp>
#include <wge/core/game_settings.hpp>
#include <wge/graphics/graphics.hpp>
#include <wge/scripting/script_engine.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/graphics/camera.hpp>

namespace wge::core
{

class engine
{
public:
	engine();
	~engine();

	void create_game(const filesystem::path& pDirectory);
	void load_game(const filesystem::path& pPath);
	void close_game();

	void step();

	bool is_loaded() const;

	core::game_settings& get_settings() noexcept
	{
		return mSettings;
	}

	core::asset_manager& get_asset_manager() noexcept
	{
		return mAsset_manager;
	}

	core::scene& get_scene() noexcept
	{
		return mScene;
	}

	graphics::graphics& get_graphics() noexcept
	{
		return mGraphics;
	}

	scripting::script_engine& get_script_engine() noexcept
	{
		return mLua_engine;
	}

	physics::physics_world& get_physics() noexcept
	{
		return mPhysics;
	}

	graphics::camera& get_default_camera() noexcept
	{
		return mDefault_camera;
	}

private:
	void load_assets();

private:
	core::game_settings mSettings;
	core::asset_manager mAsset_manager;
	core::scene mScene;
	graphics::graphics mGraphics;
	graphics::camera mDefault_camera;
	scripting::script_engine mLua_engine;
	physics::physics_world mPhysics;

	bool mLoaded{ false };
};

} // namespace wge::core
