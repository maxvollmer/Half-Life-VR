
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

namespace VR
{
	namespace Input
	{
		void Other::HandleTeleport(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				bool on = data.bActive && data.bState;
				ServerCmd(on ? "vrtele 1" : "vrtele 0");
			}
		}

		void Other::HandleFlashlight(const vr::InputDigitalActionData_t& data, const std::string& action)
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

		void Other::HandleLeftGrab(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			bool on = data.bActive && data.bState;
			g_vrInput.SetDrag(vr::TrackedControllerRole_LeftHand, on);
		}

		void Other::HandleRightGrab(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			bool on = data.bActive && data.bState;
			g_vrInput.SetDrag(vr::TrackedControllerRole_RightHand, on);
		}

		void Other::HandleLegacyUse(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				bool on = data.bActive && data.bState;
				ClientCmd(on ? "+use" : "-use");
			}
		}

		void Other::HandleLetGoOffLadder(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			bool on = data.bActive && data.bState;
			g_vrInput.SetLetGoOffLadder(on);
		}

		void Other::HandleQuickSave(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				ClientCmd("save quick");
			}
		}

		void Other::HandleQuickLoad(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				ClientCmd("load quick");
			}
		}

		void Other::HandleRestartCurrentMap(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				ClientCmd("vr_restartmap");
			}
		}

		void Other::HandlePauseGame(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				ClientCmd("pause");
			}
		}

		void Other::HandleExitGame(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				ClientCmd("quit");
			}
		}

		void Other::HandleCustomAction(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				g_vrInput.ExecuteCustomAction(action);
			}
		}

		void Other::HandleLeftHandSkeleton(const vr::VRSkeletalSummaryData_t& data, const std::string& action)
		{
			g_vrInput.SetFingerSkeletalData(vr::TrackedControllerRole_LeftHand, data.flFingerCurl);
		}

		void Other::HandleRightHandSkeleton(const vr::VRSkeletalSummaryData_t& data, const std::string& action)
		{
			g_vrInput.SetFingerSkeletalData(vr::TrackedControllerRole_RightHand, data.flFingerCurl);
		}

	}  // namespace Input
}  // namespace VR
