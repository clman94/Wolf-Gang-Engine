#include "text_editor.hpp"

namespace wge::editor
{

text_editor::text_editor()
{
	static language lang;
	lang.keywords = { "and", "break", "do", "else", "elseif", "end", "false",
		"for", "function", "if", "in", "local", "nil", "not", "or", "repeat",
		"return", "then", "true", "until", "while" };
	
	lang.operators = { "+", "-", "*", "/", "%", "^", "#", "==", "~=", "<=",
		">=", "<", ">", "=", "(", ")", "{", "}", "[", "]", ";", ":", ",",
		".", "..", "..." };
	// Sort longest to shortest
	std::sort(lang.operators.begin(), lang.operators.end(),
		[](const std::string_view& a, const std::string_view& b) { return a.length() < b.length(); });
	
	lang.singleline_comment = "--";
	mLanguage = &lang;
	mPalette =
	{
		graphics::color{ 1, 1, 1, 1 },
		graphics::color{ 0.5f, 0.5f, 1, 1 },
		graphics::color{ 0.9f, 0.7f, 0.6f, 1 },
		graphics::color{ 0.5f, 1, 0.5f, 1 },
		graphics::color{ 0.9f, 0.9f, 0.5f, 0.3f },
		graphics::color{ 0.9f, 0.8f, 0.5f, 0.3f },
		graphics::color{ 0.5f, 0.8f, 0.8f, 1 },
	};
}

void text_editor::set_text(const std::string& pText)
{
	mText = pText;

	// Remove unsupported line endings
	auto iter = std::remove(mText.begin(), mText.end(), '\r');
	mText.erase(iter, mText.end());

	mText_color.resize(mText.size());
	reset_color();
	update_line_lengths();
	highlight_all();
}

const std::string text_editor::get_text() const noexcept
{
	return mText;
}

void text_editor::insert_character(char pChar)
{
	if (pChar == '\n')
	{
		std::string str;
		str += pChar;
		// Copy indentation from the previous line
		str += get_line_indentation(mCursor_position);
		insert_text(str);
	}
	else
	{
		insert_text(std::string_view(&pChar, 1));
	}
}

void text_editor::insert_text(const std::string_view& pView)
{
	erase_selection();

	// Clean up the text by removing any unsupported line endings
	std::string cleaned_str(pView);
	auto iter = std::remove(cleaned_str.begin(), cleaned_str.end(), '\r');
	cleaned_str.erase(iter, cleaned_str.end());

	const position prev_pos = mCursor_position;

	// Insert the text
	const std::size_t index = get_character_index(mCursor_position);
	mText.insert(mText.begin() + index, cleaned_str.begin(), cleaned_str.end());
	mText_color.insert(mText_color.begin() + index, cleaned_str.length(), palette_type::default);

	// We only need to update _all_ line lengths if there is an actual change in line counts
	if (std::find(cleaned_str.begin(), cleaned_str.end(), '\n') != cleaned_str.end())
	{
		update_line_lengths();
		mCursor_position = get_position_at(index + cleaned_str.length());
	}
	else
	{
		mLine_lengths[mCursor_position.line] += cleaned_str.length();
		mCursor_position.column += cleaned_str.length();
	}

	highlight_range(prev_pos, mCursor_position);
}

void text_editor::erase_range(const position& pStart, const position& pEnd)
{
	std::size_t index_start = get_character_index(pStart);
	std::size_t index_end = get_character_index(pEnd);
	mText.erase(mText.begin() + index_start, mText.begin() + index_end);
	mText_color.erase(mText_color.begin() + index_start, mText_color.begin() + index_end);
	update_line_lengths();
	highlight_line(pStart.line);
}

void text_editor::set_range_color(const position& pStart, const position& pEnd, palette_type pPalette)
{
	std::size_t index_start = get_character_index(pStart);
	std::size_t index_end = get_character_index(pEnd);
	std::fill(mText_color.begin() + index_start, mText_color.begin() + index_end, pPalette);
}

std::string text_editor::get_range(const position & pStart, const position & pEnd) const
{
	std::size_t index_start = get_character_index(pStart);
	std::size_t index_end = get_character_index(pEnd);
	return std::string(mText.begin() + index_start, mText.begin() + index_end);
}

text_editor::position text_editor::get_selection_start() const noexcept
{
	return mSelection_end < mSelection_start ? mSelection_end : mSelection_start;
}

text_editor::position text_editor::get_selection_end() const noexcept
{
	return mSelection_end < mSelection_start ? mSelection_start : mSelection_end;
}

std::string text_editor::get_selected_text() const
{
	return get_range(get_selection_start(), get_selection_end());
}

void text_editor::select_all() noexcept
{
	mSelection_start = position{ 0, 0 };
	mSelection_end = position{ get_line_count() - 1, mLine_lengths.back() };
}

void text_editor::erase_selection()
{
	if (is_text_selected())
	{
		position start = get_selection_start();
		erase_range(start, get_selection_end());
		mCursor_position = mSelection_start = mSelection_end = start;
	}
}

void text_editor::deselect() noexcept
{
	mSelection_start = mSelection_end = position{ 0, 0 };
}

void text_editor::backspace()
{
	// Only delete the selection if there is one.
	if (is_text_selected())
	{
		erase_selection();
		return;
	}

	std::size_t index = get_character_index(mCursor_position);
	if (index == 0)
		return;

	// We want to delete the character before the cursor.
	--index;

	// Delete the line break and merge the lines
	if (mCursor_position.column == 0)
	{
		mCursor_position.column = mLine_lengths[mCursor_position.line - 1];

		// Merge the line lenths
		mLine_lengths[mCursor_position.line - 1] += mLine_lengths[mCursor_position.line];
		mLine_lengths.erase(mLine_lengths.begin() + mCursor_position.line);

		--mCursor_position.line;
	}

	--mLine_lengths[mCursor_position.line];
	--mCursor_position.column;

	mText.erase(mText.begin() + index);
	mText_color.erase(mText_color.begin() + index);

	highlight_line(mCursor_position.line);
}

void text_editor::cursor_up()
{
	const position last_pos = mCursor_position;
	if (mCursor_position.line == 0)
	{
		// Move to the start of the line if this is the first line.
		mCursor_position.column = 0;
	}
	else
	{
		std::size_t prev_distance = calc_line_distance(mCursor_position);
		--mCursor_position.line;
		mCursor_position.column = calc_actual_columns(mCursor_position.line, prev_distance);
		mCursor_position = correct_position(mCursor_position);
	}
	handle_shift_selection(last_pos);
}

void text_editor::cursor_down()
{
	const position last_pos = mCursor_position;
	if (mCursor_position.line == get_line_count() - 1)
	{
		// Move to the end of the line if this is the last line.
		mCursor_position.column = mLine_lengths[mCursor_position.line];
	}
	else
	{
		std::size_t prev_distance = calc_line_distance(mCursor_position);
		++mCursor_position.line;
		mCursor_position.column = calc_actual_columns(mCursor_position.line, prev_distance);
		mCursor_position = correct_position(mCursor_position);
	}
	handle_shift_selection(last_pos);
}

void text_editor::cursor_left()
{
	const position last_pos = mCursor_position;
	if (mCursor_position.column == 0 && mCursor_position.line != 0)
	{
		// Go to the end of the previous line
		--mCursor_position.line;
		mCursor_position.column = mLine_lengths[mCursor_position.line] - 1;
	}
	else if (mCursor_position.column > 0)
	{
		--mCursor_position.column;
	}
	handle_shift_selection(last_pos);
}

void text_editor::cursor_right()
{
	const position last_pos = mCursor_position;
	if (mCursor_position.column >= mLine_lengths[mCursor_position.line] - 1 &&
		mCursor_position.line != get_line_count() - 1)
	{
		// Go to the beginning of the next line
		++mCursor_position.line;
		mCursor_position.column = 0;
	}
	else if (mCursor_position.column != mLine_lengths[mCursor_position.line])
	{
		++mCursor_position.column;
	}
	handle_shift_selection(last_pos);
}

void text_editor::cut_to_clipboard()
{
	copy_to_clipboard();
	erase_selection();
}

void text_editor::copy_to_clipboard()
{
	ImGui::SetClipboardText(get_selected_text().c_str());
}

void text_editor::paste_from_clipboard()
{
	const char* str = ImGui::GetClipboardText();
	if (str)
		insert_text(str);
}

void text_editor::render(const ImVec2& pSize)
{
	ImGui::BeginChild("textedit", pSize);

	if (ImGui::IsWindowFocused())
	{
		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		handle_keyboard();
	}

	const float character_width = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1, " ").x;
	const float line_height = ImGui::GetTextLineHeightWithSpacing();

	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 cursor_start = ImGui::GetCursorScreenPos();

