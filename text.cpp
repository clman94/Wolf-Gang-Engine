#include "renderer.hpp"

using namespace engine;

text_node::text_node()
{
	c_anchor = anchor::topleft;
}

int
font::load(std::string path)
{
	return sf_font.loadFromFile(path);
}

void
text_node::set_font(font& f)
{
	text.setFont(f.sf_font);
}

void 
text_node::set_text(const std::string s)
{
	text.setString(s);
	c_text = s;

	auto center = center_offset<float>({ text.getLocalBounds().width, text.getLocalBounds().height }, c_anchor);
	text.setOrigin(sf::Vector2f(center.x, center.y));
}

void
text_node::append_text(const std::string s)
{
	set_text(get_text() + s);
}

std::string
text_node::get_text()
{
	return c_text;
}

int
text_node::draw(renderer &_r)
{
	fvector loc = get_position();
	text.setPosition({ (float)loc.x, (float)loc.y });
	_r.window.draw(text);
	return 0;
}

void
text_node::set_character_size(int s)
{
	text.setCharacterSize(s);
}

void
text_node::set_anchor(engine::anchor a)
{
	c_anchor = a;
}

void
text_node::set_scale(float a)
{
	text.setScale({ a, a });
}


void 
text_node::set_color(const color c)
{
	text.setColor(sf::Color(c.r, c.g, c.b, c.a));
}
