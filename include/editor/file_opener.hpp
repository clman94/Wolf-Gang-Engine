#pragma once

#include <engine/filesystem.hpp>

namespace ImGui
{

// Returns true when a file/directory is selected
bool FileOpenerPopup(const char* pName, engine::fs::path* pPath, bool pSel_files = true, bool pSel_directories = false);

}