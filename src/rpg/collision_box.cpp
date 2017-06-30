#include <rpg/collision_box.hpp>
#include <engine/log.hpp>

using namespace rpg;


// ##########
// wall_group
// ##########

wall_group::wall_group()
{
	mIs_enabled = true;
}

void wall_group::add_function(std::shared_ptr<script_function> pFunction)
{
	mFunctions.push_back(pFunction);
}

void wall_group::call_function()
{
	for (auto i : mFunctions)
		i->call();
}

void wall_group::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string & wall_group::get_name() const
{
	return mName;
}

void wall_group::set_enabled(bool pEnabled) 
{
	mIs_enabled = pEnabled;
}

bool wall_group::is_enabled()const
{
	return mIs_enabled;
}

bool trigger::call_function()
{
	if (mWall_group.expired())
		return false;
	std::shared_ptr<wall_group>(mWall_group)->call_function();
	return true;
}

// ##########
// collision_box_container
// ##########

void collision_box_container::clean()
{
	mWall_groups.clear();
	mBoxes.clear();
}

std::shared_ptr<wall_group> collision_box_container::get_group(const std::string& pName)
{
	for (auto& i : mWall_groups)
		if (i->get_name() == pName)
			return i;
	return{};
}

std::shared_ptr<wall_group> collision_box_container::create_group(const std::string & pName)
{
	for (auto& i : mWall_groups)
		if (i->get_name() == pName)
			return i;
	std::shared_ptr<wall_group> new_wall_group(new wall_group);
	mWall_groups.push_back(new_wall_group);
	new_wall_group->set_name(pName);
	return new_wall_group;
}

std::shared_ptr<collision_box> collision_box_container::add_wall()
{
	std::shared_ptr<collision_box> box(new collision_box);
	mBoxes.push_back(box);
	return box;
}

std::shared_ptr<trigger> collision_box_container::add_trigger()
{
	std::shared_ptr<trigger> box(new trigger);
	mBoxes.push_back(box);
	return box;
}

std::shared_ptr<button> collision_box_container::add_button()
{
	std::shared_ptr<button> box(new button);
	mBoxes.push_back(box);
	return box;
}

std::shared_ptr<door> collision_box_container::add_door()
{
	std::shared_ptr<door> box(new door);
	mBoxes.push_back(box);
	return box;
}

std::shared_ptr<collision_box> rpg::collision_box_container::add_collision_box(collision_box::type pType)
{
	switch (pType)
	{
	case collision_box::type::wall:
		return add_wall();
	case collision_box::type::trigger:
		return add_trigger();
	case collision_box::type::button:
		return add_button();
	case collision_box::type::door:
		return add_door();
	}
	return{}; // Should never reach this point
}

std::shared_ptr<collision_box> collision_box_container::add_collision_box(std::shared_ptr<collision_box> pBox)
{
	mBoxes.push_back(pBox);
	return pBox;
}

std::vector<std::shared_ptr<collision_box>> collision_box_container::collision(engine::frect pRect)
{
	std::vector<std::shared_ptr<collision_box>> hits;
	for (auto& i : mBoxes)
		if (i->is_enabled()
			&& i->get_region().is_intersect(pRect))
			hits.push_back(i);
	return hits;
}

std::vector<std::shared_ptr<collision_box>> collision_box_container::collision(engine::fvector pPoint)
{
	std::vector<std::shared_ptr<collision_box>> hits;
	for (auto& i : mBoxes)
		if (i->is_enabled()
			&& i->get_region().is_intersect(pPoint))
			hits.push_back(i);
	return hits;
}

std::vector<std::shared_ptr<collision_box>> collision_box_container::collision(collision_box::type pType, engine::frect pRect)
{
	std::vector<std::shared_ptr<collision_box>> hits;
	for (auto& i : mBoxes)
		if (i->is_enabled()
			&& i->get_type() == pType
			&& i->get_region().is_intersect(pRect))
			hits.push_back(i);
	return hits;
}

std::vector<std::shared_ptr<collision_box>> collision_box_container::collision(collision_box::type pType, engine::fvector pPoint)
{
	std::vector<std::shared_ptr<collision_box>> hits;
	for (auto& i : mBoxes)
		if (i->is_enabled()
			&& i->get_type() == pType
			&& i->get_region().is_intersect(pPoint))
			hits.push_back(i);
	return hits;
}

std::shared_ptr<collision_box> rpg::collision_box_container::first_collision(engine::frect pRect)
{
	for (auto& i : mBoxes)
		if (i->is_enabled()
			&& i->get_region().is_intersect(pRect))
			return i;
	return{};
}

std::shared_ptr<collision_box> rpg::collision_box_container::first_collision(engine::fvector pPoint)
{
	for (auto& i : mBoxes)
		if (i->is_enabled()
			&& i->get_region().is_intersect(pPoint))
			return i;
	return{};
}

std::shared_ptr<collision_box> collision_box_container::first_collision(collision_box::type pType, engine::frect pRect)
{
	for (auto& i : mBoxes)
		if (i->is_enabled()
			&& i->get_type() == pType
			&& i->get_region().is_intersect(pRect))
			return i;
	return{};
}

std::shared_ptr<collision_box> rpg::collision_box_container::first_collision(collision_box::type pType, engine::fvector pPoint)
{
	for (auto& i : mBoxes)
		if (i->is_enabled()
			&& i->get_type() == pType
			&& i->get_region().is_intersect(pPoint))
			return i;
	return{};
}

