
#include <windows.h>
#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "vr_input.h"

VRInput g_vrInput;

VRInput::VRInput()
{

}

VRInput::~VRInput()
{

}

void VRInput::HandleButtonPress(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp)
{
	if (leftOrRight)
	{
		switch (button)
		{
		case vr::EVRButtonId::k_EButton_ApplicationMenu:
		{
			ClientCmd("escape");
		}
		break;
		case vr::EVRButtonId::k_EButton_SteamVR_Trigger:
		{
			//downOrUp ? ClientCmd("+jump") : ClientCmd("-jump");
		}
		break;
		case vr::EVRButtonId::k_EButton_SteamVR_Touchpad:
		{
			ServerCmd(downOrUp?"vrtele 1":"vrtele 0");

			/*
			vr::VRControllerAxis_t touchPadAxis = controllerState.rAxis[vr::EVRButtonId::k_EButton_SteamVR_Touchpad - vr::EVRButtonId::k_EButton_Axis0];

			// TODO: Move in direction controller is pointing, not direction player is looking!

			if (touchPadAxis.x < -0.5f && downOrUp)
			{
				ClientCmd("+moveleft");
			}
			else
			{
				ClientCmd("-moveleft");
			}

			if (touchPadAxis.x > 0.5f && downOrUp)
			{
				ClientCmd("+moveright");
			}
			else
			{
				ClientCmd("-moveright");
			}

			if (touchPadAxis.y > 0.5f && downOrUp)
			{
				ClientCmd("+forward");
			}
			else
			{
				ClientCmd("-forward");
			}

			if (touchPadAxis.y < -0.5f && downOrUp)
			{
				ClientCmd("+back");
			}
			else
			{
				ClientCmd("-back");
			}
			*/
		}
		break;
		}
	}
	else
	{
		switch (button)
		{
		case vr::EVRButtonId::k_EButton_Grip:
		{
			downOrUp ? ClientCmd("+reload") : ClientCmd("-reload");
		}
		break;
		case vr::EVRButtonId::k_EButton_ApplicationMenu:
		{
			downOrUp ? ClientCmd("+attack2") : ClientCmd("-attack2");
		}
		break;
		case vr::EVRButtonId::k_EButton_SteamVR_Trigger:
		{
			downOrUp ? ClientCmd("+attack") : ClientCmd("-attack");
		}
		break;
		case vr::EVRButtonId::k_EButton_SteamVR_Touchpad:
		{
			vr::VRControllerAxis_t touchPadAxis = controllerState.rAxis[vr::EVRButtonId::k_EButton_SteamVR_Touchpad - vr::EVRButtonId::k_EButton_Axis0];

			if (touchPadAxis.y > 0.5f && !downOrUp)
			{
				gHUD.m_Ammo.UserCmd_NextWeapon();
				gHUD.m_iKeyBits |= IN_ATTACK;
				gHUD.m_Ammo.Think();
			}
			else if (touchPadAxis.y < -0.5f && !downOrUp)
			{
				gHUD.m_Ammo.UserCmd_PrevWeapon();
				gHUD.m_iKeyBits |= IN_ATTACK;
				gHUD.m_Ammo.Think();
			}
		}
		break;
		}
	}
}
