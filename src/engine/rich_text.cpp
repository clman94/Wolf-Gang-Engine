#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

#include "../../tinyxml2/tinyxml2.h"

using namespace engine;

text_format::text_format()
{
}

text_format::text_format(const char * pText)
{
	parse_text(pText);
}

text_format::text_format(const std::string & pText)
{
	parse_text(pText);
}

inline size_t parse_hex(const std::string& pHex)
{
	size_t val = 0;
	for (size_t i = 0; i < pHex.size(); i++)
	{
		const char c = std::tolower(pHex[pHex.size() - i - 1]);
		if (c >= '0' && c <= '9')
		{
			val += (c - '0') << (i * 4);
		}
		else if (c >= 'a' && c <= 'f')
		{
			val += (15 + (c - '0')) << (i * 4);
		}
	}
	return val;
}

color parse_xml_color(tinyxml2::XMLElement* pEle)
{
	auto hex = pEle->Attribute("hex");
	if (hex)
	{
		const std::string hex_string = util::safe_string(hex);
		if (hex_string.size() != 8)
			return{ 255, 255, 255, 255 };
		return
		{
			  static_cast<color_t>(parse_hex(hex_string.substr(0, 2)))
			, static_cast<color_t>(parse_hex(hex_string.substr(2, 2)))
			, static_cast<color_t>(parse_hex(hex_string.substr(4, 2)))
			, static_cast<color_t>(parse_hex(hex_string.substr(6, 2)))
		};
	}

	auto r = pEle->UnsignedAttribute("r");
	auto g = pEle->UnsignedAttribute("g");
	auto b = pEle->UnsignedAttribute("b");
	auto a = !pEle->Attribute("a") ? 255 : pEle->UnsignedAttribute("a"); // Default 255
	return{ 
		  static_cast<color_t>(r)
		, static_cast<color_t>(g)
		, static_cast<color_t>(b)
		, static_cast<color_t>(a)
	};
}

inline bool parse_xml_format(tinyxml2::XMLNode* pNode, std::vector<text_format::format>& pFormat_stack
	, color pColor, std::vector<text_format::block>& pBlocks)
{
	while (pNode)
	{
		if (auto xml_text = pNode->ToText())
		{
			const std::string text = util::safe_string(xml_text->Value());
			if (!text.empty())
			{
				text_format::block nblock;
				nblock.mColor = pColor;
				nblock.mFormat = 0;
				for (auto& i : pFormat_stack)
					nblock.mFormat |= static_cast<uint32_t>(i);
				nblock.mText = text;
				pBlocks.push_back(nblock);
			}
		}
		else if (auto ele = pNode->ToElement())
		{
			const std::string name = util::safe_string(ele->Name());
			if (name == "b")
			{
				pFormat_stack.push_back(text_format::format::bold);
				parse_xml_format(pNode->FirstChild(), pFormat_stack, pColor, pBlocks);
				pFormat_stack.pop_back();
			}
			else if (name == "i")
			{
				pFormat_stack.push_back(text_format::format::italics);
				parse_xml_format(pNode->FirstChild(), pFormat_stack, pColor, pBlocks);
				pFormat_stack.pop_back();
			}
			else if (name == "c")
			{
				color ncolor = parse_xml_color(ele);
				parse_xml_format(pNode->FirstChild(), pFormat_stack, ncolor, pBlocks);
			}
			else if (name == "wave")
			{
				pFormat_stack.push_back(text_format::format::wave);
				parse_xml_format(pNode->FirstChild(), pFormat_stack, pColor, pBlocks);
				pFormat_stack.pop_back();
			}
			else if (name == "shake")
			{
				pFormat_stack.push_back(text_format::format::shake);
				parse_xml_format(pNode->FirstChild(), pFormat_stack, pColor, pBlocks);
				pFormat_stack.pop_back();
			}
			else if (name == "rainbow")
			{
				pFormat_stack.push_back(text_format::format::rainbow);
				parse_xml_format(pNode->FirstChild(), pFormat_stack, pColor, pBlocks);
				pFormat_stack.pop_back();
			}
		}
		pNode = pNode->NextSibling();
	}
	return true;
}

