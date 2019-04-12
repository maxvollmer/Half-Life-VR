
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>

#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "vr_input.h"
#include "vr_renderer.h"
#include "vr_helper.h"
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

		void Movement::HandleTurn90Left(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				gVRRenderer.GetHelper()->InstantRotateYaw(-90.f);
			}
		}

		void Movement::HandleTurn90Right(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				gVRRenderer.GetHelper()->InstantRotateYaw(90.f);
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

		void Movement::HandleMoveForwardBackward(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			g_vrInput.analogforward = (data.bActive) ? data.x : 0.f;
		}

		void Movement::HandleMoveSideways(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			g_vrInput.analogsidemove = (data.bActive) ? data.x : 0.f;
		}

		void Movement::HandleMoveUpDown(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			g_vrInput.analogupmove = (data.bActive) ? data.x : 0.f;
		}

		void Movement::HandleTurn(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			g_vrInput.analogturn = (data.bActive) ? data.x : 0.f;
		}

		void Movement::HandleMoveForwardBackwardSideways(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			g_vrInput.analogforward = (data.bActive) ? data.x : 0.f;
			g_vrInput.analogsidemove = (data.bActive) ? data.y : 0.f;
		}

		void Movement::HandleMoveForwardBackwardTurn(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			g_vrInput.analogforward = (data.bActive) ? data.x : 0.f;
			g_vrInput.analogturn = (data.bActive) ? data.y : 0.f;
		}

		void Movement::HandleMoveForwardBackwardSidewaysUpDown(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			g_vrInput.analogforward = (data.bActive) ? data.x : 0.f;
			g_vrInput.analogsidemove = (data.bActive) ? data.y : 0.f;
			g_vrInput.analogupmove = (data.bActive) ? data.z : 0.f;
		}

		void Movement::HandleMoveForwardBackwardTurnUpDown(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			g_vrInput.analogforward = (data.bActive) ? data.x : 0.f;
			g_vrInput.analogturn = (data.bActive) ? data.y : 0.f;
			g_vrInput.analogupmove = (data.bActive) ? data.z : 0.f;
		}

	}
}