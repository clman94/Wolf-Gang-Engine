#include <wge/core/scene_resource.hpp>
#include <wge/core/object_resource.hpp>
#include <wge/core/layer.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/scripting/lua_engine.hpp>
#include <wge/physics/physics_world.hpp>

namespace wge::core
{

static json serialize_instance_layer(const scene_resource::instance_layer& pLayer)
{
	json instances;
	for (auto& i : pLayer.instances)
	{
		json j_inst;
		j_inst["name"] = i.name;
		j_inst["transform"] = i.transform;
		j_inst["asset_id"] = i.asset_id;
		instances.push_back(std::move(j_inst));
	}
	json result;
	result["instances"] = std::move(instances);
	return result;
}

static json serialize_tilemap_layer(const scene_resource::tilemap_layer& pLayer)
{
	json tiles;
	for (auto& i : pLayer.tiles)
	{
		json tile;
		tile["position"] = i.position;
		tile["uv"] = i.uv;
		tiles.push_back(std::move(tile));
	}

	json result;
	result["tiles"] = std::move(tiles);
	result["texture"] = pLayer.tilemap_texture;
	return result;
}

json scene_resource::serialize_data() const
{
	json layer_list;
	for (auto& i : layers)
	{
		json this_layer;
		if (auto layer = std::get_if<tilemap_layer>(&i))
		{
			this_layer = serialize_tilemap_layer(*layer);
			this_layer["type"] = "tilemap";
		}
		else if (auto layer = std::get_if<instance_layer>(&i))
		{
			this_layer = serialize_instance_layer(*layer);
			this_layer["type"] = "instance";
		}
		layer_list.push_back(std::move(this_layer));
	}
	return layer_list;
}

static scene_resource::instance_layer deserialize_instance_layer(const json& pJson)
{
	scene_resource::instance_layer layer;
	for (auto& i : pJson["instances"])
	{
		scene_resource::instance inst;
		inst.name = i["name"];
		inst.transform = i["transform"];
		inst.asset_id = i["asset_id"];
		layer.instances.push_back(std::move(inst));
	}
	return layer;
}

static scene_resource::tilemap_layer deserialize_tilemap_layer(const json& pJson)
{
	scene_resource::tilemap_layer layer;
	for (auto& i : pJson["tiles"])
	{
		tile this_tile;
		this_tile.position = i["position"];
		this_tile.uv = i["uv"];
		layer.tiles.push_back(this_tile);
	}
	layer.tilemap_texture = pJson["texture"];
	return layer;
}

void scene_resource::deserialize_data(const json& pJson)
{
	layers.clear();
	for (auto& i : pJson)
	{
		if (i["type"] == "tilemap")
			layers.push_back(deserialize_tilemap_layer(i));
		else if (i["type"] == "instance")
			layers.push_back(deserialize_instance_layer(i));
	}
}

static void generate_instance(core::layer& pLayer, const scene_resource::instance& pData, const core::asset_manager& pAsset_mgr)
{
	auto asset = pAsset_mgr.get_asset(pData.asset_id);
	auto object_resource = asset->get_resource<core::object_resource>();

	// Generate the object
	core::object obj = pLayer.add_object(pData.name);
	object_resource->generate_object(obj, pAsset_mgr);
	obj.set_asset(asset);

	if (auto transform = obj.get_component<math::transform>())
		*transform = pData.transform;
	return;
}

static layer generate_instance_layer(const scene_resource::instance_layer& pLayer, const core::asset_manager& pAsset_mgr)
{
	layer new_layer;
	for (auto& i : pLayer.instances)
		generate_instance(new_layer, i, pAsset_mgr);
	return new_layer;
}

static layer generate_tilemap_layer(const scene_resource::tilemap_layer& pLayer, const core::asset_manager& pAsset_mgr)
{
	layer new_layer;
	tilemap_manipulator tilemap(new_layer);
	tilemap.set_texture(pAsset_mgr.get_asset(pLayer.tilemap_texture));
	for (auto& i : pLayer.tiles)
		tilemap.set_tile(i.position, i.uv);
	return new_layer;
}

scene scene_resource::generate_scene(const core::asset_manager& pAsset_mgr) const
{
	scene new_scene;

	for (auto& i : layers)
	{
		if (auto inst_layer = std::get_if<scene_resource::instance_layer>(&i))
			new_scene.add_layer(generate_instance_layer(*inst_layer, pAsset_mgr));
		else if (auto tm_layer = std::get_if<scene_resource::tilemap_layer>(&i))
			new_scene.add_layer(generate_tilemap_layer(*tm_layer, pAsset_mgr));
	}
	return new_scene;
}

} // namespace wge::core
