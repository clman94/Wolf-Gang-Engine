
#include "renderer.hpp"

using namespace engine;

// ##########
// render_client
// ##########

int 
render_client::is_rendered()
{
	return client_index >= 0;
}

void
render_client::set_visible(bool a)
{
	visible = a;
}

bool
render_client::is_visible()
{
	return visible;
}

render_client::render_client()
{
	client_index = -1;
	set_visible(true);
	depth = 0;
	renderer_ = nullptr;
}

render_client::~render_client()
{
	renderer_->remove_client(this);
}

void
render_client::set_depth(depth_t d)
{
	depth = d;
	if (renderer_)
		renderer_->sort_clients();
}

float
render_client::get_depth()
{
	return depth;
}

render_client_wrapper::render_client_wrapper()
{
	ptr = nullptr;
}

int
render_client_wrapper::draw(renderer &_r)
{
	if (!ptr) return 0;
	return ptr->draw(_r);
}

void
render_client_wrapper::bind_client(render_client* client)
{
	ptr = client;
}

render_client*
render_client_wrapper::get_client()
{
	return ptr;
}

// ##########
// renderer
// ##########

renderer::renderer()
{
	events_update_sfml_window(window);
	text_record.multi_line = false;
	text_record.enable = false;
}

renderer::~renderer()
{
	for (auto i : clients)
		i->client_index = -1;
	close();
}

fvector
renderer::get_size()
{
	auto px = window.getView().getSize();
	auto scale = window.getView().getViewport();
	return
	{
		px.x / scale.width,
		px.y / scale.height
	};
}

int
renderer::initualize(ivector size, int fps)
{
	window.create(sf::VideoMode(size.x, size.y), "The Amazing Window", sf::Style::Titlebar | sf::Style::Close);
	window.setFramerateLimit(fps);
	return 0;
}


int
renderer::draw_clients()
{
	for (auto i : clients)
		if (i->is_visible())
			i->draw(*this);
	return 0;
}

int
renderer::draw()
{
	auto &c = background_color;
	window.clear(sf::Color(c.r, c.g, c.b, c.a));
	draw_clients();
	window.display();
	return 0;
}

void
renderer::start_text_record(bool multi_line)
{
	text_record.multi_line = multi_line;
	text_record.enable = true;
	text_record.text.clear();
}

void
renderer::start_text_record(std::string& ptr, bool multi_line)
{
	text_record.multi_line = multi_line;
	text_record.enable = true;
	text_record.ptr = &ptr;
}

bool
renderer::is_text_recording()
{
	return text_record.enable;
}

bool
renderer::is_text_recording(std::string& ptr)
{
	return text_record.ptr == &ptr;
}

void
renderer::end_text_record()
{
	text_record.enable = false;
	text_record.ptr = nullptr;
}
const std::string& 
renderer::get_recorded_text()
{
	return text_record.text;
}

void
renderer::refresh_clients()
{
	for (size_t i = 0; i < clients.size(); i++)
		clients[i]->client_index = i;
}

// Sort items that have a higher depth to be farther behind
void
renderer::sort_clients()
{
	struct
	{
		bool operator()(render_client* c1, render_client* c2)
		{
			return c1->depth > c2->depth;
		}
	}client_sort;
	std::sort(clients.begin(), clients.end(), client_sort);
	refresh_clients();
}

int
renderer::add_client(render_client* _client)
{
	if (_client->client_index >= 0)
	{
		return remove_client(_client);
	}
	_client->renderer_ = this;
	_client->client_index = clients.size();
	clients.push_back(_client);
	_client->refresh_renderer(*this);
	sort_clients();
	return 0;
}

int 
renderer::remove_client(render_client* _client)
{
	if (_client->client_index < 0
		|| _client->renderer_ != this) return 1;
	clients.erase(clients.begin() + _client->client_index);
	refresh_clients();
	_client->client_index = -1;
	_client->renderer_ = nullptr;
	return 0;
}

int 
renderer::close()
{
	window.close();
	return 0;
}

void
renderer::set_pixel_scale(float a)
{
	sf::View view = window.getDefaultView();
	sf::FloatRect fr = view.getViewport();
	fr.width  *= a;
	fr.height *= a;
	view.setViewport(fr);
	window.setView(view);
}

fvector
renderer::get_mouse_position()
{
	auto pos = sf::Mouse::getPosition(window);
	auto wpos = window.mapPixelToCoords(pos);
	return{ wpos.x, wpos.y };
}

fvector
renderer::get_mouse_position(fvector relative)
{
	return get_mouse_position() - relative;
}

bool
renderer::is_focused()
{
	return window.hasFocus();
}

void
renderer::set_visible(bool is_visible)
{
	window.setVisible(is_visible);
}

void
renderer::set_bg_color(color c)
{
	background_color = c;
}

void
events::refresh_pressed()
{
	for (auto &i : pressed_keys)
	{
		if (i.second == 1)
			i.second = -1;
	}

	for (auto &i : pressed_buttons)
	{
		if (i.second == 1)
			i.second = -1;
	}
}

bool
events::is_key_pressed(key_type k)
{
	if (!window->hasFocus())
		return false;
	bool is_pressed = sf::Keyboard::isKeyPressed(k);
	if (pressed_keys[k] == -1 && is_pressed)
		return false;
	pressed_keys[k] = is_pressed ? 1 : 0;
	return is_pressed;
}

bool
events::is_key_down(key_type k)
{
	if (!window->hasFocus())
		return false;
	return sf::Keyboard::isKeyPressed(k);
}

bool
events::is_mouse_pressed(mouse_button b)
{
	if (!window->hasFocus())
		return false;
	bool is_pressed = sf::Mouse::isButtonPressed((sf::Mouse::Button)b);
	if (pressed_buttons[b] == -1 && is_pressed)
		return false;
	pressed_buttons[b] = is_pressed ? 1 : 0;
	return is_pressed;
}

bool
events::is_mouse_down(mouse_button b)
{
	if (!window->hasFocus())
		return false;
	return sf::Mouse::isButtonPressed((sf::Mouse::Button)b);
}

void
events::events_update_sfml_window(sf::RenderWindow& w)
{
	window = &w;
}

int
events::update_events()
{
	refresh_pressed();

	if (!window->isOpen())
		return 1;

	while (window->pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
			return 1;

		/*if (text_record.enable && event.type == sf::Event::TextEntered)
		{
			std::string *text = &text_record.text;
			if (text_record.ptr)
				text = text_record.ptr;

			if (event.text.unicode == '\b')
			{
				if (text->size() != 0)
					text->pop_back();
			}
			else if (event.text.unicode == '\r')
			{
				if (text_record.multi_line)
					text->push_back('\n');
			}
			else if (event.text.unicode < 128)
				text->push_back(event.text.unicode);
		}*/
	}
	return 0;
}