#pragma once

#include "imgui_editor_tools.hpp"
#include "imgui_ext.hpp"

#include <string_view>

namespace wge::editor
{

class text_editor
{
public:
	enum class palette_type : std::size_t
	{
		default,
		keyword,
		string,
		comments,
		line,
		selection,
		line_number,
		count
	};

	struct position
	{
		std::size_t line{ 0 };
		std::size_t column{ 0 };

		bool operator==(const position& pPos) const noexcept
		{
			return line == pPos.line && column == pPos.column;
		}

		bool operator!=(const position& pPos) const noexcept
		{
			return !operator==(pPos);
		}

		bool operator<(const position& pPos) const noexcept
		{
			return line < pPos.line || (line == pPos.line && column < pPos.column);
		}
	};

	class language
	{
	public:
		std::set<std::string> keywords;
		std::string singleline_comment;
	};

	text_editor();

	void set_text(const std::string& pText);
	const std::string get_text() const noexcept;

	void set_language(const language& pLang) { mLanguage = &pLang; }

	void insert_character(char pChar);
	void insert_text(const std::string_view& pView);

	void erase_range(const position& pStart, const position& pEnd);
	void set_range_color(const position& pStart, const position& pEnd, palette_type pPalette);
	std::string get_range(const position& pStart, const position& pEnd) const;

	position get_selection_start() const noexcept;
	position get_selection_end() const noexcept;
	std::string get_selected_text() const;
	void select_all() noexcept;
	bool is_text_selected() const noexcept
	{
		return mSelection_start != mSelection_end;
	}
	void erase_selection();
	void deselect() noexcept;

	void backspace();
	void cursor_up();
	void cursor_down();
	void cursor_left();
	void cursor_right();

	void copy_to_clipboard();
	void paste_from_clipboard();

	std::size_t get_line_count() const noexcept
	{
		return mLine_lengths.size();
	}

	void render(const ImVec2& pSize);

private:
	void highlight_line(std::size_t pLine);
	void highlight_range(const position& pStart, const position& pEnd);
	void highlight_all();
	void reset_color();

	void update_line_lengths();

	void handle_shift_selection(const position& pLast_pos);
	void handle_selection(const position& pPos);
	void handle_keyboard();

	math::vec2 calc_text_size(const math::vec2& pCharacter_size) const;

	std::size_t get_character_index(const position& pPosition) const;
	std::string_view get_line_view(std::size_t pLine) const;
	char get_character_at(const position& pPosition) const;
	position get_position_at(std::size_t pIndex) const;

	std::size_t calc_actual_columns(std::size_t pLine, std::size_t pColumn_wo_tabs) const;
	std::size_t calc_line_distance(const position& pPosition) const;

	std::string_view get_line_indentation(std::size_t pLine) const;

	position correct_position(const position& pPosition) const;

private:
	std::array<graphics::color, (std::size_t)palette_type::count> mPalette;
	std::vector<std::size_t> mLine_lengths;
	std::vector<palette_type> mText_color;
	std::string mText;

	bool mNeeds_highlight{ false };

	const language* mLanguage{ nullptr };

	std::size_t mTab_width{ 4 };

	position mSelection_start, mSelection_end;
	position mCursor_position;
};

} // namespace wge::editor

namespace ImGui
{

bool CodeEditor(const char* pID, std::string& pText, const ImVec2& pSize = { 0, 0 });

}