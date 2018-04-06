#include <Windows.h>

#include <engine/logger.hpp>
#include <ui/clipboard.hpp>

namespace clipboard
{

bool has_text()
{
	if (!OpenClipboard(NULL))
	{
		logger::error("Failed to open windows clipboard");
		return false;
	}

	bool val = IsClipboardFormatAvailable(CF_TEXT);

	CloseClipboard();
	return val;
}

bool set_text(const std::string& pString)
{
	if (!OpenClipboard(NULL))
	{
		logger::error("Failed to open windows clipboard");
		return false;
	}
	
	if (!EmptyClipboard())
	{
		logger::error("Failed to empty windows clipboard");
		CloseClipboard();
		return false;
	}
	
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, pString.size() + 1); // +1 includes \0
	if (!hg)
	{
		logger::error("Failed to allocate data to windows clipboard");
		CloseClipboard();
		return false;
	}
	memcpy(GlobalLock(hg), pString.c_str(), pString.size() + 1);
	GlobalUnlock(hg);

	if (!SetClipboardData(CF_TEXT, hg))
	{
		logger::error("Failed to set text in windows clipboard");
		CloseClipboard();
		return false;
	}
	
	CloseClipboard();
	return true;
}

std::string get_text()
{
	if (!OpenClipboard(NULL))
	{
		logger::error("Failed to open windows clipboard");
		return{};
	}

	if (!IsClipboardFormatAvailable(CF_TEXT))
		return{};

	HGLOBAL  hg = GetClipboardData(CF_TEXT);
	std::string val = (char*)GlobalLock(hg);
	GlobalUnlock(hg);

	CloseClipboard();
	return val;
}


}