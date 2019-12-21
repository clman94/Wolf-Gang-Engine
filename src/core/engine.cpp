#include <wge/core/engine.hpp>

#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/graphics/renderer.hpp>

namespace wge::core
{

engine::engine() :
	mScene(mFactory)
{
	mFactory.register_system<graphics::renderer>();
	mFactory.register_system<physics::physics_world>();
	mFactory.register_system<scripting::script_system>(mLua_engine);

	mLua_engine.register_core_api();
	mLua_engine.register_asset_api(mAsset_manager);
	mLua_engine.register_layer_api(mAsset_manager);
	mLua_engine.register_math_api();
	mLua_engine.register_physics_api();

	mAsset_manager.register_resource_factory("texture",
		[&]()
	{
		auto res = std::make_unique<graphics::texture>();
		res->set_implementation(mGraphics.get_graphics_backend()->create_texture_impl());
		return res;
	});

	mAsset_manager.register_default_resource_factory<scripting::script>("script");
}

engine::~engine()
{
	// Clean up the scene first to prevent hanging references.
	mScene.clear();
}

void engine::create_game(const filesystem::path& pDirectory)
{
	mLoaded = false;
	mSettings.save_new(pDirectory);

	load_assets();

	// Everything was successful!
	mLoaded = true;
}

void engine::load_game(const filesystem::path& pPath)
{
	mLoaded = false;

	if (!mSettings.load(pPath))
	{
		log::error("Cannot find/parse game configuration from \"{}\"", pPath.string());
		return;
	}

	load_assets();

	// Everything was successful!
	mLoaded = true;
}

void engine::close_game()
{
	mLoaded = false;

}

void engine::render_to(const graphics::framebuffer::ptr& pFrame_buffer, const math::vec2& pOffset, const math::vec2& pScale)
{
	mScene.for_each_system<graphics::renderer>(
		[&](layer&, graphics::renderer& pRenderer)
	{
		pRenderer.set_framebuffer(pFrame_buffer);
		pRenderer.set_render_view_to_framebuffer(pOffset, 1.f / pScale);
		pRenderer.render(mGraphics);
	});
}

void engine::step()
{
	float delta = 1.f / 60.f;

	mLua_engine.update_delta(delta);

	mScene.for_each_system<scripting::script_system>(
		[&](layer&, scripting::script_system& pSys)
	{
		pSys.preupdate(delta);
	});

	mScene.for_each_system<scripting::script_system>(
		[&](layer&, scripting::script_system& pSys)
	{
		pSys.update(delta);
	});

	mScene.for_each_system<scripting::script_system>(
		[&](layer&, scripting::script_system& pSys)
	{
		pSys.postupdate(delta);
	});
}

bool engine::is_loaded() const
{
	return mLoaded;
}

void engine::load_assets()
{
	mAsset_manager.set_root_directory(mSettings.get_asset_directory());
	mAsset_manager.load_assets();
}

} // namespace wge::core
