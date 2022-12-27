
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
#include "VRHelper.h"

namespace VR
{
	namespace Input
	{
		void Poses::HandleHandPoseLeft(const vr::InputPoseActionData_t& data, const std::string& action)
		{
			if (data.bActive)
			{
				gVRRenderer.GetHelper()->SetPose(VRPoseType::LEFT_HAND, data.pose, g_vrInput.GetRole(data.activeOrigin));
			}
			else
			{
				gVRRenderer.GetHelper()->ClearPose(VRPoseType::LEFT_HAND);
			}
		}

		void Poses::HandleHandPoseRight(const vr::InputPoseActionData_t& data, const std::string& action)
		{
			if (data.bActive)
			{
				gVRRenderer.GetHelper()->SetPose(VRPoseType::RIGHT_HAND, data.pose, g_vrInput.GetRole(data.activeOrigin));
			}
			else
			{
				gVRRenderer.GetHelper()->ClearPose(VRPoseType::RIGHT_HAND);
			}
		}

		void Poses::HandleHandPointerLeft(const vr::InputPoseActionData_t& data, const std::string& action)
		{
			if (data.bActive)
			{
				gVRRenderer.GetHelper()->SetPose(VRPoseType::LEFT_POINTER, data.pose, g_vrInput.GetRole(data.activeOrigin));
			}
			else
			{
				gVRRenderer.GetHelper()->ClearPose(VRPoseType::LEFT_POINTER);
			}
		}

		void Poses::HandleHandPointerRight(const vr::InputPoseActionData_t& data, const std::string& action)
		{
			if (data.bActive)
			{
				gVRRenderer.GetHelper()->SetPose(VRPoseType::RIGHT_POINTER, data.pose, g_vrInput.GetRole(data.activeOrigin));
			}
			else
			{
				gVRRenderer.GetHelper()->ClearPose(VRPoseType::RIGHT_POINTER);
			}
		}

		void Poses::HandleHandSkeletonLeft(const vr::VRSkeletalSummaryData_t& data, const vr::VRBoneTransform_t* bones, bool hasFingers, bool hasBones, const std::string& action)
		{
			g_vrInput.SetFingerSkeletalData(vr::TrackedControllerRole_LeftHand, data.flFingerCurl, data.flFingerSplay, bones, hasFingers, hasBones);
		}

		void Poses::HandleHandSkeletonRight(const vr::VRSkeletalSummaryData_t& data, const vr::VRBoneTransform_t* bones, bool hasFingers, bool hasBones, const std::string& action)
		{
			g_vrInput.SetFingerSkeletalData(vr::TrackedControllerRole_RightHand, data.flFingerCurl, data.flFingerSplay, bones, hasFingers, hasBones);
		}

		void Poses::HandleTriggerPull(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			auto role = g_vrInput.GetRole(data.activeOrigin);

			auto it = g_vrInput.m_fallbackFingerCurlData.find(role);
			if (it == g_vrInput.m_fallbackFingerCurlData.end())
			{
				g_vrInput.m_fallbackFingerCurlData[role] = std::array<float, vr::VRFinger_Count>();
			}

			float value = data.bActive ? data.x : 1.0f;

			g_vrInput.m_fallbackFingerCurlData[role][0] = std::clamp(value, 0.5f, 1.0f);
			g_vrInput.m_fallbackFingerCurlData[role][1] = std::clamp(value, 0.1f, 1.0f);
		}

		void Poses::HandleHandCurl(const vr::InputAnalogActionData_t& data, const std::string& action)
		{
			auto role = g_vrInput.GetRole(data.activeOrigin);

			auto it = g_vrInput.m_fallbackFingerCurlData.find(role);
			if (it == g_vrInput.m_fallbackFingerCurlData.end())
			{
				g_vrInput.m_fallbackFingerCurlData[role] = std::array<float, vr::VRFinger_Count>();
			}

			float value = data.bActive ? data.x : 1.f;

			g_vrInput.m_fallbackFingerCurlData[role][2] = std::clamp(value, 0.1f, 1.0f);
			g_vrInput.m_fallbackFingerCurlData[role][3] = std::clamp(value, 0.1f, 1.0f);
			g_vrInput.m_fallbackFingerCurlData[role][4] = std::clamp(value, 0.1f, 1.0f);
		}

	}  // namespace Input
}  // namespace VR
