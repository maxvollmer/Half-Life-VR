
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
			ClientCmd("save quick");
		}
		break;
		case vr::EVRButtonId::k_EButton_SteamVR_Trigger:
		{
			downOrUp ? ClientCmd("+use") : ClientCmd("-use");
		}
		break;
		case vr::EVRButtonId::k_EButton_SteamVR_Touchpad:
		{

			if (vr_control_teleport->value == 1.f)
			{
				if (downOrUp && isTeleActive) {
					ServerCmd("vrtele 0");
					isTeleActive = false;
				}
			}
			else
				downOrUp ? ClientCmd("+jump") : ClientCmd("-jump");
		}
		break;
		case vr::EVRButtonId::k_EButton_Grip:
		{
			
	downOrUp ? ClientCmd("Impulse 100"): ClientCmd("Impulse");
			//downOrUp ? ServerCmd("vr_grabweapon 1") : ServerCmd("vr_grabweapon 0");

			//I decided not to atempt this now. A quick and dirty way to make two handed
			//	weapons is a quaternion/matrix lookat function. The weapon rotation is in
			//  Euler angles though, so a little bit of conversion is necessary. Needs to
			//	be done in UpdateGunPosition.
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
			//ServerCmd(downOrUp ? "vrtele 1" : "vrtele 0");
			vr::VRControllerAxis_t touchPadAxis = controllerState.rAxis[0];

			if (touchPadAxis.x < -0.5f && !downOrUp)
			{
				gHUD.m_Ammo.UserCmd_NextWeapon();
				gHUD.m_iKeyBits |= IN_ATTACK;
				gHUD.m_Ammo.Think();
			}
			else if (touchPadAxis.x > 0.5f && !downOrUp)
			{
				gHUD.m_Ammo.UserCmd_PrevWeapon();
				gHUD.m_iKeyBits |= IN_ATTACK;
				gHUD.m_Ammo.Think();
			}

			if (touchPadAxis.y > 0.5f && downOrUp)
			{
				ClientCmd("+reload");
			}
			else
			{
				ClientCmd("-reload");
			}

			if (touchPadAxis.y < -0.5f && downOrUp)
			{
				ServerCmd("vrtele 1");
			}
			else
			{
				ServerCmd("vrtele 0");
			}

		}
		break;
		}
	}
}

void VRInput::HandleTrackpad(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp)
{
	vr::VRControllerAxis_t touchPadAxis = controllerState.rAxis[0];
	downOrUp = (
		touchPadAxis.x < -vr_control_deadzone->value ||
		touchPadAxis.x > vr_control_deadzone->value ||
		touchPadAxis.y < -vr_control_deadzone->value ||
		touchPadAxis.y > vr_control_deadzone->value
		);

	if (leftOrRight && vr_control_teleport->value != 1.f)
	{
		if (vr_control_alwaysforward->value == 1.f)
			downOrUp ? ClientCmd("+forward") : ClientCmd("-forward");
		else
		{
			if (touchPadAxis.x < -vr_control_deadzone->value)
			{
				ClientCmd("+moveleft");
			}
			else
			{
				ClientCmd("-moveleft");
			}

			if (touchPadAxis.x > vr_control_deadzone->value)
			{
				ClientCmd("+moveright");
			}
			else
			{
				ClientCmd("-moveright");
			}

			if (touchPadAxis.y > vr_control_deadzone->value)
			{
				ClientCmd("+forward");
			}
			else
			{
				ClientCmd("-forward");
			}

			if (touchPadAxis.y < -vr_control_deadzone->value)
			{
				ClientCmd("+back");
			}
			else
			{
				ClientCmd("-back");
			}
		}
	}
	else if (leftOrRight && vr_control_teleport->value == 1.f)
	{
		if (downOrUp && !isTeleActive)
		{
			ServerCmd("vrtele 1");
			isTeleActive = true;
		}
		else if (!downOrUp && isTeleActive)
		{
			ServerCmd("vrtele 2");
			isTeleActive = false;
		}
	}
}