bool text_format::parse_text(const std::string & pText)
{
	using namespace tinyxml2;

	mUnformatted_text = pText;
	mBlocks.clear();

	// When parse fails, this object stays in a semi valid state
	XMLDocument doc;
	if (doc.Parse(pText.c_str(), pText.size()) != XML_SUCCESS)
	{
		block nblock;
		nblock.mText = pText;
		nblock.mFormat = text_format::format::none;
		nblock.mColor = mDefault_color;
		mBlocks.push_back(nblock);
		return false;
	}

	std::vector<format> format_stack;
	return parse_xml_format(doc.FirstChild(), format_stack, mDefault_color, mBlocks);
}

size_t text_format::get_block_count() const
{
	return mBlocks.size();
}

const text_format::block & text_format::get_block(size_t pIndex) const
{
	return mBlocks[pIndex];
}

std::vector<text_format::block>::const_iterator text_format::begin() const
{
	return mBlocks.begin();
}

std::vector<text_format::block>::const_iterator text_format::end() const
{
	return mBlocks.end();
}

rich_text_node::rich_text_node()
{
	mTimer = 0;
	mCharacter_size = 15;
}

void rich_text_node::set_font(std::shared_ptr<font> pFont, bool pApply_preferences)
{
	mFont = pFont;
	mFont->load();
}

void rich_text_node::set_text(const text_format & pText)
{
	mFormat = pText;
	update();
}

int rich_text_node::draw(renderer & pR)
{
	if (!mFont)
		return 1;
	mTimer += pR.get_delta();
	update_effects();

	// Screw all common sense!
	auto texture = const_cast<sf::Texture*>(&mFont->mSFML_font->getTexture(mCharacter_size));
	texture->setSmooth(false);
	return mVertex_batch.draw(pR, *texture);
}

void rich_text_node::update_effects()
{
	for (auto& i : mBlock_handles)
	{
		const auto& block = mFormat.get_block(i.mBlock_index);

		fvector offset(0, 0);

		if (block.mFormat & text_format::format::wave)
		{
			const float wave_amount = 0.1f;
			const float wideness = 0.5f;
			const float speed = 3.f;
			offset += fvector::as_y((std::cos(std::fmodf(mTimer*speed + i.mOriginal_position.x*wideness, 3.14*2)))
				*i.mVertices.get_size().y*wave_amount);
		}
		if (block.mFormat & text_format::format::shake)
		{
			offset += fvector((float)(rand() % 100) / 100 - 0.5f, (float)(rand() % 100) / 100 - 0.5f);
		}
		if (offset != fvector(0, 0))
			i.mVertices.set_position(i.mOriginal_position + offset);
	}
}

void rich_text_node::update()
{
	mBlock_handles.clear();
	mVertex_batch.clean();
	mSize = fvector(0, 0);

	auto font = mFont->mSFML_font.get();
	
	const float vspace = font->getLineSpacing(mCharacter_size);
	const float hspace = font->getGlyph(' ', mCharacter_size, true).advance;

	char prev_character = ' ';

	fvector position(0, static_cast<float>(mCharacter_size));
	for (size_t i = 0; i < mFormat.get_block_count(); i++)
	{

		const auto& block = mFormat.get_block(i);
		for (auto j : block.mText)
		{
			const auto& glyph = font->getGlyph(j, mCharacter_size, block.mFormat & text_format::format::bold);

			// Check for whitespace and advance positions
			switch (j)
			{
			case ' ':
				position.x += hspace;
				continue;
			case '\t':
				position.x += hspace * 4; // Tabs 4 spaces for now
				continue;
			case '\n':
				position.y += vspace;
				position.x = 0;
				continue;
			}

			// Create handle for connecting the blocks 
			// to the verticies. These are then iterated through
			// and effects are applied.
			const frect glyph_rect(frect::cast<int>(glyph.textureRect));
			const fvector bounds_offset(glyph.bounds.left, glyph.bounds.top);
			block_handle handle;
			handle.mBlock_index = i;
			handle.mVertices = mVertex_batch.add_quad(position + bounds_offset, glyph_rect);
			handle.mVertices.set_color(block.mColor);
			handle.mVertices.set_hskew(block.mFormat & text_format::format::italics ? 0.5f : 0);
			handle.mOriginal_position = position + bounds_offset;
			mBlock_handles.push_back(handle);

			// Update size
			if (position.x + glyph_rect.x > mSize.x)
				mSize.x = position.x + glyph_rect.w;
			if (position.y + glyph_rect.y > mSize.y)
				mSize.y = position.y + glyph_rect.h;

			position.x += hspace;

			prev_character = j;
		}

	}
}
