#include "rpg_interpreter.hpp"
#include "utility.hpp"

#include <cassert>

using namespace rpg::interpreter;

// ##########
// event
// ##########

size_t
event::get_op_count()
{
	return ops.size();
}

operation*
event::get_op(size_t index)
{
	return ops.at(index).get();
}

operation*
event::create_op(e_opcode op)
{
	switch (op)
	{
	case e_opcode::say:        ops.emplace_back(new OP_say()); break;
	case e_opcode::wait:       ops.emplace_back(new OP_wait()); break;
	case e_opcode::waitforkey: ops.emplace_back(new operation(e_opcode::waitforkey)); break;

	default: return nullptr;
	}

	auto &nop = ops.back();
	return nop.get();
}

int
event::load_xml_event(tinyxml2::XMLElement* e)
{
	assert(e != nullptr);

	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string cmd = ele->Name();

		if (cmd == "say")
		{
			create_op(e_opcode::say)->load_xml(ele);
			create_op(e_opcode::waitforkey)->load_xml(ele);
		}

		else if (cmd == "fsay")
		{
			create_op(e_opcode::say)->load_xml(ele);
		}
		
		else if (cmd == "nl")
		{
			auto op = create_op(e_opcode::say);
			op->load_xml(ele);
			OP_say* c_op = dynamic_cast<OP_say*>(op);
			c_op->append = true;
			c_op->text += "\n";
		}

		else if (cmd == "append")
		{
			auto op = create_op(e_opcode::say);
			op->load_xml(ele);
			OP_say* c_op = dynamic_cast<OP_say*>(op);
			c_op->append = true;
		}

		else if (cmd == "wait")
			create_op(e_opcode::wait)->load_xml(ele);

		else if (cmd == "waitforkey")
			create_op(e_opcode::waitforkey);

		else if (cmd == "narrative:hide")
			create_op(e_opcode::narrative_hide);

		else if (cmd == "scene:load")
			create_op(e_opcode::scene_load)->load_xml(ele);

		/*else if (cmd == "entity:move")
		{
			auto op = create_op(e_opcode::entity_move)->cast_to<OP_entity_move>();
			op->load_xml(ele);
			op->type = entity_action::entity_type::noncharacter;
		}

		else if (cmd == "character:move")
		{
			auto op = create_op(e_opcode::entity_move)->cast_to<OP_entity_move>();
			op->load_xml(ele);
			op->type = entity_action::entity_type::character;
		}

		else if (cmd == "player:move")
		{
			auto op = create_op(e_opcode::entity_move)->cast_to<OP_entity_move>();
			op->load_xml(ele);
			op->type = entity_action::entity_type::player;
		}*/

		else
			util::error("Invalid command '" + cmd + "'");

		ele = ele->NextSiblingElement();
	}
	return 0;
}

// ##########
// event_tracker
// ##########

event_tracker::event_tracker()
{
	job_start = true;
}

bool
event_tracker::is_start()
{
	return job_start;
}

void
event_tracker::next()
{
	if (events.size())
	{
		++events.back().c_job;
		job_start = true;
	}
}

void
event_tracker::wait()
{
	job_start = false;
}

void
event_tracker::wait_until(bool cond)
{
	if (cond)
		next();
	else
		wait();
}

operation*
event_tracker::get_current_op()
{
	if (!events.size()) return nullptr;
	while (events.back().c_job >= events.back().c_event->get_op_count())
	{
		events.pop_back();
		if (!events.size()) return nullptr;
		next();
	}
	return events.back().c_event->get_op(events.back().c_job);
}

void
event_tracker::call_event(event* e)
{
	events.emplace_back(e);
	job_start = true;
}

void
event_tracker::cancel_event()
{
	events.pop_back();
}

void
event_tracker::cancel_all()
{
	events.clear();
}

void
event_tracker::interrupt(event* e)
{
	cancel_all();
	call_event(e);
}

void
event_tracker::queue_event(event* e)
{
	events.emplace_front(e);
}

// ##########
// Operations Load XML
// ##########

int
OP_say::load_xml(tinyxml2::XMLElement * e)
{
	using namespace tinyxml2;

	if (auto _expression = e->Attribute("expression"))
		expression = _expression;

	if (auto _interval = e->IntAttribute("interval"))
		interval = _interval;
	else interval = defs::DEFAULT_DIALOG_SPEED;

	auto ele_text = e->FirstChild();
	while (ele_text)
	{
		if (auto t = ele_text->ToText())
			text += t->Value();
		if (auto ele = ele_text->ToElement())
			text += (std::string(ele->Name()) == "nl" ? "\n" : "");
		ele_text = ele_text->NextSibling();
	}
	return 0;
}

int
OP_wait::load_xml(tinyxml2::XMLElement * e)
{
	seconds = e->FloatAttribute("sec");
	return 0;
}

int
OP_flag_set::load_xml(tinyxml2::XMLElement * e)
{
	flag = util::safe_string(e->Attribute("name"));
	return 0;
}

int
OP_flag_unset::load_xml(tinyxml2::XMLElement * e)
{
	flag = util::safe_string(e->Attribute("name"));
	return 0;
}

int
OP_scene_load::load_xml(tinyxml2::XMLElement * e)
{
	path = util::safe_string(e->Attribute("path"));
	door = util::safe_string(e->Attribute("door"));
	return 0;
}

void
entity_action::load_entity_name(tinyxml2::XMLElement* e)
{
	entity_name = util::safe_string(e->Attribute("name"));
}

int
OP_entity_move::load_xml(tinyxml2::XMLElement * e)
{
	to.x = e->FloatAttribute("x") * 32;
	to.y = e->FloatAttribute("y") * 32;
	return 0;
}

int
OP_entity_setanimation::load_xml(tinyxml2::XMLElement * e)
{
	name = util::safe_string(e->Attribute("name"));
	return 0;
}

int 
OP_entity_setcyclegroup::load_xml(tinyxml2::XMLElement * e)
{
	return 0;
}
