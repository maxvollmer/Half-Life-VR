
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
#include "VRTextureHelper.h"
#include "VRGUIRenderer.h"

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

	unsigned int GetVRMenuTextureHandle()
	{
		//return gVRRenderer.GetHelper()->GetVRGLMenuTexture();
		return gVRRenderer.GetHelper()->GetVRGLGUITexture();
		//return VRTextureHelper::Instance().GetTexture("vr_menu_placeholder.png");
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

	int vr_menu_placement = (int)CVAR_GET_FLOAT("vr_menu_placement");
	float scale = CVAR_GET_FLOAT("vr_menu_scale");
	float distance = CVAR_GET_FLOAT("vr_menu_distance");
	float opacity = CVAR_GET_FLOAT("vr_menu_opacity");

	if (scale < 0.1f) scale = 0.1f;
	if (scale > 10.f) scale = 10.f;

	if (distance < -10.f) distance = -10.f;
	if (distance > 10.f) distance = 10.f;

	if (opacity < 0.1f) opacity = 0.1f;
	if (opacity > 1.f) opacity = 1.f;

	vr::Texture_t texture;
	texture.eColorSpace = vr::ColorSpace_Gamma;
	texture.eType = vr::TextureType_OpenGL;
	texture.handle = reinterpret_cast<void*>(GetVRMenuTextureHandle());
	error = vr::VROverlay()->SetOverlayTexture(m_hlMenu, &texture);
	if (error != vr::VROverlayError_None)
	{
		gEngfuncs.Con_DPrintf("Could not set texture for HL menu overlay: %s\n", vr::VROverlay()->GetOverlayErrorNameFromEnum(error));
	}

	vr::VRTextureBounds_t textureBounds = { 0.f, 0.f, 1.f, 1.f };
	vr::VROverlay()->SetOverlayTextureBounds(m_hlMenu, &textureBounds);

	vr::VROverlay()->SetOverlayAlpha(m_hlMenu, opacity);  // no error handling, we don't care if this fails

	vr::HmdMatrix34_t transform =
	{
		scale, 0.0f, 0.0f, 0.f,
		0.0f, scale, 0.0f, 0.f,
		0.0f, 0.0f, scale, -distance
	};

	vr::TrackedDeviceIndex_t menuDeviceIndex;
	switch (vr_menu_placement)
	{
	case 1:
		menuDeviceIndex = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
		break;
	case 2:
		menuDeviceIndex = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);
		break;
	case 0:
	default:
		menuDeviceIndex = vr::k_unTrackedDeviceIndex_Hmd;
		break;
	}

	error = vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_hlMenu, menuDeviceIndex, &transform);
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

	// Annoyingly Valve removed support for interactive overlays in apps in 2018.
	// Just why Valve?
#if 0
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
#endif

	bool useRightController;
	int vr_menu_placement = (int)CVAR_GET_FLOAT("vr_menu_placement");
	switch (vr_menu_placement)
	{
	case 1: // left hand
		useRightController = true;
		break;
	case 2: // right hand
		useRightController = false;
		break;
	case 0: // hmd
	default:
		useRightController = CVAR_GET_FLOAT("vr_lefthand_mode") == 0.f;
		break;
	}

	Vector position = useRightController ? gVRRenderer.GetRightControllerPosition() : gVRRenderer.GetLeftControllerPosition();
	Vector direction = useRightController ? gVRRenderer.GetRightControllerForward() : gVRRenderer.GetLeftControllerForward();
	bool pressed = useRightController ? g_vrInput.IsDragOn(vr::TrackedControllerRole_RightHand) : g_vrInput.IsDragOn(vr::TrackedControllerRole_LeftHand);

	vr::VROverlayIntersectionParams_t params;
	params.eOrigin = vr::TrackingUniverseStanding;
	for (int i = 0; i < 3; i++)
	{
		params.vSource.v[i] = position[i];
		params.vDirection.v[i] = direction[i];
	}

	VRGUIRenderer::VRControllerState uiControllerState;
	uiControllerState.isPressed = pressed;

	vr::VROverlayIntersectionResults_t results;
	if (vr::VROverlay()->ComputeOverlayIntersection(m_hlMenu, &params, &results))
	{
		uiControllerState.isOverGUI = true;
		uiControllerState.x = results.vUVs.v[0];
		uiControllerState.y = 1.f - results.vUVs.v[1];
	}
	else
	{
		uiControllerState.isOverGUI = false;
	}

	VRGUIRenderer::Instance().UpdateControllerState(uiControllerState);
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
	texture.handle = reinterpret_cast<void*>(GetVRMenuTextureHandle());
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
