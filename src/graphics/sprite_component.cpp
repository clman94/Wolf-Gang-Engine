#include <wge/graphics/sprite_component.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/object_node.hpp>
#include <wge/core/transform_component.hpp>

using namespace wge;
using namespace wge::graphics;

sprite_component::sprite_component(core::object_node * pNode) :
	component(pNode)
{
	subscribe_to(pNode, "on_render", &sprite_component::on_render, this);

	// Requirements
	require<core::transform_component>();
}

json sprite_component::serialize() const
{
	json result;
	result["offset"] = { mOffset.x, mOffset.y };
	result["texture"] = mTexture ? json(mTexture->get_id()) : json();
	return result;
}

void sprite_component::deserialize(const json & pJson)
{
	mOffset = math::vec2(pJson["offset"][0], pJson["offset"][1]);
	const json& texture_j = pJson["texture"];
	if (!texture_j.is_null())
		mTexture = get_asset_manager()->get_asset<texture>(static_cast<core::asset_uid>(texture_j));
}

void sprite_component::on_render(renderer * pRenderer)
{
	// No texture
	if (!mTexture)
		return;

	core::transform_component* transform = get_object()->get_component<core::transform_component>();
	// No transform component
	if (!transform)
		return;

	batch_builder batch;
	batch.set_texture(&(*mTexture));

	const math::vec2 texture_size(mTexture->get_width(), mTexture->get_height());

	vertex_2d verts[4];
	verts[0].position = math::vec2(0, 0);
	verts[0].uv = math::vec2(0, 0);
	verts[1].position = texture_size.swizzle(math::_x, 0);
	verts[1].uv = math::vec2(1, 0);
	verts[2].position = texture_size;
	verts[2].uv = math::vec2(1, 1);
	verts[3].position = texture_size.swizzle(0, math::_y);
	verts[3].uv = math::vec2(0, 1);

	// Get transform and scale it by the pixel size
	math::mat33 transform_mat = transform->get_absolute_transform();
	transform_mat.scale(math::vec2(pRenderer->get_pixel_size(), pRenderer->get_pixel_size()));

	// Transform the vertices
	for (int i = 0; i < 4; i++)
	{
		verts[i].position = transform_mat * (verts[i].position + (-mAnchor * texture_size) + mOffset);
		if (i == 0)
			mSceen_aabb = math::aabb{ verts[i].position, verts[i].position };
		else
			mSceen_aabb.merge(verts[i].position);
	}

	batch.add_quad(verts);

	pRenderer->push_batch(*batch.get_batch());
}

// Set the texture based on its asset id.
// This will use the asset manager in the current context.

void sprite_component::set_texture(core::asset_uid pID)
{
	mTexture = get_asset_manager()->get_asset<texture>(pID);
}

// Set the texture based on its asset path.
// This will use the asset manager in the current context.

void sprite_component::set_texture(const std::string & pPath)
{
	mTexture = get_asset_manager()->get_asset<texture>(pPath);
}