	// Offset the render to the right so we can fit in line numbers
	cursor_start.x += character_width * 6;


	math::vec2 edit_size = calc_text_size({ character_width , line_height });
	edit_size.x = math::max(edit_size.x, ImGui::GetWindowContentRegionWidth());
	edit_size.y = math::max(edit_size.y, ImGui::GetWindowContentRegionMax().y);

	ImGui::InvisibleButton("interaction", edit_size);

	if (ImGui::BeginPopupContextWindow("EditPopup"))
	{
		if (ImGui::MenuItem("Change Color"))
		{
			set_range_color(get_selection_start(), get_selection_end(), palette_type::keyword);
		}
		ImGui::EndPopup();
	}
	
	// Calculate mouse select
	{
		math::vec2 pos = math::vec2(ImGui::GetMousePos()) - math::vec2(cursor_start);
		std::size_t line = static_cast<std::size_t>(math::max(pos.y / line_height, 0.f));
		std::size_t column = static_cast<std::size_t>(math::round(math::max(pos.x / character_width, 0.f)));

		handle_selection(position{ line, column });
	}

	if (mNeeds_highlight)
	{
		highlight_all();
		mNeeds_highlight = false;
	}

	// Draw a box around the line where the cursor is at.
	{
		ImVec2 a(
			cursor_start.x + ImGui::GetScrollX(),
			cursor_start.y + line_height * mCursor_position.line);
		ImVec2 b(
			dl->GetClipRectMax().x,
			a.y + line_height);
		dl->AddRect(a, b, ImGui::GetColorU32(mPalette[(std::size_t)palette_type::line]), 0, 15, 3);
	}

