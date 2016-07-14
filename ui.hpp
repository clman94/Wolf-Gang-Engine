#ifndef ENGINE_UI_HPP
#define ENGINE_UI_HPP

#include "renderer.hpp"

namespace engine{
namespace ui{

class ui_client;

class ui_instance
{
	std::vector<ui_client*> clients;
	int focus;
public:
	ui_instance();
	friend class ui_client;
};

class ui_client
{
	int client_index;
	ui_instance* instance;
public:
	ui_client();
	ui_client(ui_instance& a);
	void set_instance(ui_instance& a);
	void request_focus();
	void loose_focus();
	bool is_focused();
};

class button_area : 
	public node,
	public ui_client
{
	bool enable, hover, pressed;
	fvector size, sel;
public:
	button_area();
	void set_size(fvector s);
	const fvector& get_size();
	bool is_pressed();
	bool is_mouseover();
	void set_enable(bool enable);
	bool is_enabled();
	fvector get_mouse_position();
	void update(renderer &_r);
};

class button_simple :
	public button_area,
	public render_client
{
	rectangle_node rect;
public:
	virtual int draw(renderer &_r)
	{
		update(_r);
		if (is_enabled())
		{
			if (is_pressed())
			{
				rect.set_color({ 255, 255, 255 });
				request_focus();
			}
			else if (is_mouseover())
				rect.set_color({ 120, 120, 120 });
			else
				rect.set_color({ 100, 100, 100 });
		}
		else
			rect.set_color({ 50, 50, 50 });
		rect.set_size({ get_size().x, get_size().y });
		rect.set_position(get_position());
		rect.draw(_r);
		return 0;
	}
};

class input_box :
	public button_area,
	public render_client
{
	text_node text;
	std::string str, message;
	clock blinker_clock;
public:
	text_node& get_text_node(); // bleh
	void set_text(std::string s);
	void set_text(int n);
	void set_text(float n);
	const std::string& get_text();
	int get_int();
	float get_float();
	void set_message(std::string s);
	virtual int draw(renderer &_r);
};

}}

#endif