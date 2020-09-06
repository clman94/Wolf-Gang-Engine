#include <wge/core/instance.hpp>

#include <wge/core/object_resource.hpp>
#include <wge/scripting/script_engine.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/scripting/events.hpp>

namespace wge::core
{

void instantiate_asset(const instantiation_options& pOptions,
	object pObject, const core::asset_manager& pAsset_mgr)
{
	assert(pObject.is_valid());
	assert(pOptions.instantiable_asset_id.is_valid());

	auto src_asset = pAsset_mgr.get_asset(pOptions.instantiable_asset_id);
	if (!src_asset)
	{
		log::error("Could not instantiate asset with id \"{}\"", pOptions.instantiable_asset_id.to_string());
		return;
	}

	pObject.set_name(pOptions.name);
	pObject.set_asset(src_asset);

	if (src_asset->get_type() == "object")
	{
		// Generate the object using the resource's generator.
		auto object_resource = src_asset->get_resource<core::object_resource>();
		object_resource->generate_object(pObject, pAsset_mgr);

		// Setup the create event.
		if (auto unique_create_script = pAsset_mgr.get_asset(pOptions.creation_script_id))
		{
			auto unique_create = pObject.add_component<scripting::event_selector::unique_create>();
			unique_create->source_script = unique_create_script;
		}

		// Setup the transform.
		if (auto t = pObject.get_component<math::transform>())
			*t = pOptions.transform;
	}
	else if (src_asset->get_type() == "sprite")
	{
		// Generate a basic sprite object
		pObject.add_component(pOptions.transform);
		pObject.add_component(graphics::sprite_component{ src_asset });
		pObject.add_component(physics::physics_component{});
		pObject.add_component(physics::sprite_fixture{});
	}
}

} // namespace wge::core
