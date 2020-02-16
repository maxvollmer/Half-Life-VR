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
			static void HandleMoveForward(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleMoveBackward(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleMoveLeft(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleMoveRight(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleMoveUp(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleMoveDown(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTurnLeft(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTurnRight(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTurn45Left(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTurn45Right(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTurn90Left(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTurn90Right(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTurn180(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleJump(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleCrouch(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleLongJump(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleWalk(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleAnalogJump(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleAnalogCrouch(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleAnalogLongJump(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleMoveForwardBackward(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleMoveSideways(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleMoveUpDown(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleTurn(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleMoveForwardBackwardSideways(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleMoveForwardBackwardTurn(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleMoveForwardBackwardSidewaysUpDown(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleMoveForwardBackwardTurnUpDown(const vr::InputAnalogActionData_t& data, const std::string& action);
		};

		class Weapons
		{
		public:
			static void HandleFire(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleAltFire(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleAnalogFire(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleReload(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleHolster(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleNext(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandlePrevious(const vr::InputDigitalActionData_t& data, const std::string& action);
		};

		class Other
		{
		public:
			static void HandleTeleport(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleFlashlight(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleLeftGrab(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleRightGrab(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleLegacyUse(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleLetGoOffLadder(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleQuickSave(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleQuickLoad(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleRestartCurrentMap(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandlePauseGame(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleExitGame(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleCustomAction(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleLeftHandSkeleton(const vr::VRSkeletalSummaryData_t& data, const std::string& action);
			static void HandleRightHandSkeleton(const vr::VRSkeletalSummaryData_t& data, const std::string& action);
		};

		class Poses
		{
		public:
			static void HandleFlashlight(const vr::InputPoseActionData_t& data, const std::string& action);
			static void HandleMovement(const vr::InputPoseActionData_t& data, const std::string& action);
			static void HandleTeleporter(const vr::InputPoseActionData_t& data, const std::string& action);
		};
	}  // namespace Input
}  // namespace VR
