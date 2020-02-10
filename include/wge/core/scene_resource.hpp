#include <wge/core/asset.hpp>
#include <wge/core/game_object.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/math/transform.hpp>
#include <wge/math/vector.hpp>
#include <wge/graphics/render_batch_2d.hpp>
#include <wge/core/scene.hpp>
#include <wge/graphics/tileset.hpp>

#include <vector>
#include <variant>

namespace wge::core
{

struct tile
{
	math::ivec2 position, uv;
};

struct tilemap_info
{
	math::fvec2 tile_size{ 10, 10 };
	core::resource_handle<graphics::tileset> tileset;
	// This is the cached texture handle so we don't have to keep looking it up.
	core::resource_handle<graphics::texture> texture;
};

struct tile_animation
{
	float timer = 0;
	math::ivec2 start_frame;
	std::size_t frame_count;
	float interval;
};

struct tilemap_manipulator
{
public:
	tilemap_manipulator(layer& pLayer) :
		mLayer(&pLayer),
		mInfo(pLayer.layer_components.get<tilemap_info>())
	{
		if (!mInfo)
			mInfo = mLayer->layer_components.insert(tilemap_info{});
	}

	object find_tile(math::ivec2 pPosition)
	{
		for (auto& [id, tile] : mLayer->each<core::tile>())
			if (tile.position == pPosition)
				return mLayer->get_object(id);
		return object{};
	}

	void update_animations(float pDelta)
	{
		for (auto& [id, animation, tile, quad] :
			mLayer->each<tile_animation, tile, graphics::quad_vertices>())
		{
			animation.timer += pDelta;
		}
	}

	bool clear_tile(math::ivec2 pPosition)
	{
		if (object tile = find_tile(pPosition))
		{
			tile.destroy();
			return true;
		}
		return false;
	}

	void set_tileset(core::resource_handle<graphics::tileset> pTileset, const asset_manager& pAsset_manager)
	{
		mInfo->tileset = pTileset;
		mInfo->texture = pAsset_manager.get_asset(pTileset->texture_id);
	}

	core::resource_handle<graphics::tileset> get_tileset() const
	{
		return mInfo->tileset;
	}

	core::resource_handle<graphics::texture> get_texture() const
	{
		return mInfo->texture;
	}

	math::ivec2 get_tilesize() const
	{
		assert(mInfo);
		assert(mInfo->tileset);
		return mInfo->tileset->tile_size;
	}

	bool clear_tile(math::ivec2 pPosition, queue_destruction_flag)
	{
		if (object tile = find_tile(pPosition))
		{
			tile.destroy(queue_destruction);
			return true;
		}
		return false;
	}

	object set_tile(const tile& pTile)
	{
		if (object existing = find_tile(pTile.position))
		{
			// Update the uv of an existing tile.
			auto tile_comp = existing.get_component<tile>();
			tile_comp->uv = pTile.uv;
			auto quad_comp = existing.get_component<graphics::quad_vertices>();
			quad_comp->set_uv(get_uvrect(pTile.uv));
			return existing;
		}
		else
		{
			// Create a new object for the tile.
			object new_tile = mLayer->add_object();

			new_tile.add_component(pTile);

			graphics::quad_vertices quad_verts;
			quad_verts.set_rect(math::rect(math::vec2(pTile.position), math::vec2(1, 1)));
			quad_verts.set_uv(get_uvrect(pTile.uv));
			new_tile.add_component(quad_verts);
			new_tile.add_component(graphics::quad_indicies{});

			return new_tile;
		}
	}

	object set_tile(math::ivec2 pPosition, math::ivec2 pUV)
	{
		return set_tile(tile{ pPosition, pUV });
	}

	void update_tile_uvs()
	{
		if (!mInfo->texture)
			return;
		for (auto& [id, tile, quad_verts] : mLayer->each<tile, graphics::quad_vertices>())
			quad_verts.set_uv(get_uvrect(tile.uv));
	}

private:
	math::rect get_uvrect(math::ivec2 pUV) const
	{
		if (mInfo->texture)
		{
			auto tile_uv_size = mInfo->tile_size / mInfo->texture->get_size();
			return math::rect(math::vec2(pUV) * tile_uv_size, tile_uv_size);
		}
		else
			return math::rect{};
	}

private:
	tilemap_info* mInfo;
	layer* mLayer;
};

inline bool is_tilemap_layer(const layer& pLayer) noexcept
{
	return pLayer.layer_components.has<tilemap_info>();
}

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
		tilemap_info const* info = pLayer.layer_components.get<tilemap_info>();
		tilemap_layer.tileset_id = info->tileset ? info->tileset.get_asset()->get_id() : util::uuid{};
		for (auto& [id, tile] : pLayer.each<tile>())
			tilemap_layer.tiles.push_back(tile);
		return tilemap_layer;
	}
};

} // namespace wge::core
