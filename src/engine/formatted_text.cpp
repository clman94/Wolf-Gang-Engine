#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

#include "../../tinyxml2/tinyxml2.h"

using namespace engine;

text_format::text_format()
{
	mDefault_color = color(255, 255, 255, 255);
}

text_format::text_format(const char * pText)
{
	mDefault_color = color(255, 255, 255, 255);
	parse_text(pText);
}

text_format::text_format(const std::string & pText)
{
	mDefault_color = color(255, 255, 255, 255);
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
			else if (name == "br")
			{
				text_format::block nblock;
				nblock.mFormat = 0;
				for (auto& i : pFormat_stack)
					nblock.mFormat |= static_cast<uint32_t>(i);
				nblock.mText = "\n";
				pBlocks.push_back(nblock);

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
	mBlocks.clear();
	return start_parse(pText);
}

bool text_format::append(const std::string & pText)
{
	return start_parse(pText);
}

void text_format::append(const text_format & pFormat)
{
	mBlocks.insert(mBlocks.end(), pFormat.mBlocks.begin(), pFormat.mBlocks.end());
}

size_t text_format::get_block_count() const
{
	return mBlocks.size();
}

const text_format::block & text_format::get_block(size_t pIndex) const
{
	return mBlocks[pIndex];
}

text_format & text_format::operator+=(const text_format & pFormat)
{
	append(pFormat);
	return *this;
}

std::vector<text_format::block>::const_iterator text_format::begin() const
{
	return mBlocks.begin();
}

std::vector<text_format::block>::const_iterator text_format::end() const
{
	return mBlocks.end();
}

text_format text_format::substr(size_t pOffset, size_t pCount) const
{
	if (pOffset == 0 && pCount >= length())
		return *this;

	if (pCount == 0)
		return{};

	text_format retformat;

	size_t characters_passed = 0;
	for (size_t i = 0; i < mBlocks.size(); i++)
	{
		// Has reached the Start, now keep added blocks
		if (characters_passed + (mBlocks[i].mText.size() - 1) >= pOffset)
		{
			retformat.mBlocks.push_back(mBlocks[i]);

			// Cut the extra characters at the front
			if (characters_passed < pOffset)
			{
				const std::string cut_text
					= retformat.mBlocks.back().mText.substr(pOffset - characters_passed);
				retformat.mBlocks.back().mText = cut_text;
			}
		}

		// Has reached the end
		if (characters_passed + (mBlocks[i].mText.size() - 1) >= pOffset + pCount)
		{
			// Cut the extra characters at the back
			const std::string cut_text
				= retformat.mBlocks.back().mText.substr(0, pOffset + pCount - characters_passed);
			retformat.mBlocks.back().mText = cut_text;

			// Everything is done!
			return retformat;
		}
		characters_passed += mBlocks[i].mText.size();
	}
	return retformat;
}

bool text_format::word_wrap(size_t pLength)
{
	if (pLength == 0)
		return false;

	size_t characters_passed = 0;
	size_t last_line = 0;
	size_t last_space = 0;
	size_t last_space_index = 0;
	block* last_space_block = nullptr;

	for (auto& i : mBlocks)
	{
		for (size_t j = 0; j < i.mText.size(); j++)
		{
			if (i.mText[j] == '\n')
			{
				last_line = characters_passed;
			}

			if (i.mText[j] == ' ')
			{
				last_space = characters_passed;
				last_space_index = j;
				last_space_block = &i;
			}

			if (characters_passed - last_line > pLength
				&& last_space_block != nullptr)
			{
				last_space_block->mText[last_space_index] = '\n';
				last_space_block = nullptr;
				last_line = last_space;
			}

			++characters_passed;
		}
	}

	return true;
}

void text_format::remove_first_line()
{
	size_t offset = 0;
	for (auto& i : mBlocks)
	{
		for (auto j : i.mText)
		{
			if (j == '\n')
			{
				*this = substr(offset + 1, INT32_MAX);
				return;
			}
			++offset;
		}
	}
	mBlocks.clear();
}

void text_format::limit_lines(size_t pLines)
{
	size_t lines = line_count();
	if (lines <= pLines)
		return;
	for (size_t i = 0; i < lines - pLines; i++)
		remove_first_line();
}

size_t text_format::line_count() const
{
	if (mBlocks.empty())
		return 0;
	size_t lines = 1;
	for (auto& i : mBlocks)
	{
		for (auto j : i.mText)
			if (j == '\n')
				++lines;
	}
	return lines;
}

size_t text_format::length() const
{
	size_t total = 0;
	for (auto& i : mBlocks)
	{
		total += i.mText.length();
	}
	return total;
}

bool text_format::start_parse(const std::string & pText)
{
	// When parse fails, this object is still in a valid state
	tinyxml2::XMLDocument doc;
	const std::string text_rooted = "<__root>" + pText + "</__root>"; // Add a root node so it parses correctly
	if (doc.Parse(text_rooted.c_str(), text_rooted.size()) != tinyxml2::XML_SUCCESS)
	{
		// Create default block with unformmatted text
		block nblock;
		nblock.mText = pText;
		nblock.mFormat = text_format::format::none;
		nblock.mColor = mDefault_color;
		mBlocks.push_back(nblock);
		return false;
	}

	std::vector<format> format_stack;
	auto root_node = doc.FirstChild();
	return parse_xml_format(root_node->FirstChild(), format_stack, mDefault_color, mBlocks);
}

formatted_text_node::formatted_text_node()
{
	mAnchor = anchor::topleft;
	mTimer = 0;
	mCharacter_size = 30;
}

void formatted_text_node::set_font(std::shared_ptr<font> pFont, bool pApply_preferences)
{
	mFont = pFont;
	mFont->load();
	mCharacter_size = mFont->mCharacter_size;
	update();
}

void formatted_text_node::set_text(const text_format & pText)
{
	mFormat = pText;
	update();
}

const text_format& formatted_text_node::get_text() const
{
	return mFormat;
}

void formatted_text_node::set_color(const color & pColor)
{
	for (auto& i : mBlock_handles)
	{
		i.mVertices.set_color(pColor);
	}
}

void formatted_text_node::set_anchor(anchor pAnchor)
{
	mAnchor = pAnchor;
}

fvector formatted_text_node::get_size() const
{
	return mSize;
}

void formatted_text_node::set_character_size(size_t pSize)
{
	mCharacter_size = pSize;
}

frect formatted_text_node::get_render_rect() const
{
	return{ get_exact_position() + engine::anchor_offset(get_size(), mAnchor), get_size() };
}

int formatted_text_node::draw(renderer & pR)
{
	if (!mFont)
		return 1;
	mTimer += pR.get_delta();
	update_effects();

	mVertex_batch.set_position(get_exact_position() + anchor_offset(mSize, mAnchor));

	// Screw all common sense!
	sf::Texture* texture = const_cast<sf::Texture*>(&mFont->mSFML_font->getTexture(mCharacter_size*4));
	texture->setSmooth(false);
	return mVertex_batch.draw(pR, *texture);
}

void formatted_text_node::update_effects()
{
	for (auto& i : mBlock_handles)
	{
		const auto& block = mFormat.get_block(i.mBlock_index);

		fvector offset(0, 0);

		if (block.mFormat & text_format::format::wave)
		{
			const float wave_amount = 0.1f;
			const float wideness_scale = 0.5f;
			const float speed = 3.f;
			offset += fvector::as_y((std::sin(std::fmod(mTimer*speed + i.mOriginal_position.x*wideness_scale, 3.14f*2)))
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

void formatted_text_node::update()
{
	const float scale_quality = 4;
	const size_t scaled_character_size = static_cast<size_t>(mCharacter_size*scale_quality);

	mBlock_handles.clear();
	mVertex_batch.clean();
	mSize = fvector(0, 0);

	auto font = mFont->mSFML_font.get();
	
	const float vspace = font->getLineSpacing(scaled_character_size) / scale_quality;
	const float hspace = font->getGlyph(' ', scaled_character_size, true).advance / scale_quality;

	fvector position = fvector(0, static_cast<float>(mCharacter_size)) + mFont->mOffset;
	for (size_t i = 0; i < mFormat.get_block_count(); i++)
	{

		const auto& block = mFormat.get_block(i);
		for (auto j : block.mText)
		{


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

			const auto& glyph = font->getGlyph(j, scaled_character_size, block.mFormat & text_format::format::bold);

			// Create handle for connecting the blocks 
			// to the verticies. These are then iterated through
			// and effects are applied.
			const frect glyph_rect(frect::cast<int>(glyph.textureRect));
			const fvector bounds_offset = fvector(glyph.bounds.left, glyph.bounds.top) / scale_quality;
			block_handle handle;
			handle.mBlock_index = i;
			handle.mVertices = mVertex_batch.add_quad(position + bounds_offset, glyph_rect);
			handle.mVertices.set_size(fvector(glyph_rect.w, glyph_rect.h) / scale_quality);
			handle.mVertices.set_color(block.mColor);// Color
			handle.mVertices.set_hskew((block.mFormat & text_format::format::italics) ? 0.5f : 0); // Italics
			handle.mOriginal_position = position + bounds_offset;
			mBlock_handles.push_back(handle);

			// Update size

			fvector vert_size = handle.mVertices.get_size() + handle.mVertices.get_position();

			if (vert_size.x > mSize.x)
				mSize.x = vert_size.x;
			if (vert_size.y > mSize.y)
				mSize.y = vert_size.y;

			position.x += hspace;
		}

	}
}


