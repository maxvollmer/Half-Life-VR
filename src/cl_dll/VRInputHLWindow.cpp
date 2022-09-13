
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>
#include <fstream>
#include <cctype>
#include <regex>

#include "hud.h"
#include "cl_util.h"
#include "VRInput.h"
#include "eiface.h"
#include "VRCommon.h"
#include "VRRenderer.h"

// TODO: Maybe add support for other systems as well?
#include <windows.h>

namespace
{
	HWND hlWindow{ nullptr };

	BOOL CALLBACK VRFindHalflifeWindow(_In_ HWND hwnd, _In_ LPARAM lParam)
	{
		if (IsWindowVisible(hwnd))
		{
			DWORD procId = GetCurrentProcessId();
			DWORD windowProcId;
			GetWindowThreadProcessId(hwnd, &windowProcId);
			if (procId == windowProcId)
			{
				RECT rect;
				GetWindowRect(hwnd, &rect);
				if (rect.right > rect.left&& rect.bottom > rect.top)
				{
					hlWindow = hwnd;
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	void VRFindHLWindow()
	{
		if (!IsWindow(hlWindow) || !IsWindowVisible(hlWindow))
		{
			EnumWindows(VRFindHalflifeWindow, 0);
		}
	}

	void VRGetAbsoluteHLWindowCoords(float x, float y, int* ix, int* iy)
	{
		VRFindHLWindow();
		RECT rect;
		GetWindowRect(hlWindow, &rect);
		int width = (rect.right - rect.left);
		int height = (rect.bottom - rect.top);
		*ix = int(x * width);
		*iy = int(y * height);
	}

	void VRSendMouseClickToHLWindow(float x, float y);
	void VRSendMouseMessageToHLWindow(UINT msg, float x, float y)
	{
		int ix, iy;
		VRGetAbsoluteHLWindowCoords(x, y, &ix, &iy);
		PostMessageA(hlWindow, msg, 0, MAKELPARAM(ix, iy));

		VRSendMouseClickToHLWindow(x, y);
	}

	bool blatest = false;
	void VRSendMouseClickToHLWindow(float x, float y)
	{
		// With bits from here: https://stackoverflow.com/a/5790851/9199167
		VRFindHLWindow();

		bool mouseButtonsSwapped = GetSystemMetrics(SM_SWAPBUTTON) != 0;

		const float XSCALEFACTOR = 65535.f / (GetSystemMetrics(SM_CXSCREEN) - 1.f);
		const float YSCALEFACTOR = 65535.f / (GetSystemMetrics(SM_CYSCREEN) - 1.f);

		HWND hPreviousForegroundWindow = GetForegroundWindow();

		SetActiveWindow(hlWindow);
		SetForegroundWindow(hlWindow);

		RECT rect;
		GetWindowRect(hlWindow, &rect);

		RECT clientRect;
		GetClientRect(hlWindow, &clientRect);

		POINT cursorPos;
		GetCursorPos(&cursorPos);

		float previousCursorX = cursorPos.x * XSCALEFACTOR;
		float previousCursorY = cursorPos.y * YSCALEFACTOR;

		int ix, iy;
		VRGetAbsoluteHLWindowCoords(x, y, &ix, &iy);

		INPUT input{ 0 };
		input.type = INPUT_MOUSE;
		input.mi.dx = (ix + rect.left) * XSCALEFACTOR;
		input.mi.dy = (iy + rect.top) * YSCALEFACTOR;
		input.mi.mouseData = 0;
		input.mi.time = 0;
		input.mi.dwExtraInfo = 0;
		input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
		if (blatest)
		{
			if (mouseButtonsSwapped)
			{
				input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP;
			}
			else
			{
				input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
			}
		}
		SendInput(1, &input, sizeof(INPUT));

		input.mi.dx = previousCursorX;
		input.mi.dy = previousCursorY;
		input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
		//SendInput(1, &input, sizeof(INPUT));

		SetForegroundWindow(hPreviousForegroundWindow);
	}

	void VRForceWindowToForeground()
	{
		VRFindHLWindow();
		if (hlWindow != GetFocus())
		{
			SwitchToThisWindow(hlWindow, TRUE);
		}
	}

	void VRDisplayErrorPopup(const char* errorMessage)
	{
		VRFindHLWindow();
		MessageBoxA(hlWindow, errorMessage, "Error starting Half-Life: VR", MB_OK);
	}
}  // namespace

void VRInput::SendMousePosToHLWindow(float x, float y)
{
	VRSendMouseMessageToHLWindow(WM_MOUSEMOVE, x, y);
}

void VRInput::SendMouseButtonClickToHLWindow(float x, float y)
{
	VRSendMouseClickToHLWindow(x, y);
}

void VRInput::DisplayErrorPopup(const char* errorMessage)
{
	VRDisplayErrorPopup(errorMessage);
}

void VRInput::ForceWindowToForeground()
{
	VRForceWindowToForeground();
}