bool collision_box_container::load_xml(tinyxml2::XMLElement * pEle)
{
	clean();
	auto ele_box = pEle->FirstChildElement();
	while (ele_box)
	{
		const std::string type = ele_box->Name();

		std::shared_ptr<collision_box> box;

		// Construct collision box based on type
		if (type == "wall")
		{
			box = add_wall();
		}
		else if (type == "trigger")
		{
			box = add_trigger();
		}
		else if (type == "button")
		{
			box = add_button();
		}
		else if (type == "door")
		{
			auto ndoor = add_door();
			ndoor->set_name        (util::safe_string(ele_box->Attribute("name")));
			ndoor->set_destination (util::safe_string(ele_box->Attribute("destination")));
			ndoor->set_scene       (util::safe_string(ele_box->Attribute("scene")));
			ndoor->set_offset      ({ele_box->FloatAttribute("offsetx")
				                   , ele_box->FloatAttribute("offsety") });
			box = ndoor;
		}
		else
		{
			logger::warning("Unknown collision box type '" + type + "'");
			ele_box = ele_box->NextSiblingElement();
			continue;
		}

		// Set the wall group (if any)
		const std::string group_name = util::safe_string(ele_box->Attribute("group"));
		if (!group_name.empty())
			box->set_wall_group(create_group(group_name));

		// Get rectangle region
		engine::frect rect;
		rect.x = ele_box->FloatAttribute("x");
		rect.y = ele_box->FloatAttribute("y");
		rect.w = ele_box->FloatAttribute("w");
		rect.h = ele_box->FloatAttribute("h");
		box->set_region(rect);

		ele_box = ele_box->NextSiblingElement();
	}
	return true;
}

bool collision_box_container::generate_xml(tinyxml2::XMLDocument & pDocument, tinyxml2::XMLElement * pEle) const
{
	pEle->DeleteChildren();
	for (auto& i : mBoxes)
	{
		std::string type_name;
		switch (i->get_type())
		{
		case collision_box::type::wall:
			type_name = "wall";
			break;
		case collision_box::type::trigger:
			type_name = "trigger";
			break;
		case collision_box::type::button:
			type_name = "button";
			break;
		case collision_box::type::door:
			type_name = "door";
			break;
		}

		auto ele_box = pDocument.NewElement(type_name.c_str());
		pEle->InsertEndChild(ele_box);
		i->generate_xml_attibutes(ele_box);
	}
	return false;
}

bool collision_box_container::remove_box(std::shared_ptr<collision_box> pBox)
{
	for (size_t i = 0; i < mBoxes.size(); i++)
		if (mBoxes[i] == pBox)
		{
			mBoxes.erase(mBoxes.begin() + i);
			return true;
		}
	return false;
}

bool collision_box_container::remove_box(size_t pIndex)
{
	mBoxes.erase(mBoxes.begin() + pIndex);
	return true;
}

const std::vector<std::shared_ptr<collision_box>>& collision_box_container::get_boxes() const
{
	return mBoxes;
}

size_t collision_box_container::get_count() const
{
	return mBoxes.size();
}

std::vector<std::shared_ptr<collision_box>>::iterator collision_box_container::begin()
{
	return mBoxes.begin();
}

std::vector<std::shared_ptr<collision_box>>::iterator collision_box_container::end()
{
	return mBoxes.end();
}


// ##########
// collision_box
// ##########

collision_box::collision_box() {}

collision_box::collision_box(engine::frect pRect)
{
	mRegion = pRect;
}

bool collision_box::is_enabled() const
{
	if (!mWall_group.expired())
		return std::shared_ptr<wall_group>(mWall_group)->is_enabled();
	return true;
}

const engine::frect& collision_box::get_region() const
{
	return mRegion;
}

void collision_box::set_region(engine::frect pRegion)
{
	mRegion = pRegion;
}

void collision_box::set_wall_group(std::shared_ptr<wall_group> pWall_group)
{
	mWall_group = pWall_group;
}

std::shared_ptr<wall_group> rpg::collision_box::get_wall_group()
{
	if (mWall_group.expired())
		return{};
	return std::shared_ptr<wall_group>(mWall_group);
}

void collision_box::generate_xml_attibutes(tinyxml2::XMLElement * pEle) const
{
	generate_basic_attributes(pEle);
}

void collision_box::generate_basic_attributes(tinyxml2::XMLElement * pEle) const
{
	pEle->SetAttribute("x", mRegion.x);
	pEle->SetAttribute("y", mRegion.y);
	pEle->SetAttribute("w", mRegion.w);
	pEle->SetAttribute("h", mRegion.h);
	if (!mWall_group.expired())
		pEle->SetAttribute("group", std::shared_ptr<wall_group>(mWall_group)->get_name().c_str());
}

// ##########
// door
// ##########

void door::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string & door::get_name() const
{
	return mName;
}

void door::set_destination(const std::string & pName)
{
	mDestination = pName;
}

const std::string & door::get_destination() const
{
	return mDestination;
}

void door::set_scene(const std::string & pName)
{
	mScene = pName;
}

const std::string & door::get_scene() const
{
	return mScene;
}

void door::set_offset(engine::fvector pOffset)
{
	mOffset = pOffset;
}

engine::fvector door::get_offset() const
{
	return mOffset;
}

engine::fvector door::calculate_player_position() const
{
	return mRegion.get_offset() + (mRegion.get_size()*0.5f) + mOffset;
}

void door::generate_xml_attibutes(tinyxml2::XMLElement * pEle) const
{
	generate_basic_attributes(pEle);
	pEle->SetAttribute("name", mName.c_str());
	pEle->SetAttribute("destination", mDestination.c_str());
	pEle->SetAttribute("scene", mScene.c_str());
	pEle->SetAttribute("offsetx", mOffset.x);
	pEle->SetAttribute("offsety", mOffset.y);
}
