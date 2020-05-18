#include <wge/core/asset.hpp>
#include <wge/core/game_object.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/math/transform.hpp>
#include <wge/math/vector.hpp>
#include <wge/graphics/render_batch_2d.hpp>
#include <wge/core/scene.hpp>
#include <wge/graphics/tileset.hpp>
#include <wge/core/tilemap.hpp>

#include <vector>
#include <variant>

namespace wge::core
{


class scene_resource :
	public resource
{
public:
	struct instance
	{
		// Unique name of instance.
		std::string name;
		math::transform transform;
		util::uuid asset_id;
	};

	struct instance_layer
	{
		std::string name;
		std::vector<instance> instances;
	};

	struct tilemap_layer
	{
		std::string name;
		std::vector<tile> tiles;
		math::vec2 tile_size;
		util::uuid tileset_id;
	};

	using layer_variant = std::variant<instance_layer, tilemap_layer>;
	std::vector<layer_variant> layers;

public:
	/* Generates a json with the following format */
	/*
	
	[
		{
			type: "tilemap",
			tiles: [ { position:..., uv:... } ],
			texture:...
		}
		{
			type: "instance",
			instances: [ { name:..., transform:..., asset:... } ]
		}
	]

	*/
	virtual json serialize_data() const override;
	virtual void deserialize_data(const json& pJson) override;

	scene generate_scene(const asset_manager& pAsset_mgr) const;

	void update_data(scene& pScene)
	{
		layers.clear();
		for (auto& layer : pScene)
		{
			if (core::is_tilemap_layer(layer))
				layers.push_back(read_tilemap(layer));
			else
				layers.push_back(read_instance_layer(layer));
		}
	}

	instance_layer read_instance_layer(layer& pLayer)
	{
		instance_layer inst_layer;
		inst_layer.name = pLayer.get_name();
		for (auto obj : pLayer)
		{
			instance& inst = inst_layer.instances.emplace_back();
			inst.asset_id = obj.get_asset()->get_id();
			inst.name = obj.get_name();
			if (auto transform = obj.get_component<math::transform>())
				inst.transform = *transform;
		}
		return inst_layer;
	}

	tilemap_layer read_tilemap(layer& pLayer)
	{
		tilemap_layer tilemap_layer;
		tilemap_layer.name = pLayer.get_name();
		tilemap_info const* info = pLayer.layer_components.get<tilemap_info>();
		tilemap_layer.tileset_id = info->tileset ? info->tileset.get_asset()->get_id() : util::uuid{};
		for (auto& [id, tile] : pLayer.each<tile>())
			tilemap_layer.tiles.push_back(tile);
		return tilemap_layer;
	}
};

} // namespace wge::core