	position selection_start = mSelection_start;
	position selection_end = mSelection_end;
	// Flip them if the text was selected backwards.
	if (selection_end < selection_start)
		std::swap(selection_start, selection_end);

	// Optimise drawing by only drawing lines if they are visible
	std::size_t start_line = static_cast<std::size_t>(math::floor(ImGui::GetScrollY() / line_height));
	std::size_t end_line = static_cast<std::size_t>(math::ceil((ImGui::GetScrollY() + ImGui::GetWindowHeight()) / line_height));
	end_line = math::min(end_line, get_line_count());

	// Draw line numbers
	for (std::size_t line = start_line; line < end_line; line++)
	{
		std::string number_str = std::to_string(line + 1);

		ImVec2 pos = cursor_start;
		pos.y += line_height * line;
		// Offset the line numbers so they are to the left of the text.
		pos.x -= character_width * (number_str.size() + 1);
		// Offset them again so they are always visible.
		pos.x += ImGui::GetScrollX();

		dl->AddText(pos, ImGui::GetColorU32(ImVec4(mPalette[(std::size_t)palette_type::line_number])), &number_str[0], &number_str.back() + 1);
	}

	dl->PushClipRect(cursor_start + ImVec2(ImGui::GetScrollX(), 0), cursor_start + ImGui::GetWindowContentRegionMax(), true);

	// Draw cursor
	{
		float offset = 0;
		ImVec2 a(
			cursor_start.x + character_width * calc_line_distance(mCursor_position),
			cursor_start.y + line_height * mCursor_position.line);
		ImVec2 b(a.x, a.y + line_height);
		dl->AddLine(a, b, ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 1);
	}

