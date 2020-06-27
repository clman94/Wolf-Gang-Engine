#include <wge/core/scene_resource.hpp>
#include <wge/core/object_resource.hpp>
#include <wge/core/layer.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/scripting/script_engine.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/physics/physics_component.hpp>

namespace wge::core
{

json scene_resource::serialize_data() const
{
	json layer_list;
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
	}
	return layer_list;
}

void scene_resource::deserialize_data(const json& pJson)
{
	layers.clear();
	for (auto& i : pJson)
	{
		if (i["type"] == "tilemap")
			layers.push_back(tilemap_layer::deserialize(i));
		else if (i["type"] == "instance")
			layers.push_back(instance_layer::deserialize(i));
	}
}

void scene_resource::generate_scene(scene& pScene, const core::asset_manager& pAsset_mgr) const
{
	for (auto& i : layers)
	{
		// Generate each new layer.
		std::visit([&](const auto& v) {
			v.generate(pScene.add_layer(), pAsset_mgr);
		}, i);
	}
}

void scene_resource::update_data(scene& pScene)
{
	layers.clear();
	for (auto& i : pScene)
	{
		// Set the data type based on the layers structure.
		// In this case we are checking if it has any data structures
		// related to tilemaps.
		if (core::is_tilemap_layer(i))
			layers.push_back(tilemap_layer{});
		else
			layers.push_back(instance_layer{});

		// Read the layer's data.
		std::visit([&](auto& v) {
			v.from(i);
		}, layers.back());
	}
}

void instance::from(const object& pObject)
{
	name = pObject.get_name();
	asset_id = pObject.get_asset()->get_id();
	if (auto t = pObject.get_component<math::transform>())
		transform = *t;
}

void instance::generate(core::object pObject, const core::asset_manager& pAsset_mgr) const
{
	auto asset = pAsset_mgr.get_asset(asset_id);
	pObject.set_name(name);
	pObject.set_asset(asset);

	if (asset->get_type() == "object")
	{
		// Generate the object using the resources generator.
		auto object_resource = asset->get_resource<core::object_resource>();
		object_resource->generate_object(pObject, pAsset_mgr);
		// Setup the transform.
		if (auto t = pObject.get_component<math::transform>())
			*t = transform;
	}
	else if (asset->get_type() == "sprite")
	{
		pObject.add_component(transform);
		pObject.add_component(graphics::sprite_component{ asset });
		pObject.add_component(physics::physics_component{});
		pObject.add_component(physics::sprite_fixture{});
	}
}

json instance::serialize(const instance& pData)
{
	json result;
	result["name"] = pData.name;
	result["transform"] = pData.transform;
	result["asset_id"] = pData.asset_id;
	return result;
}

instance instance::deserialize(const json& pJson)
{
	instance inst;
	inst.name = pJson["name"];
	inst.transform = pJson["transform"];
	inst.asset_id = pJson["asset_id"];
	return inst;
}


void instance_layer::from(core::layer& pLayer)
{
	name = pLayer.get_name();
	for (auto obj : pLayer)
	{
		instances.emplace_back().from(obj);
	}
}
 
void instance_layer::generate(layer& pLayer, const core::asset_manager& pAsset_mgr) const
{
	pLayer.set_name(name);
	for (auto& i : instances)
	{
		i.generate(pLayer.add_object(), pAsset_mgr);
	}
}

json instance_layer::serialize(const instance_layer& pData)
{
	json instances;
	for (auto& i : pData.instances)
	{
		instances.push_back(instance::serialize(i));
	}
	json result;
	result["name"] = pData.name;
	result["instances"] = std::move(instances);
	return result;
}

instance_layer instance_layer::deserialize(const json& pJson)
{
	instance_layer data;
	for (auto& i : pJson["instances"])
	{
		data.instances.push_back(instance::deserialize(i));
	}
	data.name = pJson["name"];
	return data;
}

void tilemap_layer::from(core::layer& pLayer)
{
	name = pLayer.get_name();

	// Let's just handle the raw tilemap data for now.
	tilemap_info const* info = pLayer.layer_components.get<tilemap_info>();

	// Read the tileset id.
	tileset_id = info->tileset ? info->tileset.get_asset()->get_id() : util::uuid{};

	// Read the tiles.
	for (auto& [id, tile] : pLayer.each<tile>())
	{
		tiles.push_back(tile);
	}
}

void tilemap_layer::generate(layer& pLayer, const core::asset_manager& pAsset_mgr) const
{
	pLayer.set_name(name);

	// This will automatically generate the needed data structures
	// that is required for a tilemap.
	tilemap_manipulator tilemap(pLayer);

	// Setup the tileset.
	if (tileset_id.is_valid() && pAsset_mgr.has_asset(tileset_id))
	{
		auto asset = pAsset_mgr.get_asset(tileset_id);
		tilemap.set_tileset(asset);
	}
	
	// Apply all the tiles.
	for (auto& i : tiles)
	{
		tilemap.set_tile(i);
	}
}

json tilemap_layer::serialize(const tilemap_layer& pData)
{
	json tiles;
	for (auto& i : pData.tiles)
	{
		json tile;
		tile["position"] = i.position;
		tile["uv"] = i.uv;
		tiles.push_back(std::move(tile));
	}

	json result;
	result["name"] = pData.name;
	result["tiles"] = std::move(tiles);
	result["tileset"] = pData.tileset_id;
	return result;
}

tilemap_layer tilemap_layer::deserialize(const json& pJson)
{
	tilemap_layer data;
	for (auto& i : pJson["tiles"])
	{
		tile this_tile;
		this_tile.position = i["position"];
		this_tile.uv = i["uv"];
		data.tiles.push_back(this_tile);
	}
	data.name = pJson["name"];
	data.tileset_id = pJson["tileset"];
	return data;
}

} // namespace wge::core
