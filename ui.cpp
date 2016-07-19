#include "ui.hpp"
using namespace engine;
using namespace engine::ui;


// ###########
// ui_instance
// ###########

ui_instance::ui_instance()
{
	focus = -1;
}

// #########
// ui_client
// #########

ui_client::ui_client()
{
	instance = nullptr;
	client_index = 0;
}

ui_client::ui_client(ui_instance& a)
{
	set_instance(a);
}

void
ui_client::set_instance(ui_instance& a)
{
	instance = &a;
	client_index = instance->clients.size();
	instance->clients.push_back(this);
}
void
ui_client::request_focus()
{
	if (instance)
		instance->focus = client_index;
}

void
ui_client::loose_focus()
{
	if (is_focused())
		instance->focus = -1;
}

bool
ui_client::is_focused()
{
	if (!instance)
		return false; // Defaults
	return instance->focus == client_index;
}

// ###########
// button_area
// ###########

button_area::button_area()
{
	enable = true;
	pressed = false;
	hover = false;
}

void
button_area::set_size(fvector s)
{
	size = s;
}

const fvector&
button_area::get_size()
{
	return size;
}

bool
button_area::is_pressed()
{
	return pressed;
}

bool
button_area::is_mouseover()
{
	return hover;
}

void
button_area::set_enable(bool enable)
{
	enable = true;
}

bool
button_area::is_enabled()
{
	return enable;
}

fvector
button_area::get_mouse_position()
{
	return sel;
}

void
button_area::update(renderer &_r)
{
	if (enable)
	{
		auto mouse_pos = _r.get_mouse_position();
		auto pos = get_position();
		if (mouse_pos.x >= pos.x &&
			mouse_pos.y >= pos.y &&
			mouse_pos.x <= pos.x + size.x &&
			mouse_pos.y <= pos.y + size.y)
		{
			hover = true;
			pressed = _r.is_mouse_down(_r.mouse_left);
			sel = pos - mouse_pos;
		}
		else
		{
			hover = false;
			pressed = false;
		}
	}
	else
	{
		hover = false;
		pressed = false;
	}
}

// #########
// input_box
// #########

text_node&
input_box::get_text_node()
{
	return text;
}

void
input_box::set_text(std::string s)
{
	str = s;
}

void
input_box::set_text(int n)
{
	str = std::to_string(n);
}

void
input_box::set_text(float n)
{
	str = std::to_string(n);
}

const std::string&
input_box::get_text()
{
	return str;
}

void
input_box::set_message(std::string s)
{
	message = s;
}

int
input_box::get_int()
{
	return std::stoi(str);
}

float
input_box::get_float()
{
	return std::stof(str);
}

int
input_box::draw(renderer &_r)
{
	update(_r);
	if (is_pressed())
	{
		request_focus();
		_r.start_text_record(str);
	}
	if (is_focused())
	{
		bool is_cursor_visible = (blinker_clock.get_elapse().ms_i() % 1000) < 500;
		text.set_text(is_cursor_visible ? str + "<" : str);
		if (_r.is_key_down(renderer::key_type::Return))
			loose_focus();
		if (_r.is_key_down(renderer::key_type::LControl) &&
			_r.is_key_down(renderer::key_type::BackSpace))
			str.clear();
	}
	else
	{
		if (_r.is_text_recording(str))
			_r.end_text_record();
		text.set_text(message + ":[" + str + "]");
	}
	text.set_position(get_position());
	text.draw(_r);
	return 0;
}