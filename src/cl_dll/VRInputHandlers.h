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
			static void HandleMove(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleContinuousTurn(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleTurnLeft(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTurnRight(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleJump(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleCrouch(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleWalk(const vr::InputDigitalActionData_t& data, const std::string& action);
		};

		class Weapons
		{
		public:
			static void HandleFire(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleAltFire(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleReload(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleNext(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandlePrevious(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleSelect(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleHolster(const vr::InputDigitalActionData_t& data, const std::string& action);
		};

		class Interact
		{
		public:
			static void HandleGrab(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleLegacyUse(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleTeleport(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleFlashlight(const vr::InputDigitalActionData_t& data, const std::string& action);
		};

		class Other
		{
		public:
			static void HandleToggleVRMenu(const vr::InputDigitalActionData_t& data, const std::string& action);
			static void HandleClickVRMenu(const vr::InputDigitalActionData_t& data, const std::string& action);

			static void HandleCustomAction(const vr::InputDigitalActionData_t& data, const std::string& action);
		};

		class Poses
		{
		public:
			static void HandleHandPoseLeft(const vr::InputPoseActionData_t& data, const std::string& action);
			static void HandleHandPoseRight(const vr::InputPoseActionData_t& data, const std::string& action);
			static void HandleHandPointerLeft(const vr::InputPoseActionData_t& data, const std::string& action);
			static void HandleHandPointerRight(const vr::InputPoseActionData_t& data, const std::string& action);

			static void HandleHandSkeletonLeft(const vr::VRSkeletalSummaryData_t& data, const vr::VRBoneTransform_t* bones, bool hasFingers, bool hasBones, const std::string& action);
			static void HandleHandSkeletonRight(const vr::VRSkeletalSummaryData_t& data, const vr::VRBoneTransform_t* bones, bool hasFingers, bool hasBones, const std::string& action);

			static void HandleHandCurl(const vr::InputAnalogActionData_t& data, const std::string& action);
			static void HandleTriggerPull(const vr::InputAnalogActionData_t& data, const std::string& action);
		};
	}  // namespace Input
}  // namespace VR
