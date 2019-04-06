#pragma once

#include <unordered_map>
#include <string>

#include "openvr/openvr.h"

namespace VR
{
	namespace Input
	{
		class Movement
		{
		public:
			static void HandleMoveForward(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleMoveBackward(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleMoveLeft(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleMoveRight(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleMoveUp(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleMoveDown(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleTurnLeft(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleTurnRight(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleTurn90Left(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleTurn90Right(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleTurn180(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleJump(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleCrouch(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleLongJump(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleMoveForwardBackward(vr::InputAnalogActionData_t data, const std::string& action);
			static void HandleMoveSideways(vr::InputAnalogActionData_t data, const std::string& action);
			static void HandleMoveUpDown(vr::InputAnalogActionData_t data, const std::string& action);
			static void HandleTurn(vr::InputAnalogActionData_t data, const std::string& action);
			static void HandleMoveForwardBackwardSideways(vr::InputAnalogActionData_t data, const std::string& action);
			static void HandleMoveForwardBackwardTurn(vr::InputAnalogActionData_t data, const std::string& action);
			static void HandleMoveForwardBackwardSidewaysUpDown(vr::InputAnalogActionData_t data, const std::string& action);
			static void HandleMoveForwardBackwardTurnUpDown(vr::InputAnalogActionData_t data, const std::string& action);
		};

		class Weapons
		{
		public:
			static void HandleFire(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleAltFire(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleAnalogFire(vr::InputAnalogActionData_t data, const std::string& action);
			static void HandleReload(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleHolster(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleNext(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandlePrevious(vr::InputDigitalActionData_t data, const std::string& action);
		};

		class Other
		{
		public:
			static void HandleTeleport(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleFlashlight(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleGrab(vr::InputDigitalActionData_t data, const std::string& action);
			static void HandleLegacyUse(vr::InputDigitalActionData_t data, const std::string& action);
		};

		class Poses
		{
		public:
			static void HandleFlashlight(vr::InputPoseActionData_t data, const std::string& action);
			static void HandleMovement(vr::InputPoseActionData_t data, const std::string& action);
			static void HandleTeleporter(vr::InputPoseActionData_t data, const std::string& action);
			static void HandleWeapon(vr::InputPoseActionData_t data, const std::string& action);
		};
	}
}

