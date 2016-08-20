#define ENGINE_INTERNAL

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
	mSfml_text.setFont(pFont.sf_font);
}

void 
text_node::set_text(const std::string pText)
{
	mSfml_text.setString(pText);
	mString = pText;

	auto center = center_offset<float>({ mSfml_text.getLocalBounds().width, mSfml_text.getLocalBounds().height }, mAnchor);
	mSfml_text.setOrigin(center);
}

void
text_node::append_text(const std::string pText)
{
	set_text(get_text() + pText);
}

std::string
text_node::get_text()
{
	return mString;
}

int
text_node::draw(renderer &pR)
{
	mSfml_text.setPosition(get_exact_position());
	pR.get_sfml_window().draw(mSfml_text);
	return 0;
}

void
text_node::set_character_size(int pPixels)
{
	mSfml_text.setCharacterSize(pPixels);
}

void
text_node::set_anchor(engine::anchor pAnchor)
{
	mAnchor = pAnchor;
}

void
text_node::set_scale(float pScale)
{
	mSfml_text.setScale({ pScale, pScale });
}

void
text_node::copy_format(const text_node& pText_node)
{
	auto nfont = pText_node.mSfml_text.getFont();
	if(nfont) mSfml_text.setFont(*nfont);

	mSfml_text.setFillColor(pText_node.mSfml_text.getFillColor());
	mSfml_text.setCharacterSize(pText_node.mSfml_text.getCharacterSize());
	mSfml_text.setScale(pText_node.mSfml_text.getScale());
	mSfml_text.setStyle(pText_node.mSfml_text.getStyle());
}


void 
text_node::set_color(const color& pColor)
{
	mSfml_text.setFillColor(pColor);
}
