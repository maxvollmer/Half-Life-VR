
#ifdef _WIN32

#include "EasyHook/include/easyhook.h"

#include <Windows.h>
#include <algorithm>

namespace
{
	bool g_wasthreadattached = false;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_DETACH)
	{
		// We need to uninstall all EasyHook hooks here.
		// EasyHook32.dll calls LhUninstallAllHooks when it is unloaded,
		// but hooks might be invalid by then as DLLs we hooked might be unloaded already,
		// which causes a crash when EasyHook calls IsBadReadPtr on every hook.
		// (See also https://devblogs.microsoft.com/oldnewthing/20060927-07/?p=29563)
		if (g_wasthreadattached)
		{
			LhUninstallAllHooks();
			extern bool g_fmodIgnoreEverythingWeAreShuttingDown;
			g_fmodIgnoreEverythingWeAreShuttingDown = true; // ah, beautifuly wacky-hacky wackhacks
			std::exit(0);
		}
	}
	if (fdwReason == DLL_THREAD_ATTACH)
	{
		g_wasthreadattached = true;
	}
	return TRUE;
}

#endif
