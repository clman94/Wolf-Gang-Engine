#include <wge/core/scene_resource.hpp>
#include <wge/core/object_resource.hpp>
#include <wge/core/layer.hpp>
#include <wge/graphics/renderer.hpp>
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

static void serialize_scene(json& pJson, scene& pScene)
{
	auto& layers = pJson["layers"];
	for (auto& l : pScene)
	{
		json this_layer;
		this_layer["name"] = l.get_name();
		if (core::is_tilemap_layer(l))
		{
			core::tilemap_manipulator mani(l);
			this_layer["type"] = "tilemap";
			this_layer["tile_size"] = mani.get_tilesize();
			this_layer["tileset"] = mani.get_tileset().get_id();
			auto& tiles = this_layer["tiles"];
			for (auto& [id, t] : l.each<tile>())
				tiles.push_back({
					{ "position", t.position },
					{ "uv" , t.uv }
					});
		}
		else
		{
			auto& instances = this_layer["instances"];
			for (auto& obj : l)
			{
				if (obj.get_asset())
				{
					auto creation_script = obj.get_component<scripting::event_selector::unique_create>();
					auto transform = obj.get_component<math::transform>();
					instances.push_back({
						{ "name", obj.get_name() },
						{ "id", obj.get_asset()->get_id() },
						{ "transform", *transform },
						{ "create_script", creation_script ? asset_id{ creation_script->source_script.get_id() } : asset_id{} }
						});
				}
			}
		}
		layers.push_back(std::move(this_layer));
	}
}

static void deserialize_scene(const json& pJson, scene& pScene, const core::asset_manager& pAsset_mgr)
{
	for (auto& l : pJson["layers"])
	{
		auto& dlayer = pScene.add_layer(l["name"].get<std::string>());
		if (l["type"] == "tileset")
		{
			tilemap_manipulator mani(dlayer);

			// Setup the tileset.
			if (auto tileset = pAsset_mgr.get_asset(l["tileset"].get<asset_id>()))
				mani.set_tileset(tileset);

			// Set the tiles.
			for (auto& i : pJson["tiles"])
			{
				tile t;
				t.position = i["position"].get<math::ivec2>();
				t.uv = i["uv"].get<math::ivec2>();
				mani.set_tile(t);
			}
		}
		else
		{
			auto& instances = l["instances"];
			for (auto& i : instances)
			{
				instantiation_options inst_opt;
				inst_opt.name = i["name"].get<std::string>();
				inst_opt.transform = i["transform"].get<math::transform>();
				inst_opt.instantiable_asset_id = i["asset_id"].get<asset_id>();
				inst_opt.creation_script_id = util::json_get_or<util::uuid>(pJson, "creation_script_id", util::uuid{});
				instantiate_asset(inst_opt, dlayer.add_object(), pAsset_mgr);
			}
		}
	}
}


json scene_resource::serialize_data() const
{
	/* result;
	auto& layer_list = result["layers"];
	for (auto& i : layers)
	{
		json this_layer;
		if (auto layer = std::get_if<tilemap_layer>(&i))
		{
			this_layer = tilemap_layer::serialize(*layer);
			this_layer["type"] = "tilemap";
		}
		else if (auto layer = std::get_if<instance_layer>(&i))
		{
			this_layer = instance_layer::serialize(*layer);
			this_layer["type"] = "instance";
		}
		layer_list.push_back(std::move(this_layer));
	}*/
	return scene_data;
}

void scene_resource::deserialize_data(const json& pJson)
{
	scene_data = pJson;
}

void scene_resource::generate_scene(scene& pScene, const core::asset_manager& pAsset_mgr) const
{
	deserialize_scene(scene_data, pScene, pAsset_mgr);
}

void scene_resource::update_data(scene& pScene)
{
	scene_data.clear();
	serialize_scene(scene_data, pScene);
}

} // namespace wge::core
