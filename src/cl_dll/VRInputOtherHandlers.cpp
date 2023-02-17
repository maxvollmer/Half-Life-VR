
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
		void Other::HandleToggleVRMenu(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				if (g_vrInput.IsInMenu())
				{
					VRGameFunctions::CloseMenu();
				}
				else
				{
					VRGameFunctions::OpenMenu();
				}
			}
		}

		void Other::HandleClickVRMenu(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			bool on = data.bActive && data.bState;
			g_vrInput.SetVRMenuClickStatus(on);
		}

		void Other::HandleCustomAction(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				g_vrInput.ExecuteCustomAction(action);
			}
		}

	}  // namespace Input
}  // namespace VR
