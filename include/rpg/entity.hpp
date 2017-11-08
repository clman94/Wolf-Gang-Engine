#ifndef RPG_ENTITY_HPP
#define RPG_ENTITY_HPP

#include <engine/renderer.hpp>
#include <engine/node.hpp>
#include <engine/utility.hpp>
#include <engine/AS_utility.hpp>

namespace rpg {

/// An object that represents a graphical object in the game.
class entity :
	public engine::render_object,
	public util::tracked_owner
{
public:
	entity();
	virtual ~entity() {}

	enum class type
	{
		other,
		sprite,
		text
	};

	virtual type get_type() const
	{
		return type::other;
	}

	/// Set the dynamically changing depth according to its
	/// Y position.
	void set_dynamic_depth(bool pIs_dynamic);
	
	void set_z(float pZ);
	float get_z() const;

	void set_parallax(float pAmount);

protected:
	/// Updates the depth of the entity to its Y position.
	/// Should be called if draw() is overridden by a subclass.
	void update_depth();

	engine::fvector calculate_offset() const;

private:
	float mParallax;
	bool mDynamic_depth;
	float mZ;
};
/// For referencing entities in scripts.
typedef util::tracking_ptr<entity> entity_reference;

}

namespace util {
template<>
struct AS_type_to_string<rpg::entity_reference> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "entity";
	}
};
}

#endif // !RPG_ENTITY_HPP
