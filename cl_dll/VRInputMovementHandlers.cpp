
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
		void Movement::HandleMoveForward(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+forward");
				else
					ClientCmd("-forward");
			}
		}

		void Movement::HandleMoveBackward(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+back");
				else
					ClientCmd("-back");
			}
		}

		void Movement::HandleMoveLeft(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+moveleft");
				else
					ClientCmd("-moveleft");
			}
		}

		void Movement::HandleMoveRight(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+moveright");
				else
					ClientCmd("-moveright");
			}
		}

		void Movement::HandleMoveUp(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+moveup");
				else
					ClientCmd("-moveup");
			}
		}

		void Movement::HandleMoveDown(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+movedn");
				else
					ClientCmd("-movedn");
			}
		}

		void Movement::HandleTurnLeft(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			g_vrInput.m_rotateLeft = data.bActive && data.bState;
		}

		void Movement::HandleTurnRight(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			g_vrInput.m_rotateRight = data.bActive && data.bState;
		}

		void Movement::HandleTurn45Left(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				gVRRenderer.GetHelper()->InstantRotateYaw(45.f);
			}
		}

		void Movement::HandleTurn45Right(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				gVRRenderer.GetHelper()->InstantRotateYaw(-45.f);
			}
		}

		void Movement::HandleTurn90Left(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				gVRRenderer.GetHelper()->InstantRotateYaw(90.f);
			}
		}

		void Movement::HandleTurn90Right(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				gVRRenderer.GetHelper()->InstantRotateYaw(-90.f);
			}
		}

		void Movement::HandleTurn180(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				gVRRenderer.GetHelper()->InstantRotateYaw(180.f);
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
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+duck");
				else
					ClientCmd("-duck");
			}
		}

		void Movement::HandleLongJump(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				ClientCmd("vr_lngjump");
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

		void Movement::HandleAnalogJump(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			if (data.bActive && fabs(data.deltaX) > EPSILON)
			{
				if (fabs(data.x) > CVAR_GET_FLOAT("vr_move_analog_deadzone"))
					ClientCmd("+jump");
				else
					ClientCmd("-jump");
			}
		}

		void Movement::HandleAnalogCrouch(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			if (data.bActive && fabs(data.deltaX) > EPSILON)
			{
				if (fabs(data.x) > CVAR_GET_FLOAT("vr_move_analog_deadzone"))
					ClientCmd("+duck");
				else
					ClientCmd("-duck");
			}
		}

		void Movement::HandleAnalogLongJump(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			if (data.bActive && fabs(data.x) > EPSILON)
			{
				if (fabs(data.x) > CVAR_GET_FLOAT("vr_move_analog_deadzone"))
					ClientCmd("vr_lngjump");
			}
		}

		void TryAddInAnalogSpeed(float& target, const float& inputValue, const char* cvar_inverted)
		{
			if (fabs(inputValue) > CVAR_GET_FLOAT("vr_move_analog_deadzone"))
			{
				target += (CVAR_GET_FLOAT(cvar_inverted) != 0.f) ? -inputValue : inputValue;
			}
		}

		void Movement::HandleMoveForwardBackward(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			TryAddInAnalogSpeed(g_vrInput.analogforward, data.x, "vr_move_analogforward_inverted");
		}

		void Movement::HandleMoveSideways(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			TryAddInAnalogSpeed(g_vrInput.analogsidemove, data.x, "vr_move_analogsideways_inverted");
		}

		void Movement::HandleMoveUpDown(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			TryAddInAnalogSpeed(g_vrInput.analogupmove, data.x, "vr_move_analogupdown_inverted");
		}

		void Movement::HandleTurn(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			TryAddInAnalogSpeed(g_vrInput.analogturn, data.x, "vr_move_analogturn_inverted");
		}

		void Movement::HandleMoveForwardBackwardSideways(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			// y is forward, x is sideways
			TryAddInAnalogSpeed(g_vrInput.analogforward, data.y, "vr_move_analogforward_inverted");
			TryAddInAnalogSpeed(g_vrInput.analogsidemove, data.x, "vr_move_analogsideways_inverted");
		}

		void Movement::HandleMoveForwardBackwardTurn(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			// y is forward, x is turn
			TryAddInAnalogSpeed(g_vrInput.analogforward, data.y, "vr_move_analogforward_inverted");
			TryAddInAnalogSpeed(g_vrInput.analogturn, data.x, "vr_move_analogturn_inverted");
		}

		void Movement::HandleMoveForwardBackwardSidewaysUpDown(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			// y is forward, x is sideways, z is updown
			TryAddInAnalogSpeed(g_vrInput.analogforward, data.y, "vr_move_analogforward_inverted");
			TryAddInAnalogSpeed(g_vrInput.analogsidemove, data.x, "vr_move_analogsideways_inverted");
			TryAddInAnalogSpeed(g_vrInput.analogupmove, data.z, "vr_move_analogupdown_inverted");
		}

		void Movement::HandleMoveForwardBackwardTurnUpDown(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			// y is forward, x is turn, z is updown
			TryAddInAnalogSpeed(g_vrInput.analogforward, data.y, "vr_move_analogforward_inverted");
			TryAddInAnalogSpeed(g_vrInput.analogturn, data.x, "vr_move_analogturn_inverted");
			TryAddInAnalogSpeed(g_vrInput.analogupmove, data.z, "vr_move_analogupdown_inverted");
		}

	}  // namespace Input
}  // namespace VR
