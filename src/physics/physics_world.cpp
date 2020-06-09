#include <wge/physics/physics_world.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/box_collider_component.hpp>
#include <wge/math/transform.hpp>
#include <wge/graphics/sprite_component.hpp>

#include <wge/core/layer.hpp>

#include <Box2D/Box2D.h>

namespace wge::physics
{

physics_world::physics_world() :
	mWorld(std::make_unique<b2World>(b2Vec2(0, 1)))
{}

void physics_world::set_gravity(math::vec2 pVec)
{
	mWorld->SetGravity({ pVec.x, pVec.y });
}

math::vec2 physics_world::get_gravity() const
{
	b2Vec2 gravity = mWorld->GetGravity();
	return{ gravity.x, gravity.y };
}

b2World* physics_world::get_world() const
{
	return mWorld.get();
}

void physics_world::preupdate(core::layer& pLayer, float pDelta)
{
	// Create all the bodies
	for (auto [id, physics, transform] : pLayer.each<physics_component, math::transform>())
	{
		if (!physics.mBody)
		{
			b2BodyDef body_def;
			body_def.position.x = transform.position.x;
			body_def.position.y = transform.position.y;
			body_def.angle = transform.rotation;
			body_def.type = b2_dynamicBody;
			body_def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(id));
			physics.mBody = mWorld->CreateBody(&body_def);
			physics.mBody->ResetMassData();
		}
	}

	// Generate fixtures based on the sprites.
	for (auto [id, sprite_col_comp, physics_comp, sprite_comp, transform_comp] :
		pLayer.each<sprite_fixture, physics_component, graphics::sprite_component, math::transform>())
	{
		if ((!sprite_col_comp.mFixture || sprite_comp.get_controller().is_new_frame())
			&& physics_comp.mBody && sprite_comp.get_sprite())
		{
			// Create the shape around the sprite.
			const auto sprite = sprite_comp.get_sprite()->get_resource<graphics::sprite>();
			const auto frame_size = math::vec2(sprite->get_frame_size());
			const auto frame_anchor = math::vec2(sprite->get_frame_anchor(sprite_comp.get_controller().get_frame())) * transform_comp.scale;
			const auto sprite_size = frame_size * transform_comp.scale;
			b2PolygonShape shape;
			shape.SetAsBox(sprite_size.x, sprite_size.y, { frame_anchor.x, frame_anchor.y }, 0);

			// Setup the fixture settings.
			b2FixtureDef fixture_def;
			fixture_def.shape = &shape;
			fixture_def.density = 1;
			fixture_def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(id));

			// Remove the existing fixture
			if (sprite_col_comp.mFixture)
				physics_comp.mBody->DestroyFixture(sprite_col_comp.mFixture);

			// Create the new fixture.
			sprite_col_comp.mFixture = physics_comp.mBody->CreateFixture(&fixture_def);
		}
	}

	// Calculate physics
	mWorld->Step(pDelta, 1, 1);
	update_object_transforms(pLayer);
}

void physics_world::postupdate(core::layer& pLayer, float pDelta)
{
	// Update the body to the transform of the transform component
	for (auto [id, physics, transform] : pLayer.each<physics_component, math::transform>())
	{
		if (physics.mBody)
		{
			math::vec2 position = transform.position;
			math::radians rotation = transform.rotation;
			physics.mBody->SetTransform({ position.x, position.y }, rotation);
		}
	}

	// Update the shapes
	for (auto [id, collider, transform] : pLayer.each<box_collider_component, math::transform>())
	{
		// FIXME: Only update when the transform's scale actually changes.
		collider.update_current_shape(transform.scale);
	}
}

void physics_world::update_object_transforms(core::layer& pLayer)
{
	for (auto [id, physics, transform] : pLayer.each<physics_component, math::transform>())
	{
		if (physics.mBody)
		{
			b2Vec2 position = physics.mBody->GetPosition();
			transform.position = math::vec2{ position.x, position.y };
			transform.rotation = math::radians(physics.mBody->GetAngle());
		}
	}
}

} // namespacw wge::physics