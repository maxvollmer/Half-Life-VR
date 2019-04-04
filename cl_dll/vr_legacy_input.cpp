
#include <windows.h>
#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "vr_input.h"

void VRInput::LegacyHandleButtonPress(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp)
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
			if (gHUD.m_Flash.IsOn() != downOrUp)
			{
				ClientCmd("impulse 100");
			}
			//downOrUp ? ClientCmd("impulse 1337") : ClientCmd("impulse 1338");
		}
		break;
		case vr::EVRButtonId::k_EButton_SteamVR_Touchpad:
		{
			if (CVAR_GET_FLOAT("vr_movecontrols") != 0.0f)
			{
				vr::VRControllerAxis_t touchPadAxis = controllerState.rAxis[vr::EVRButtonId::k_EButton_SteamVR_Touchpad - vr::EVRButtonId::k_EButton_Axis0];

				// TODO: Move in direction controller is pointing, not direction player is looking!

				if (touchPadAxis.x < -0.5f)
				{
					m_rotateLeft = downOrUp;
				}
				if (touchPadAxis.x > 0.5f)
				{
					m_rotateRight = downOrUp;
				}

				/*
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
				*/

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

				if (fabs(touchPadAxis.x) < 0.5f && fabs(touchPadAxis.y) < 0.5f && downOrUp)
				{
					ServerCmd("vrtele 1");
				}
				else if (!downOrUp)
				{
					ServerCmd("vrtele 0");
				}
			}
			else
			{
				ServerCmd(downOrUp ? "vrtele 1" : "vrtele 0");
			}
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

			if (touchPadAxis.y > 0.5f && downOrUp)
			{
				if (gHUD.m_Ammo.IsCurrentWeaponLastWeapon())
				{
					// Select hand when current weapon is last weapon
					ServerCmd("weapon_barehand");
					PlaySound("common/wpn_select.wav", 1);
					//PlaySound("common/wpn_hudon.wav", 1);
					gHUD.m_Ammo.m_pWeapon = nullptr;
				}
				else
				{
					gHUD.m_Ammo.UserCmd_NextWeapon();
					gHUD.m_iKeyBits |= IN_ATTACK;
					gHUD.m_Ammo.Think();
				}
			}
			else if (touchPadAxis.y < -0.5f && downOrUp)
			{
				if (gHUD.m_Ammo.IsCurrentWeaponFirstWeapon())
				{
					// Select hand when current weapon is first weapon
					ServerCmd("weapon_barehand");
					PlaySound("common/wpn_select.wav", 1);
					//PlaySound("common/wpn_moveselect.wav", 1);
					gHUD.m_Ammo.m_pWeapon = nullptr;
				}
				else
				{
					gHUD.m_Ammo.UserCmd_PrevWeapon();
					gHUD.m_iKeyBits |= IN_ATTACK;
					gHUD.m_Ammo.Think();
				}
			}
		}
		break;
		}
	}
}

void VRInput::LegacyHandleButtonTouch(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp)
{
	/*noop*/
}