	std::size_t line_start_index = get_character_index(position{ start_line, 0 });
	for (std::size_t line = start_line; line < end_line; line++)
	{
		ImVec2 line_pos = cursor_start;
		line_pos.y += line_height * line;

		// Draw selection
		if (selection_start != selection_end &&
			selection_start.line <= line && selection_end.line >= line)
		{
			float start = 0;
			if (line == selection_start.line)
				start = static_cast<float>(calc_line_distance(selection_start));

			float end;
			if (line == selection_end.line)
				end = static_cast<float>(calc_line_distance(selection_end));
			else
				end = static_cast<float>(calc_line_distance(position{ line, mLine_lengths[line] }));

			ImVec2 a(
				line_pos.x + start * character_width,
				line_pos.y);
			ImVec2 b(
				line_pos.x + end * character_width,
				line_pos.y + line_height);
			dl->AddRectFilled(a, b, ImGui::GetColorU32(mPalette[(std::size_t)palette_type::selection]));
		}

		// Draw text
		std::string_view line_str(&mText[line_start_index], mLine_lengths[line]);
		std::size_t start = 0;
		palette_type prevcolor = palette_type::default;
		for (std::size_t i = 0; i < line_str.size(); i++)
		{
			char c = line_str[i];

			palette_type color = mText_color[line_start_index + i];
			if (c == '\t' || c == '\n' || color != prevcolor)
			{
				// Only draw if needed
				if (start < i)
				{
					dl->AddText(line_pos, ImGui::GetColorU32(ImVec4(mPalette[(std::size_t)prevcolor])), &line_str[start], &line_str[i]);
					line_pos.x += character_width * static_cast<float>(i - start);
				}
				start = i;
			}

			// Tab will offset the rendering for the next batch
			if (c == '\t')
			{
				line_pos.x += character_width * mTab_width;
				++start; // Skip
			}

			prevcolor = color;
		}

		// Render the last bit if there is any.
		if (start < line_str.size())
			dl->AddText(line_pos, ImGui::GetColorU32(ImVec4(mPalette[(std::size_t)prevcolor])), &line_str[start], &line_str.back() + 1);

		line_start_index += mLine_lengths[line];
	}

	dl->PopClipRect();

	ImGui::EndChild();
}

static constexpr bool is_letter(char pC) noexcept
{
	return pC >= 'a' && pC <= 'z' || pC >= 'A' && pC <= 'Z';
}

static constexpr bool is_number(char pC) noexcept
{
	return pC >= '0' && pC <= '9';
}

inline std::string_view parse_identifier(const std::string_view& pView)
{
	auto i = pView.begin();
	for (; i != pView.end(); ++i)
		if (!is_letter(*i) && !is_number(*i) && *i != '_')
			break;
	return std::string_view{ &*pView.begin(), (std::size_t)std::distance(pView.begin(), i) };
}

inline std::size_t parse_string(const std::string_view& pView)
{
	auto i = pView.begin();
	++i; // Skip initial quote.
	for (; i != pView.end(); ++i)
	{
		// Escape codes
		if (*i == '\\' && i + 1 != pView.end())
		{
			++i; // Skip code
		}
		else if (*i == '\"' || *i == '\'')
		{
			++i;
			break;
		}
	}
	return (std::size_t)std::distance(pView.begin(), i);
}

