
#if defined(__WIN32__) || defined(WIN32)

#define _WIN32_WINNT 0x0500
#include <windows.h>

void hide_console()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
}

#endif
