#ifndef RPG_COLLISION_BOX_HPP
#define RPG_COLLISION_BOX_HPP

#include "../../tinyxml2/tinyxml2.h"

#include <string>

#include <engine/rect.hpp>
#include <engine/utility.hpp>

#include <rpg/script_function.hpp>
#include <rpg/flag_container.hpp>


namespace rpg {

class wall_group
{
public:
	wall_group();

	void set_function(script_function& pFunction);
	bool call_function();

	void set_name(const std::string& pName);
	const std::string& get_name() const;

	void set_enabled(bool pEnabled);
	bool is_enabled() const;

private:
	std::string mName;
	bool mIs_enabled;
	script_function* mFunction;
};

// A basic collision box
class collision_box
{
public:
	enum class type
	{
		wall,
		trigger,
		button,
		door
	};

	collision_box();
	collision_box(engine::frect pRect);
	bool is_enabled() const;

	const engine::frect& get_region() const;
	void set_region(engine::frect pRegion);

	void set_wall_group(std::shared_ptr<wall_group> pWall_group);
	std::shared_ptr<wall_group> get_wall_group();

	virtual type get_type()
	{
		return type::wall;
	}

	virtual void generate_xml_attibutes(tinyxml2::XMLElement* pEle) const;

protected:
	engine::frect mRegion;
	std::weak_ptr<wall_group> mWall_group;
	void generate_basic_attributes(tinyxml2::XMLElement* pEle) const;
};

// A collisionbox that is activated once the player has walked over it.
class trigger : public collision_box
{
public:
	bool call_function();

	virtual type get_type()
	{
		return type::trigger;
	}
};

class button :
	public trigger
{
public:
	virtual type get_type()
	{
		return type::button;
	}
};

class door : public collision_box
{
public:
	void set_name(const std::string& pName);
	const std::string& get_name() const;

	void set_destination(const std::string& pName);
	const std::string& get_destination() const;

	void set_scene(const std::string& pName);
	const std::string& get_scene() const;

	void set_offset(engine::fvector pOffset);
	engine::fvector get_offset() const;

	virtual type get_type()
	{
		return type::door;
	}

	virtual void generate_xml_attibutes(tinyxml2::XMLElement* pEle) const;

private:
	std::string mName;
	std::string mScene;
	std::string mDestination;
	engine::fvector mOffset;
};

class collision_box_container
{
public:
	void clean();

	std::shared_ptr<wall_group>    get_group(const std::string& pName);
	std::shared_ptr<wall_group>    create_group(const std::string& pName);

	std::shared_ptr<collision_box> add_wall();
	std::shared_ptr<trigger>       add_trigger();
	std::shared_ptr<button>        add_button();
	std::shared_ptr<door>          add_door();

	std::shared_ptr<collision_box> add_collision_box(collision_box::type pType);

	std::vector<std::shared_ptr<collision_box>> collision(engine::frect pRect);
	std::vector<std::shared_ptr<collision_box>> collision(engine::fvector pPoint);
	std::vector<std::shared_ptr<collision_box>> collision(collision_box::type pType, engine::frect pRect);
	std::vector<std::shared_ptr<collision_box>> collision(collision_box::type pType, engine::fvector pPoint);

	std::shared_ptr<collision_box> first_collision(engine::frect pRect);
	std::shared_ptr<collision_box> first_collision(engine::fvector pPoint);
	std::shared_ptr<collision_box> first_collision(collision_box::type pType, engine::frect pRect);
	std::shared_ptr<collision_box> first_collision(collision_box::type pType, engine::fvector pPoint);

	bool load_xml(tinyxml2::XMLElement* pEle);
	bool generate_xml(tinyxml2::XMLDocument& pDocument, tinyxml2::XMLElement* pEle) const;

	bool remove_box(std::shared_ptr<collision_box> pBox);
	bool remove_box(size_t pIndex);

	const std::vector<std::shared_ptr<collision_box>>& get_boxes() const;

	size_t get_count() const;

private:
	std::vector<std::shared_ptr<wall_group>> mWall_groups;
	std::vector<std::shared_ptr<collision_box>> mBoxes;
};


}

#endif // !RPG_COLLISION_BOX_HPP