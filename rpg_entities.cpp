#include "rpg.hpp"

using namespace rpg;

/*
#include <boost/variant.hpp>
class entity_manager
	: engine::node
{

	struct entry
	{
		std::string name;
		boost::variant<
			engine::ptr_GC<engine::sprite_node>,
			engine::ptr_GC<engine::animated_sprite_node>,
			engine::ptr_GC<character >> n;
		int type;
	};
	std::vector<entry> nodes;

	engine::ptr_GC<engine::node> find_entity(std::string name, int type);
public:
	engine::ptr_GC<engine::sprite_node>          get_entity_sprite(std::string name);
	engine::ptr_GC<engine::animated_sprite_node> get_entity_animation(std::string name);
	engine::ptr_GC<character>                    get_entity_character(std::string name);
	engine::ptr_GC<engine::sprite_node>          create_sprite(std::string name);
	engine::ptr_GC<engine::animated_sprite_node> create_animation(std::string name);
	engine::ptr_GC<character>                    create_character(std::string name);
};*/

engine::ptr_GC<engine::node>
entity_manager::find_entity(std::string name, int type)
{
	for (auto &i : nodes)
	{
		if (i.type == type &&
			i.name == name)
			return i.n;
	}
	return nullptr;
}

engine::ptr_GC<engine::sprite_node>
entity_manager::get_entity_sprite(std::string name)
{
	return find_entity(name, entity_type::ENTITY_TYPE_SPRITE);
}

engine::ptr_GC<engine::animated_sprite_node>
entity_manager::get_entity_animation(std::string name)
{
	return find_entity(name, entity_type::ENTITY_TYPE_ANIMATION);
}

engine::ptr_GC<character>
entity_manager::get_entity_character(std::string name)
{
	return find_entity(name, entity_type::ENTITY_TYPE_CHARACTER);
}

engine::ptr_GC<engine::sprite_node>
entity_manager::create_sprite(std::string name)
{
	entry ne;
	ne.n = engine::allocate_ptr<engine::sprite_node>();
	ne.name = name;
	ne.type = ENTITY_TYPE_SPRITE;
	add_child(ne.n);
	nodes.push_back(ne);
	return ne.n;
}

engine::ptr_GC<engine::animated_sprite_node>
entity_manager::create_animation(std::string name)
{
	entry ne;
	ne.n = engine::allocate_ptr<engine::animated_sprite_node>();
	ne.name = name;
	ne.type = ENTITY_TYPE_ANIMATION;
	add_child(ne.n);
	nodes.push_back(ne);
	return ne.n;
}

engine::ptr_GC<character>
entity_manager::create_character(std::string name)
{
	entry ne;
	ne.n = engine::allocate_ptr<character>();
	ne.name = name;
	ne.type = ENTITY_TYPE_CHARACTER;
	add_child(ne.n);
	nodes.push_back(ne);
	return ne.n;
}