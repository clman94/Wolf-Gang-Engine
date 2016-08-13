#include "renderer.hpp"

using namespace engine;

text_node::text_node()
{
	mAnchor = anchor::topleft;
}

int
font::load(std::string pPath)
{
	return sf_font.loadFromFile(pPath);
}

void
text_node::set_font(font& pFont)
{
	text.setFont(pFont.sf_font);
}

void 
text_node::set_text(const std::string pText)
{
	text.setString(pText);
	mText = pText;

	auto center = center_offset<float>({ text.getLocalBounds().width, text.getLocalBounds().height }, mAnchor);
	text.setOrigin(center);
}

void
text_node::append_text(const std::string pText)
{
	set_text(get_text() + pText);
}

std::string
text_node::get_text()
{
	return mText;
}

int
text_node::draw(renderer &pR)
{
	text.setPosition(get_exact_position());
	pR.mWindow.draw(text);
	return 0;
}

void
text_node::set_character_size(int pPixels)
{
	text.setCharacterSize(pPixels);
}

void
text_node::set_anchor(engine::anchor a)
{
	mAnchor = a;
}

void
text_node::set_scale(float a)
{
	text.setScale({ a, a });
}

void
text_node::copy_format(const text_node& node)
{
	auto nfont = node.text.getFont();
	if(nfont) text.setFont(*nfont);

	text.setColor(node.text.getColor());
	text.setCharacterSize(node.text.getCharacterSize());
	text.setScale(node.text.getScale());
	text.setStyle(node.text.getStyle());
}


void 
text_node::set_color(const color c)
{
	text.setColor(sf::Color(c.r, c.g, c.b, c.a));
}
