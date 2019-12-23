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

void scene_resource::generate_layer(core::layer& pLayer, const core::asset_manager& pAsset_mgr) const
{
	auto renderer = pLayer.add_system<graphics::renderer>();
	renderer->set_pixel_size(0.01f);
	pLayer.add_system<scripting::script_system>();
	for (auto& i : instances)
		generate_instance(pLayer, pAsset_mgr, i);
}

void scene_resource::generate_scene(core::scene& pScene, const core::asset_manager& pAsset_mgr) const
{
	auto layer = pScene.add_layer("Game");
	generate_layer(*layer, pAsset_mgr);
}

} // namespace wge::core
