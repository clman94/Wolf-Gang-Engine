#include <wge/core/engine.hpp>

#include <wge/core/transform_component.hpp>
#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/graphics/renderer.hpp>

namespace wge::core
{

engine::engine() :
	mGame_context(*this)
{
	mFactory.register_system<graphics::renderer>();
	mFactory.register_system<physics::physics_world>();
	mFactory.register_system<scripting::script_system>(mLua_engine);

	mFactory.register_component<core::transform_component>();
	mFactory.register_component<graphics::sprite_component>();
	mFactory.register_component<physics::physics_component>();
	mFactory.register_component<physics::box_collider_component>();
	mFactory.register_component<scripting::event_state_component>();
	mFactory.register_component<scripting::event_components::on_create>();
	mFactory.register_component<scripting::event_components::on_update>();

	mAsset_manager.register_resource_factory("texture",
		[&](core::asset::ptr& pAsset)
	{
		auto res = std::make_shared<graphics::texture>();
		res->set_implementation(mGraphics.get_graphics_backend()->create_texture_impl());
		filesystem::path path = pAsset->get_file_path();
		path.remove_extension();
		res->load(path.string());
		pAsset->set_resource(res);
	});

	mAsset_manager.register_resource_factory("script",
		[&](core::asset::ptr& pAsset)
	{
		auto res = std::make_shared<scripting::script>();
		filesystem::path path = pAsset->get_file_path();
		path.remove_extension();
		res->load(path);
		pAsset->set_resource(res);
	});
}

engine::~engine()
{
	// Clean up the scene first to prevent hanging references.
	mGame_context.clear();
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
		log::error() << "Cannot find/parse game configuration" << log::endm;
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
	for (auto& i : mGame_context.get_layer_container())
	{
		if (auto renderer = i->get_system<graphics::renderer>())
		{
			renderer->set_framebuffer(pFrame_buffer);
			renderer->set_render_view_to_framebuffer(pOffset, 1 / pScale);
			renderer->render(mGraphics);
		}
	}
}

bool engine::is_loaded() const
{
	return mLoaded;
}

void engine::load_assets()
{
	mAsset_manager.set_root_directory(mSettings.get_asset_directory());
	mAsset_manager.load_assets();
	mAsset_manager.import_all_with_ext(".png", "texture");
	mAsset_manager.import_all_with_ext(".lua", "script");
}

} // namespace wge::core
