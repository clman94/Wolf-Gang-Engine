#include <editor/file_opener.hpp>
#include <engine/time.hpp>
#include <engine/logger.hpp>

#include <ImGui.h>

#include <vector>

#include <cstddef>
#include <cstdint>

// Amount of seconds until the file list refreshes
const float update_frequency = 5;

struct entry_info
{
	std::string name;
	engine::fs::path path;
	bool is_directory;
	std::uintmax_t size;
};

static struct {
	engine::fs::path current_directory;
	std::vector<entry_info> file_entries;
	engine::timer update_timer;
} opener_data;

static inline void update_file_list()
{
	opener_data.file_entries.clear();
	for (auto i : engine::fs::directory_iterator(opener_data.current_directory))
	{
		entry_info info;
		try {
			info.name = i.path().filename().string();
			info.path = i;
			info.is_directory = engine::fs::is_directory(i);
			opener_data.file_entries.push_back(info);
		}
		catch (...)
		{
			logger::warning("Unable to fetch data of a file");
		}
	}

	// Sort so all the directories are at the top
	std::stable_sort(opener_data.file_entries.begin(), opener_data.file_entries.end()
		, [](const entry_info& l, const entry_info& r)->bool
	{
		return l.is_directory && !r.is_directory;
	});
}

static inline void change_current_directory(const engine::fs::path& pPath)
{
	opener_data.current_directory = pPath;
	update_file_list();
}


bool ImGui::FileOpenerPopup(const char * pName, engine::fs::path* pPath, bool pSel_files, bool pSel_directories)
{
	assert(pSel_files || pSel_directories); // At least one of them has to be true
	assert(pPath);

	bool has_opened_file = false;

	if (ImGui::BeginPopupModal(pName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (opener_data.current_directory.empty())
			opener_data.current_directory = engine::fs::current_path();
		
		// Refresh file list
		if (opener_data.update_timer.is_reached())
		{
			update_file_list();
			opener_data.update_timer.start(update_frequency);
		}

		ImGui::BeginGroup();

		ImGui::PushItemWidth(-1);

		static char file_path_buf[512];
		if (ImGui::InputText("", file_path_buf, 512, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			engine::fs::path new_path = file_path_buf;
			if (engine::fs::exists(new_path))
			{
				if (engine::fs::is_directory(new_path))
					change_current_directory(new_path);
				else
				{
					// Set inputed path as the selected item
					*pPath = new_path;
					change_current_directory(new_path.parent_path());
				}
			}
			else
				logger::error("Entry '" + new_path.string() + "' does not exist");
		}
		ImGui::PopItemWidth();

		if (!ImGui::IsItemActive())
		{
			// Update textbox with current directory
			std::string path_str = opener_data.current_directory.string();
			if (path_str.size() + 1 < 512)
				memcpy(file_path_buf, path_str.c_str(), path_str.size() + 1);
		}

		ImGui::BeginChild("FileChooser", ImVec2(400, 400));

		ImGui::Columns(2, 0, false);

		// Parent directory item ".."
		if (opener_data.current_directory.has_parent_path())
		{
			ImGui::Selectable("..##gobackfileshooser", false, ImGuiSelectableFlags_SpanAllColumns);
			if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
				change_current_directory(opener_data.current_directory.parent_path());

			ImGui::NextColumn();
			ImGui::NextColumn();
		}

		for (const auto& i : opener_data.file_entries)
		{
			if (!i.is_directory && !pSel_files) // Only allow directories
				continue;

			if (ImGui::Selectable(i.name.c_str(), i.path == *pPath, ImGuiSelectableFlags_SpanAllColumns))
			{
				// Pick file (or directory)
				if (!i.is_directory || pSel_directories)
					*pPath = i.path;
			}

			// Open directory
			if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
			{
				if (i.is_directory)
				{
					change_current_directory(i.path);
					break;
				}
			}

			//quick_tooltip(("File Size: " + std::to_string(i.size) + " bytes").c_str());

			ImGui::NextColumn();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), i.is_directory ? "Directory" : "File");
			ImGui::NextColumn();
		}
		ImGui::EndChild();

		ImGui::EndGroup();

		ImGui::BeginGroup();

		if (ImGui::Button("Open", ImVec2(100, 25)))
		{
			if (!pPath->empty())
			{
				has_opened_file = true;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 25)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndGroup();

		ImGui::EndPopup();
	}
	return has_opened_file;
}