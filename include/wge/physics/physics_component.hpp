#pragma once

#include <queue>

#include <wge/core/component.hpp>
#include <wge/core/object_node.hpp>
#include <wge/math/vector.hpp>

class b2Body;
class b2Fixture;
struct b2FixtureDef;
enum b2BodyType;


namespace wge::physics
{

class physics_world_component;

class physics_component :
	public core::component
{
	WGE_COMPONENT("Physics", 5431);
public:
	enum {
		type_rigidbody,
		type_static
	};

	physics_component(core::object_node* pObj);
	virtual ~physics_component();

	virtual json serialize() const override;
	virtual void deserialize(const json& pJson) override;

	void set_type(int pType);

	b2Fixture* create_fixture(const b2FixtureDef& pDef);

	// Queues the fixture to be destroyed
	void destroy_fixture(b2Fixture* pFixture);

private:
	void on_physics_update_bodies(physics_world_component* pComponent);
	void on_preupdate(float);
	void on_postupdate(float);

	// Get the box2d body type
	b2BodyType get_b2_type() const;

	void destroy_queued_fixtures();
	
	// Update the transform component to the
	// transform of the body.
	void update_object_transform();
	// Update the body transform with the transform of
	// the transform component.
	void update_body_transform();

private:
	std::vector<std::weak_ptr<core::object_node>> mObjects_with_collision;
	b2Body* mBody;
	physics_world_component* mPhysics_world;

	std::queue<b2Fixture*> mFixture_destruction_queue;

	int mType;
	friend class physics_world_component;
};

}