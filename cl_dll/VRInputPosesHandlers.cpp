
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
		vr::ETrackedControllerRole GetRole(vr::VRInputValueHandle_t origin)
		{
			vr::InputOriginInfo_t originInfo;
			vr::ETrackedControllerRole role{ vr::TrackedControllerRole_Invalid };
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(origin, &originInfo, sizeof(vr::InputOriginInfo_t)))
			{
				role = gVRRenderer.GetVRSystem()->GetControllerRoleForTrackedDeviceIndex(originInfo.trackedDeviceIndex);
			}
			return role;
		}

		void Poses::HandleFlashlight(const vr::InputPoseActionData_t& data, const std::string& action)
		{
			if (data.bActive)
			{
				gVRRenderer.GetHelper()->SetPose(VRPoseType::FLASHLIGHT, data.pose, GetRole(data.activeOrigin));
			}
			else
			{
				gVRRenderer.GetHelper()->ClearPose(VRPoseType::FLASHLIGHT);
			}
		}

		void Poses::HandleMovement(const vr::InputPoseActionData_t& data, const std::string& action)
		{
			if (data.bActive)
			{
				gVRRenderer.GetHelper()->SetPose(VRPoseType::MOVEMENT, data.pose, GetRole(data.activeOrigin));
			}
			else
			{
				gVRRenderer.GetHelper()->ClearPose(VRPoseType::MOVEMENT);
			}
		}

		void Poses::HandleTeleporter(const vr::InputPoseActionData_t& data, const std::string& action)
		{
			if (data.bActive)
			{
				gVRRenderer.GetHelper()->SetPose(VRPoseType::TELEPORTER, data.pose, GetRole(data.activeOrigin));
			}
			else
			{
				gVRRenderer.GetHelper()->ClearPose(VRPoseType::TELEPORTER);
			}
		}
	}  // namespace Input
}  // namespace VR
