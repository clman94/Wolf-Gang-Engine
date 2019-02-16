#pragma once

#include <wge/logging/log.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/component.hpp>
#include <wge/math/anchor.hpp>
#include <wge/math/aabb.hpp>
#include <wge/math/vector.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/graphics/texture.hpp>

namespace wge::graphics
{

class renderer;

class sprite_component :
	public core::component
{
	WGE_COMPONENT("Sprite", 12409);
public:
	// Creates a batch
	void create_batch(core::transform_component& pTransform, renderer& pRenderer);

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
	void set_texture(const texture::ptr& pAsset) noexcept;
	// Get the current texture
	texture::ptr get_texture() const noexcept;

	virtual bool has_aabb() const override { return true; }
	virtual math::aabb get_screen_aabb() const override { return mSceen_aabb; };
	virtual math::aabb get_local_aabb() const override { return mLocal_aabb; };

protected:
	virtual json on_serialize(core::serialize_type) const override;
	virtual void on_deserialize(const core::game_object& pObj, const json& pJson) override;

private:
	math::aabb mSceen_aabb, mLocal_aabb;
	texture::ptr mTexture;
	math::vec2 mOffset, mAnchor{ math::anchor::topleft };
	util::uuid mAnimation_id;
};

} // namespace wge::graphics
