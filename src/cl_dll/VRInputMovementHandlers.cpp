
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>

#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "VRInput.h"
#include "VRRenderer.h"
#include "VRHelper.h"
#include "eiface.h"

namespace VR
{
	namespace Input
	{

		void TryAddInAnalogSpeed(float& target, const float& inputValue, const char* cvar_inverted)
		{
			if (fabs(inputValue) > CVAR_GET_FLOAT("vr_move_analog_deadzone"))
			{
				target += (CVAR_GET_FLOAT(cvar_inverted) != 0.f) ? -inputValue : inputValue;
			}
		}

		void Movement::HandleMove(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			if (data.bActive)
			{
				// y is forward, x is sideways
				TryAddInAnalogSpeed(g_vrInput.analogforward, data.y, "vr_move_analogforward_inverted");
				TryAddInAnalogSpeed(g_vrInput.analogsidemove, data.x, "vr_move_analogsideways_inverted");
			}
		}

		void Movement::HandleContinuousTurn(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			if (data.bActive && CVAR_GET_FLOAT("vr_move_snapturn_enabled") == 0.f)
			{
				TryAddInAnalogSpeed(g_vrInput.analogturn, -data.x, "vr_move_analogturn_inverted");
			}
		}

		void Movement::HandleTurnLeft(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged && CVAR_GET_FLOAT("vr_move_snapturn_enabled") != 0.f)
			{
				float angle = CVAR_GET_FLOAT("vr_move_snapturn_angle");
				gVRRenderer.GetHelper()->InstantRotateYaw(angle);
			}
		}

		void Movement::HandleTurnRight(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged && CVAR_GET_FLOAT("vr_move_snapturn_enabled") != 0.f)
			{
				float angle = CVAR_GET_FLOAT("vr_move_snapturn_angle");
				gVRRenderer.GetHelper()->InstantRotateYaw(-angle);
			}
		}

		void Movement::HandleJump(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+jump");
				else
					ClientCmd("-jump");
			}
		}

		void Movement::HandleCrouch(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (CVAR_GET_FLOAT("vr_toggle_crouch") != 0.f)
			{
				g_vrInput.m_crouchState = data.bActive && data.bState;
			}
			else
			{
				g_vrInput.m_crouchState = false;
				if (data.bChanged)
				{
					if (data.bState)
					{
						ClientCmd("+duck");
					}
					else
					{
						ClientCmd("-duck");
					}
				}
			}
		}

		void Movement::HandleWalk(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (CVAR_GET_FLOAT("vr_togglewalk") != 0.f)
				{
					if (data.bState)
						ClientCmd("+speed");
					else
						ClientCmd("-speed");
				}
				else
				{
					extern kbutton_t in_speed;
					if (in_speed.state & 1)
					{
						in_speed.down[0] = in_speed.down[1] = 0;
						in_speed.state = 0;
					}
					else
					{
						in_speed.down[0] = in_speed.down[1] = 0;
						in_speed.state = 1;
					}
				}
			}
		}

	}  // namespace Input
}  // namespace VR
