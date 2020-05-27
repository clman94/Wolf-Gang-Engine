#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/layer.hpp>
#include <wge/graphics/tileset.hpp>
#include <wge/graphics/texture.hpp>
#include <wge/graphics/render_batch_2d.hpp>
#include <wge/math/vector.hpp>

namespace wge::core
{

struct tile
{
	math::ivec2 position, uv;
};

struct tilemap_info
{
	core::resource_handle<graphics::tileset> tileset;
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

	bool clear_tile(math::ivec2 pPosition, queue_destruction_flag)
	{
		if (object tile = find_tile(pPosition))
		{
			tile.destroy(queue_destruction);
			return true;
		}
		return false;
	}

	void set_tileset(core::resource_handle<graphics::tileset> pTileset)
	{
		mInfo->tileset = pTileset;
	}

	core::resource_handle<graphics::tileset> get_tileset() const
	{
		return mInfo->tileset;
	}

	math::ivec2 get_tilesize() const
	{
		assert(mInfo);
		assert(mInfo->tileset);
		return mInfo->tileset->tile_size;
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
		if (!mInfo->tileset)
			return;
		for (auto& [id, tile, quad_verts] : mLayer->each<tile, graphics::quad_vertices>())
			quad_verts.set_uv(get_uvrect(tile.uv));
	}

private:
	math::rect get_uvrect(math::ivec2 pUV) const
	{
		if (mInfo->tileset)
		{
			auto tile_uv_size = math::vec2(mInfo->tileset->tile_size) / math::vec2(mInfo->tileset->get_texture().get_size());
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

} // namespace wge::core
