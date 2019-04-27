
#include "VRMODExtradataErrorIntercepter.h"

#include <Windows.h>
#include <string>

std::string g_ModExtradataModelName;

namespace
{
	HHOOK hMsgBoxHook;

	LRESULT CALLBACK MsgBoxProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode != HCBT_ACTIVATE)
		{
			return CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);
		}

		HWND hwndMsgBox = (HWND)wParam;
		HWND hwndLabel = GetDlgItem(hwndMsgBox, 0xFFFF);

		char _title[1024]{ 0 };
		GetWindowTextA(hwndMsgBox, _title, sizeof(_title) - 1);
		std::string title{ _title };

		char _text[1024]{ 0 };
		GetWindowTextA(hwndLabel, _text, sizeof(_text) - 1);
		std::string text{ _text };

		if (title == "Fatal Error" && text == "Mod_Extradata: caching failed")
		{
			SetWindowTextA(hwndMsgBox, (title + ": " + g_ModExtradataModelName).data());

			SetWindowTextA(hwndLabel, (text + ": " + g_ModExtradataModelName).data());

			RECT rect{ 0 };
			GetWindowRect(hwndLabel, &rect);
			SetWindowPos(hwndLabel, HWND_TOP, 0, 0, (rect.right - rect.left) + 500, (rect.bottom - rect.top), SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE);

			GetWindowRect(hwndMsgBox, &rect);
			SetWindowPos(hwndMsgBox, HWND_TOP, 0, 0, (rect.right - rect.left) + 500, (rect.bottom - rect.top), SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE);

			return 0;
		}
		else
		{
			return CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);
		}
	}
}

VRMODExtradataErrorIntercepter::VRMODExtradataErrorIntercepter()
{
	hMsgBoxHook = SetWindowsHookExA(
		WH_CBT,
		MsgBoxProc,
		NULL,
		GetCurrentThreadId()
	);
}

VRMODExtradataErrorIntercepter::~VRMODExtradataErrorIntercepter()
{
	UnhookWindowsHookEx(hMsgBoxHook);
}

VRMODExtradataErrorIntercepter VRMODExtradataErrorIntercepter::m_instance{};
