
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>

#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "vr_input.h"
#include "eiface.h"

namespace VR
{
	namespace Input
	{
		void Other::HandleTeleport(vr::InputDigitalActionData_t data, const std::string& action)
		{
			if (data.bChanged)
			{
				bool on = data.bActive && data.bState;
				// TODO: Send telestart or telestop event to server
			}
		}

		void Other::HandleFlashlight(vr::InputDigitalActionData_t data, const std::string& action)
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

		void Other::HandleGrab(vr::InputDigitalActionData_t data, const std::string& action)
		{
			//g_vrInput.
		}

		void Other::HandleLegacyUse(vr::InputDigitalActionData_t data, const std::string& action)
		{
			if (data.bChanged)
			{
				bool on = data.bActive && data.bState;
				ClientCmd(on ? "+use" : "-use");
			}
		}

		void Other::HandleQuickSave(vr::InputDigitalActionData_t data, const std::string& action)
		{
		}

		void Other::HandleQuickLoad(vr::InputDigitalActionData_t data, const std::string& action)
		{
		}

		void Other::HandleRestartCurrentMap(vr::InputDigitalActionData_t data, const std::string& action)
		{
		}

		void Other::HandlePauseGame(vr::InputDigitalActionData_t data, const std::string& action)
		{
		}

		void Other::HandleExitGame(vr::InputDigitalActionData_t data, const std::string& action)
		{
		}

		void Other::HandleCustomAction(vr::InputDigitalActionData_t data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				g_vrInput.ExecuteCustomAction(action);
			}
		}
	}
}
