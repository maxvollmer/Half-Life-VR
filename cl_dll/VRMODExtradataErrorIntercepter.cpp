
#include "VRMODExtradataErrorIntercepter.h"

#include <string>
#include <Windows.h>

std::string g_ModExtradataModelName;

namespace
{
	HHOOK hMsgBoxHook;

	BOOL CALLBACK FindMod_ExtradataErrorMessageChildWindow(_In_ HWND hwnd, _In_ LPARAM lParam)
	{
		char _text[1024]{ 0 };
		GetWindowTextA(hwnd, _text, sizeof(_text) - 1);
		std::string text{ _text };
		if (text == "Mod_Extradata: caching failed")
		{
			*((HWND*)lParam) = hwnd;
			return FALSE;
		}
		return EnumChildWindows(hwnd, FindMod_ExtradataErrorMessageChildWindow, lParam);
	}

	LRESULT CALLBACK MsgBoxProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode != HCBT_ACTIVATE)
		{
			return CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);
		}

		HWND hwndMsgBox = (HWND)wParam;

		char _title[1024]{ 0 };
		GetWindowTextA(hwndMsgBox, _title, sizeof(_title) - 1);
		std::string title{ _title };

		if (title == "Fatal Error")
		{
			HWND hwndLabel = NULL;
			EnumChildWindows(hwndMsgBox, FindMod_ExtradataErrorMessageChildWindow, (LPARAM)&hwndLabel);

			if (hwndLabel)
			{
				std::string errorMessageWithModelName = ("Mod_Extradata: caching failed: " + g_ModExtradataModelName);

				RECT rect{ 0 };
				DrawTextA(GetDC(hwndLabel), errorMessageWithModelName.data(), errorMessageWithModelName.size(), &rect, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
				int neededWidth = abs(rect.right - rect.left) * 2;

				GetWindowRect(hwndLabel, &rect);
				SetWindowPos(hwndLabel, HWND_TOP, 0, 0, max(neededWidth, abs(rect.right - rect.left)), abs(rect.bottom - rect.top), SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE);

				GetWindowRect(hwndMsgBox, &rect);
				SetWindowPos(hwndMsgBox, HWND_TOP, 0, 0, max(neededWidth, abs(rect.right - rect.left)), abs(rect.bottom - rect.top), SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE);

				SetWindowTextA(hwndMsgBox, (title + ": " + g_ModExtradataModelName).data());
				SetWindowTextA(hwndLabel, errorMessageWithModelName.data());
			}

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
