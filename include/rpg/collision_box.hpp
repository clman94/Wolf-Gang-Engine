#ifndef RPG_COLLISION_BOX_HPP
#define RPG_COLLISION_BOX_HPP

#include "../../3rdparty/tinyxml2/tinyxml2.h"

#include <string>

#include <engine/rect.hpp>
#include <engine/utility.hpp>

#include <rpg/script_function.hpp>
#include <rpg/flag_container.hpp>

#include <engine/AS_utility.hpp>

namespace rpg {

class wall_group
{
public:
	wall_group();

	void add_function(std::shared_ptr<script_function> pFunction);
	void call_function();

	void set_name(const std::string& pName);
	const std::string& get_name() const;

	void set_enabled(bool pEnabled);
	bool is_enabled() const;

private:
	std::string mName;
	bool mIs_enabled;
	std::vector<std::shared_ptr<script_function>> mFunctions;
};

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

	typedef std::shared_ptr<collision_box> ptr;

	collision_box();
	collision_box(engine::frect pRect);
	bool is_enabled() const;

	const engine::frect& get_region() const;
	void set_region(engine::frect pRegion);

	void set_wall_group(std::shared_ptr<wall_group> pWall_group);
	std::shared_ptr<wall_group> get_wall_group();

	void set_inverted(bool pIs_inverted);
	bool is_inverted() const;

	virtual type get_type()
	{
		return type::wall;
	}

	virtual void generate_xml_attibutes(tinyxml2::XMLElement* pEle) const;

	virtual void set(std::shared_ptr<collision_box> pBox);
	virtual std::shared_ptr<collision_box> copy();

protected:
	engine::frect mRegion;
	bool mInverted;
	std::weak_ptr<wall_group> mWall_group;
	void generate_basic_attributes(tinyxml2::XMLElement* pEle) const;
};

class trigger :
	public collision_box
{
public:
	bool call_function();

	virtual type get_type()
	{
		return type::trigger;
	}

	virtual void set(std::shared_ptr<collision_box> pBox) override;
	virtual std::shared_ptr<collision_box> copy() override;
};

class button :
	public trigger
{
public:
	virtual type get_type()
	{
		return type::button;
	}

	virtual void set(std::shared_ptr<collision_box> pBox) override; 
	virtual std::shared_ptr<collision_box> copy() override;
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


	engine::fvector calculate_player_position() const;

	virtual type get_type()
	{
		return type::door;
	}

	virtual void generate_xml_attibutes(tinyxml2::XMLElement* pEle) const;


	virtual void set(std::shared_ptr<collision_box> pBox) override;
	virtual std::shared_ptr<collision_box> copy() override;

private:
	std::string mName;
	std::string mScene;
	std::string mDestination;
	engine::fvector mOffset;
};

class collision_box_container
{
public:
	void clear();

	std::shared_ptr<wall_group>    get_group(const std::string& pName);
	std::shared_ptr<wall_group>    create_group(const std::string& pName);

	std::shared_ptr<collision_box> add_wall();
	std::shared_ptr<trigger>       add_trigger();
	std::shared_ptr<button>        add_button();
	std::shared_ptr<door>          add_door();
	std::shared_ptr<collision_box> add_collision_box(collision_box::type pType);
	std::shared_ptr<collision_box> add_collision_box(std::shared_ptr<collision_box> pBox);

	std::vector<std::shared_ptr<collision_box>> collision(const engine::frect& pRect) const;
	std::vector<std::shared_ptr<collision_box>> collision(const engine::fvector& pPoint) const;
	std::vector<std::shared_ptr<collision_box>> collision(collision_box::type pType, const engine::frect& pRect) const;
	std::vector<std::shared_ptr<collision_box>> collision(collision_box::type pType, const engine::fvector& pPoint) const;

	std::shared_ptr<collision_box> first_collision(const engine::frect& pRect) const;
	std::shared_ptr<collision_box> first_collision(const engine::fvector& pPoint) const;
	std::shared_ptr<collision_box> first_collision(collision_box::type pType, const engine::frect& pRect) const;
	std::shared_ptr<collision_box> first_collision(collision_box::type pType, const engine::fvector& pPoint) const;

	bool load_xml(tinyxml2::XMLElement* pEle);
	bool save_xml(tinyxml2::XMLDocument& pDocument, tinyxml2::XMLElement* pEle) const;

	bool remove_box(std::shared_ptr<collision_box> pBox);
	bool remove_box(size_t pIndex);

	const std::vector<std::shared_ptr<collision_box>>& get_boxes() const;

	size_t get_count() const;

	std::vector<std::shared_ptr<collision_box>>::iterator begin();
	std::vector<std::shared_ptr<collision_box>>::iterator end();


private:
	std::vector<std::shared_ptr<wall_group>> mWall_groups;
	std::vector<std::shared_ptr<collision_box>> mBoxes;
};


}

namespace util {
template<>
struct AS_type_to_string<rpg::collision_box> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "box";
	}
};
}

#endif // !RPG_COLLISION_BOX_HPP