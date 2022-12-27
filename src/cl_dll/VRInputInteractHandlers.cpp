
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>

#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "VRInput.h"
#include "eiface.h"
#include "VRRenderer.h"
#include "VRGameFunctions.h"

namespace VR
{
	namespace Input
	{
		void Interact::HandleGrab(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			bool on = data.bActive && data.bState;
			g_vrInput.SetDrag(g_vrInput.GetRole(data.activeOrigin), on);
		}

		void Interact::HandleLegacyUse(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				bool on = data.bActive && data.bState;
				if (on)
				{
					// don't +on if in menu!
					if (g_vrInput.IsInGame() && !g_vrInput.IsInMenu())
					{
						ClientCmd("+use");
					}
				}
				else
				{
					ClientCmd("-use");
				}
			}
		}

		void Interact::HandleTeleport(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				bool on = data.bActive && data.bState;
				ServerCmd(on ? "vrtele 1" : "vrtele 0");
			}
		}

		void Interact::HandleFlashlight(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (CVAR_GET_FLOAT("vr_flashlight_toggle") != 0.f)
			{
				if (data.bActive && data.bState && data.bChanged)
				{
					ClientCmd("impulse 100");
				}
			}
			else
			{
				bool on = data.bActive && data.bState;
				if (gHUD.m_Flash.IsOn() != on)
				{
					ClientCmd("impulse 100");
				}
			}
		}

	}  // namespace Input
}  // namespace VR
