#pragma once

#include <wge/logging/log.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/component.hpp>
#include <wge/math/anchor.hpp>
#include <wge/math/aabb.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/transform.hpp>
#include <wge/graphics/texture.hpp>

namespace wge::graphics
{

class renderer;

class sprite_component
{
public:
	// Creates a batch
	void create_batch(math::transform& pTransform, renderer& pRenderer);

	// Set the offset of the image in pixels
	void set_offset(const math::vec2& pOffset) noexcept;
	// Get the offset of the image in pixels
	math::vec2 get_offset() const noexcept;

	// Set the anchor ratio. Defaults topleft (0, 0).
	void set_anchor(const math::vec2& pRatio) noexcept;
	math::vec2 get_anchor() const noexcept;

	bool set_animation(const std::string& pName) noexcept;
	bool set_animation(const util::uuid& pId) noexcept;

	// Set the texture from a texture pointer.
	void set_texture(const core::asset::ptr& pAsset) noexcept;
	// Get the current texture
	core::asset::ptr get_texture() const noexcept;

private:
	math::aabb mSceen_aabb, mLocal_aabb;
	texture::handle mTexture;
	math::vec2 mOffset, mAnchor{ math::anchor::topleft };
	util::uuid mAnimation_id;
};

} // namespace wge::graphics
