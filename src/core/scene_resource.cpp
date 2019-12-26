#include <wge/core/scene_resource.hpp>
#include <wge/core/object_resource.hpp>
#include <wge/core/layer.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/scripting/lua_engine.hpp>
#include <wge/physics/physics_world.hpp>

namespace wge::core
{

json scene_resource::serialize_data() const
{
	json j;
	for (const instance& i : instances)
	{
		json j_inst;
		j_inst["name"] = i.name;
		j_inst["transform"] = i.transform;
		j_inst["id"] = i.id;
		j_inst["asset_id"] = i.asset_id;
		j["instances"].push_back(j_inst);
	}
	return j;
}

void scene_resource::deserialize_data(const json& pJson)
{
	for (const json& i : pJson["instances"])
	{
		instance inst;
		inst.name = i["name"];
		inst.transform = i["transform"];
		inst.id = i["id"];
		inst.asset_id = i["asset_id"];
		instances.emplace_back(std::move(inst));
	}
}

core::object scene_resource::generate_instance(core::layer& pLayer, const core::asset_manager& pAsset_mgr, const instance& pData)
{
	auto asset = pAsset_mgr.get_asset(pData.asset_id);
	auto object_resource = asset->get_resource<core::object_resource>();

	// Generate the object
	core::object obj = pLayer.add_object(pData.name);
	object_resource->generate_object(obj, pAsset_mgr);
	obj.set_asset(asset);

	if (auto transform = obj.get_component<math::transform>())
		*transform = pData.transform;
	return obj;
}

layer scene_resource::generate_layer(const core::asset_manager& pAsset_mgr) const
{
	layer new_layer;
	for (auto& i : instances)
		generate_instance(new_layer, pAsset_mgr, i);
	return new_layer;
}

scene scene_resource::generate_scene(const core::asset_manager& pAsset_mgr) const
{
	scene new_scene;
	new_scene.add_layer(generate_layer(pAsset_mgr));
	return new_scene;
}

} // namespace wge::core