void text_editor::highlight_line(std::size_t pLine)
{
	if (mText.empty() || !mLanguage)
		return;
	std::size_t line_begin = get_character_index(position{ pLine, 0 });
	std::size_t line_end = line_begin + mLine_lengths[pLine];

	// Reset the color in this line
	std::fill(mText_color.begin() + line_begin, mText_color.begin() + line_end, palette_type::default);

	for (std::size_t i = line_begin; i < line_end; i++)
	{
		const char* c = &mText[i];

		// Comments
		if (line_begin + mLanguage->singleline_comment.length() <= line_end &&
			std::string_view(&mText[i], mLanguage->singleline_comment.length()) == mLanguage->singleline_comment)
		{
			std::fill(mText_color.begin() + i, mText_color.begin() + line_end, palette_type::comments);
			break; // Comments use the rest of the line.
		}

		// Identifiers
		else if (is_letter(*c))
		{
			std::string_view identifier = parse_identifier(std::string_view(c, line_end - i));
			if (std::find(mLanguage->keywords.begin(), mLanguage->keywords.end(), identifier)
				!= mLanguage->keywords.end())
			{
				std::fill(mText_color.begin() + i, mText_color.begin() + i + identifier.size(), palette_type::keyword);
			}
			i += identifier.size() - 1;
		}

		// Strings
		else if (*c == '\"' || *c == '\'')
		{
			std::size_t length = parse_string(std::string_view(c, line_end - i));
			std::fill(mText_color.begin() + i, mText_color.begin() + i + length, palette_type::string);
			i += length;
		}
	}
}

void text_editor::highlight_range(const position& pStart, const position& pEnd)
{
	for (std::size_t i = pStart.line; i <= pEnd.line; i++)
		highlight_line(i);
}

void text_editor::highlight_all()
{
	reset_color();
	for (std::size_t i = 0; i < get_line_count(); i++)
		highlight_line(i);
}

void text_editor::reset_color()
{
	std::fill(mText_color.begin(), mText_color.end(), palette_type::default);
}

void text_editor::update_line_lengths()
{
	mLine_lengths.clear();
	mLine_lengths.push_back(0);
	for (auto i : mText)
	{
		++mLine_lengths.back();
		if (i == '\n')
			mLine_lengths.push_back(0);
	}
}

void text_editor::handle_shift_selection(const position& pLast_pos)
{
	if (ImGui::GetIO().KeyShift)
	{
		if (!is_text_selected())
			mSelection_start = pLast_pos;
		mSelection_end = mCursor_position;
	}
	else
		deselect();
}

void text_editor::handle_selection(const position& pPos)
{
	if (ImGui::IsItemActive())
	{
		std::size_t column = calc_actual_columns(pPos.line, pPos.column);

		mCursor_position.line = math::min<std::size_t>(pPos.line, get_line_count() - 1);
		mCursor_position.column = math::min<std::size_t>(column, mLine_lengths[mCursor_position.line]);

		// Check if the cursor position is at the end of a line and
		// move it before the '\n' if neccessary.
		if (mCursor_position.line != get_line_count() - 1 &&
			mCursor_position.column > 0 &&
			mText.at(get_character_index(mCursor_position) - 1) == '\n')
			--mCursor_position.column;
	}

	if (ImGui::IsItemClicked(0) && !ImGui::GetIO().KeyShift)
		mSelection_start = mCursor_position;
	if (ImGui::IsItemActive())
		mSelection_end = mCursor_position;
}

void text_editor::handle_keyboard()
{
	ImGuiIO& io = ImGui::GetIO();
	io.WantCaptureKeyboard = true;
	io.WantTextInput = true;

	bool ctrl = io.KeyCtrl;
	bool alt = io.KeyAlt;

	if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
		select_all();
	else if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
		copy_to_clipboard();
	else if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
		cut_to_clipboard();
	else if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
		paste_from_clipboard();
	else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
		backspace();
	else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
		insert_character('\t');
	else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
		insert_character('\n');
	else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
		cursor_up();
	else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
		cursor_down();
	else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
		cursor_left();
	else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
		cursor_right();
	else
	{
		for (auto i : io.InputCharacters)
			if (i != 0 && i < 128)
				insert_character(static_cast<char>(i));
	}
}

math::vec2 text_editor::calc_text_size(const math::vec2& pCharacter_size) const
{
	math::vec2 result;
	result.y = get_line_count() * pCharacter_size.y;

	std::size_t longest_line = 0;
	for (auto i : mLine_lengths)
		longest_line = math::max(longest_line, i);
	result.x = pCharacter_size.x * static_cast<float>(longest_line);
	return result;
}

