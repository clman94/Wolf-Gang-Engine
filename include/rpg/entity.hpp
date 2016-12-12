#ifndef RPG_ENTITY_HPP
#define RPG_ENTITY_HPP

#include <engine/renderer.hpp>
#include <engine/node.hpp>
#include <engine/utility.hpp>

namespace rpg {

/// An object that represents a graphical object in the game.
class entity :
	public engine::render_object,
	public engine::node,
	public util::tracked_owner
{
public:
	entity() :
		dynamic_depth(false)
	{}
	virtual ~entity() {}

	enum class entity_type
	{
		other,
		sprite,
		text
	};

	virtual entity_type get_entity_type()
	{
		return entity_type::other;
	}

	/// Set the dynamically changing depth according to its
	/// Y position.
	void set_dynamic_depth(bool pIs_dynamic);

	void set_name(const std::string& pName);
	const std::string& get_name();

protected:
	/// Updates the depth of the entity to its Y position.
	/// Should be called if draw() is overridden by a subclass.
	void update_depth();

private:
	bool dynamic_depth;
	std::string mName;
};
/// For referencing entities in scripts.
typedef util::tracking_ptr<entity> entity_reference;

}
#endif // !RPG_ENTITY_HPP
