#pragma once

#include <wge/core/context.hpp>
#include <wge/core/game_settings.hpp>
#include <wge/graphics/graphics.hpp>
#include <wge/scripting/lua_engine.hpp>

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

	void render_to(const graphics::framebuffer::ptr& pFrame_buffer, const math::vec2& pOffset, const math::vec2& pScale);
	
	bool is_loaded() const;

	core::game_settings& get_settings() noexcept
	{
		return mSettings;
	}

	core::asset_manager& get_asset_manager() noexcept
	{
		return mAsset_manager;
	}

	core::context& get_context() noexcept
	{
		return mGame_context;
	}

	graphics::graphics& get_graphics() noexcept
	{
		return mGraphics;
	}

	scripting::lua_engine& get_script_engine() noexcept
	{
		return mLua_engine;
	}

	const factory& get_factory() const noexcept
	{
		return mFactory;
	}

private:
	void load_assets();

private:
	core::game_settings mSettings;
	core::asset_manager mAsset_manager;
	core::context mGame_context;
	core::factory mFactory;

	graphics::graphics mGraphics;

	scripting::lua_engine mLua_engine;

	bool mLoaded{ false };
};

} // namespace wge::core
