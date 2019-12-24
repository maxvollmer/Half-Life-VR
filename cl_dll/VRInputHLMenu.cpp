
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
#include "VRHelper.h"

#include "IGameConsole.h"

namespace
{
	IGameConsole* gGameConsole = nullptr;
	bool IsConsoleOpen()
	{
		if (!gGameConsole)
		{
			auto gameuiModule = Sys_LoadModule("gameui.dll");
			if (gameuiModule)
			{
				CreateInterfaceFn gameUIFactory = Sys_GetFactory(gameuiModule);
				if (gameUIFactory)
				{
					int returnCode = 0;
					gGameConsole = dynamic_cast<IGameConsole*>(gameUIFactory(GAMECONSOLE_INTERFACE_VERSION, &returnCode));
				}
			}
		}
		return gGameConsole && gGameConsole->IsConsoleVisible();
	}
}  // namespace

void VRInput::CreateHLMenu()
{
	auto error = vr::VROverlay()->CreateOverlay("HalfLifeMenu", "HalfLifeMenu", &m_hlMenu);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not create HL menu overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
		return;
	}

	// TODO: Make these cvars and handle changes
	float width = 1.f;
	float distance = 1.f;
	float opacity = 0.9f;

	vr::Texture_t texture;
	texture.eColorSpace = vr::ColorSpace_Gamma;
	texture.eType = vr::TextureType_OpenGL;
	texture.handle = reinterpret_cast<void*>(gVRRenderer.GetHelper()->GetVRGLMenuTexture());
	error = vr::VROverlay()->SetOverlayTexture(m_hlMenu, &texture);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not set texture for HL menu overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
	}

	vr::VRTextureBounds_t textureBounds = { 0.f, 0.f, 1.f, 1.f };
	vr::VROverlay()->SetOverlayTextureBounds(m_hlMenu, &textureBounds);

	vr::VROverlay()->SetOverlayAlpha(m_hlMenu, opacity);  // no error handling, we don't care if this fails

	vr::HmdMatrix34_t transform = {
		1.f, 0.0f, 0.0f, 0.f, 0.0f, 1.f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, -1.f };
	error = vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_hlMenu, vr::k_unTrackedDeviceIndex_Hmd, &transform);
	//error = vr::VROverlay()->SetOverlayTransformAbsolute(m_hlMenu, vr::TrackingUniverseStanding, &transform);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not set transform for HL menu overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
	}

	error = vr::VROverlay()->SetOverlayInputMethod(m_hlMenu, vr::VROverlayInputMethod_Mouse);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not set input mode for HL menu overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
	}

	vr::HmdVector2_t mouseScale{ 1.f, 1.f };
	error = vr::VROverlay()->SetOverlayMouseScale(m_hlMenu, &mouseScale);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not set mouse scale for HL menu overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
	}
}

void VRInput::HandleHLMenuInput()
{
	vr::VREvent_t event{ 0 };
	while (vr::VROverlay()->PollNextOverlayEvent(m_hlMenu, &event, sizeof(vr::VREvent_t)))
	{
		switch (event.eventType)
		{
		case vr::VREvent_MouseMove:
		case vr::VREvent_TouchPadMove:
			SendMousePosToHLWindow(event.data.mouse.x, event.data.mouse.y);
			break;
		case vr::VREvent_MouseButtonDown:
			SendMouseButtonClickToHLWindow(event.data.mouse.x, event.data.mouse.y);
			break;
		case vr::VREvent_MouseButtonUp:
			//SendMouseButtonUpToHLWindow(event.data.mouse.x, event.data.mouse.y);
			break;
		case vr::VREvent_KeyboardCharInput:
			//event.data.keyboard.cNewInput
			gEngfuncs.Con_DPrintf("Input: %s\n", event.data.keyboard.cNewInput);
			break;
		case vr::VREvent_KeyboardDone:
		case vr::VREvent_KeyboardClosed:
			m_vrMenuKeyboardOpen = false;
			break;
		default:
			/*ignore*/
			break;
		}
	}
}

void VRInput::ShowHLMenu()
{
	if (m_isHLMenuShown)
	{
		HandleHLMenuInput();
	}

	if (m_hlMenu == vr::k_ulOverlayHandleInvalid)
	{
		CreateHLMenu();
	}

	if (IsConsoleOpen())
	{
		if (!m_vrMenuKeyboardOpen)
		{
			//auto error = vr::VROverlay()->ShowKeyboard(vr::k_EGamepadTextInputModeNormal, vr::k_EGamepadTextInputLineModeSingleLine, "HL:VR Console Input Keyboard", 1024, "", false, 0);
			//if (error != vr::VROverlayError_None && error != vr::VROverlayError_KeyboardAlreadyInUse)
			{
				//	gEngfuncs.Con_DPrintf("Could not show keyboard overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
			}
			// set to true even if we got an error, otherwise console gets spammed with the error message every frame
			m_vrMenuKeyboardOpen = true;
		}
	}
	else
	{
		if (m_vrMenuKeyboardOpen)
		{
			//vr::VROverlay()->HideKeyboard();
		}
		m_vrMenuKeyboardOpen = false;
	}

	auto error = vr::VROverlay()->ShowOverlay(m_hlMenu);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not show HL menu overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
	}

	vr::Texture_t texture;
	texture.eColorSpace = vr::ColorSpace_Auto;
	texture.eType = vr::TextureType_OpenGL;
	texture.handle = reinterpret_cast<void*>(gVRRenderer.GetHelper()->GetVRGLMenuTexture());
	error = vr::VROverlay()->SetOverlayTexture(m_hlMenu, &texture);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not update HL menu overlay texture: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
	}

	m_isHLMenuShown = true;
}

void VRInput::HideHLMenu()
{
	if (!m_isHLMenuShown)
		return;

	auto error = vr::VROverlay()->HideOverlay(m_hlMenu);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not hide HL menu overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
		return;
	}

	vr::VROverlay()->HideKeyboard();

	m_isHLMenuShown = false;
}