std::size_t text_editor::get_character_index(const position& pPosition) const
{
	std::size_t index = 0;
	for (std::size_t i = 0; i < math::min(mLine_lengths.size(), pPosition.line); i++)
		index += mLine_lengths[i];
	return math::min(index + pPosition.column, mText.length());
}

std::string_view text_editor::get_line_view(std::size_t pLine) const
{
	auto begin = mText.begin() + get_character_index(position{ pLine, 0 });
	if (begin == mText.end())
		return{};
	return std::string_view(&*begin, mLine_lengths[pLine]);
}

char text_editor::get_character_at(const position& pPosition) const
{
	return mText.at(get_character_index(pPosition));
}

text_editor::position text_editor::get_position_at(std::size_t pIndex) const
{
	position result;
	std::size_t index = 0;
	for (auto i : mLine_lengths)
	{
		if (index + i > pIndex)
		{
			result.column = pIndex - index;
			break;
		}

		// We ended up after the '\n' character so
		// let's go to the next line instead.
		else if (index + i == pIndex)
		{
			++result.line;
			break;
		}
		++result.line;
		index += i;
	}
	return correct_position(result);
}

std::size_t text_editor::calc_actual_columns(std::size_t pLine, std::size_t pColumn_wo_tabs) const
{
	std::size_t actual_column = pColumn_wo_tabs;
	std::string_view line_str = get_line_view(pLine);
	for (std::size_t i = 0; i < math::min(line_str.size(), actual_column); i++)
	{
		if (line_str[i] == '\t')
		{
			// Cursor is after the tab
			if (actual_column >= mTab_width + i)
				actual_column -= mTab_width - 1;

			// Cursor is inside the tab
			else if (actual_column < mTab_width + i)
			{
				actual_column = i + static_cast<std::size_t>(math::round(static_cast<float>(actual_column - i) / static_cast<float>(mTab_width)));
				break;
			}

		}
	}
	return actual_column;
}

std::size_t text_editor::calc_line_distance(const position& pPosition) const
{
	std::size_t dist = pPosition.column;
	std::size_t character_index = get_character_index(position{ pPosition.line, 0 });
	for (std::size_t i = 0; i < std::min(pPosition.column, mLine_lengths[pPosition.line]); i++)
		if (mText[character_index + i] == '\t')
			dist += mTab_width - 1; // One space is already there.
	return dist;
}

std::string_view text_editor::get_line_indentation(const position& pPosition) const
{
	std::string_view view = get_line_view(pPosition.line);
	std::size_t i = 0;
	for (; i < view.length(); i++)
	{
		char c = view[i];
		if (c != '\t' && c != ' ')
			break;
	}
	return view.substr(0, std::min(i, pPosition.column));
}

text_editor::position text_editor::correct_position(const position& pPosition) const
{
	position result = pPosition;
	if (pPosition.line >= get_line_count())
	{
		result.line = get_line_count() - 1;
		result.column = mLine_lengths.back();
	}
	else if (pPosition.line < get_line_count() - 1 &&
		pPosition.column >= mLine_lengths[pPosition.line])
	{
		result.column = mLine_lengths[pPosition.line] - 1;
	}
	return result;
}

} // namespace wge::editor


bool ImGui::CodeEditor(const char* pID, std::string& pText, const ImVec2& pSize)
{
	using namespace wge::editor;

	static std::map<ImGuiID, text_editor> gEditors;

	ImGuiID id = ImGui::GetID(pID);

	const bool first_time = gEditors.find(id) == gEditors.end();

	text_editor& editor = gEditors[id];

	ImGui::PushID(pID);

	bool modified = false;
	// Update the widget's text on first time or when it changes outside the editor.
	if (editor.get_text() != pText || first_time)
	{
		editor.set_text(pText);
		editor.render(pSize);
	}
	else
	{
		editor.render(pSize);
		modified = editor.get_text() != pText;
		pText = editor.get_text();
	}

	ImGui::PopID();
	
	return modified;
}
