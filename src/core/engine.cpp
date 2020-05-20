#include <wge/core/engine.hpp>

#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/graphics/renderer.hpp>

namespace wge::core
{

engine::engine()
{
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
	// Again, script assets contain references to lua objects that
	// need to be cleaned up beforehand.
	mAsset_manager.remove_all_assets();
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

void engine::step()
{
	float delta = 1.f / 60.f;

	for (auto& i : mScene.get_layer_container())
		mLua_engine.update_layer(i, delta);
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
