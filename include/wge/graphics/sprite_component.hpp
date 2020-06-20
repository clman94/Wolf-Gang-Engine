#pragma once

#include <wge/logging/log.hpp>
#include <wge/core/asset.hpp>
#include <wge/math/anchor.hpp>
#include <wge/math/aabb.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/transform.hpp>
#include <wge/graphics/sprite.hpp>

namespace wge::graphics
{

class renderer;

class sprite_component
{
public:

	sprite_component() = default;
	sprite_component(sprite::handle pSprite) :
		mController(pSprite)
	{}

	// Creates a batch
	void create_batch(math::transform& pTransform, renderer& pRenderer);

	// Set the offset of the image in pixels
	void set_offset(const math::vec2& pOffset) noexcept;
	// Get the offset of the image in pixels
	math::vec2 get_offset() const noexcept;

	// Set the texture from a texture pointer.
	void set_sprite(const core::asset::ptr& pAsset) noexcept;
	// Get the current texture
	core::asset::ptr get_sprite() const noexcept;

	const math::aabb& get_local_aabb() const noexcept
	{
		return mLocal_aabb;
	}

	sprite_controller& get_controller() noexcept
	{
		return mController;
	}

	const sprite_controller& get_controller() const noexcept
	{
		return mController;
	}

private:
	sprite_controller mController;
	math::aabb mLocal_aabb;
	math::vec2 mOffset;
};

} // namespace wge::graphics
