
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

VRInputAction::VRInputAction() :
	m_type{ ActionType::INVALID }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, DigitalActionHandler handler, bool handleWhenNotInGame, bool handAgnostic) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::DIGITAL },
	m_digitalActionHandler{ handler },
	m_handleWhenNotInGame{ handleWhenNotInGame },
	m_handAgnostic{ handAgnostic }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, AnalogActionHandler handler, bool handleWhenNotInGame, bool handAgnostic) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::ANALOG },
	m_analogActionHandler{ handler },
	m_handleWhenNotInGame{ handleWhenNotInGame },
	m_handAgnostic{ handAgnostic }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, PoseActionHandler handler, bool handleWhenNotInGame, bool handAgnostic) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::POSE },
	m_poseActionHandler{ handler },
	m_handleWhenNotInGame{ handleWhenNotInGame },
	m_handAgnostic{ handAgnostic }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, SkeletalActionHandler handler, bool handleWhenNotInGame, bool handAgnostic) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::SKELETAL },
	m_skeletalActionHandler{ handler },
	m_handleWhenNotInGame{ handleWhenNotInGame },
	m_handAgnostic{ handAgnostic }
{
}

void VRInputAction::HandleDigitalInput(vr::VRInputValueHandle_t device)
{
	vr::InputDigitalActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetDigitalActionData(m_handle, &data, sizeof(vr::InputDigitalActionData_t), device);
	if (result == vr::VRInputError_None)
	{
		if (data.bActive && data.bChanged && data.bState)
		{
			g_vrInput.m_vrInputThisFrame = true;
		}
		(m_digitalActionHandler)(data, m_id);
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error while trying to get digital input action %s. (Error code: %i)\n", m_id.data(), result);
	}
}

void VRInputAction::HandleAnalogInput(vr::VRInputValueHandle_t device)
{
	vr::InputAnalogActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetAnalogActionData(m_handle, &data, sizeof(vr::InputAnalogActionData_t), device);
	if (result == vr::VRInputError_None)
	{
		if (data.bActive && (fabs(data.deltaX) > CVAR_GET_FLOAT("vr_move_analog_deadzone")
							|| fabs(data.deltaY) > CVAR_GET_FLOAT("vr_move_analog_deadzone")
							|| fabs(data.deltaZ) > CVAR_GET_FLOAT("vr_move_analog_deadzone") ))
		{
			g_vrInput.m_vrInputThisFrame = true;
		}
		(m_analogActionHandler)(data, m_id);
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error while trying to get analog input action %s. (Error code: %i)\n", m_id.data(), result);
	}
}

void VRInputAction::HandlePoseInput(vr::VRInputValueHandle_t device)
{
	vr::InputPoseActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetPoseActionDataRelativeToNow(m_handle, vr::TrackingUniverseStanding, 0.f, &data, sizeof(vr::InputPoseActionData_t), device);
	if (result == vr::VRInputError_None)
	{
		(m_poseActionHandler)(data, m_id);
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error while trying to get pose input action %s. (Error code: %i)\n", m_id.data(), result);
	}
}

void VRInputAction::HandleSkeletalInput()
{
	vr::InputSkeletalActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetSkeletalActionData(m_handle, &data, sizeof(vr::InputSkeletalActionData_t));
	if (result == vr::VRInputError_None)
	{
		if (data.bActive)
		{
			vr::VRSkeletalSummaryData_t summaryData{ 0 };
			result = vr::VRInput()->GetSkeletalSummaryData(
				m_handle,
				vr::EVRSummaryType::VRSummaryType_FromAnimation,
				&summaryData);

			vr::VRBoneTransform_t bones[31];
			vr::EVRInputError result2 = vr::VRInput()->GetSkeletalBoneData(
				m_handle,
				vr::EVRSkeletalTransformSpace::VRSkeletalTransformSpace_Parent,
				vr::EVRSkeletalMotionRange::VRSkeletalMotionRange_WithoutController,
				bones,
				31);

			if (result == vr::VRInputError_None || result2 == vr::VRInputError_None)
			{
				(m_skeletalActionHandler)(summaryData, bones, result == vr::VRInputError_None, result2 == vr::VRInputError_None, m_id);
			}
			else
			{
				gEngfuncs.Con_DPrintf("Error while trying to get skeletal data from input action %s. (Error codes: %i, %i)\n", m_id.data(), result, result2);
			}
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error while trying to get skeletal input action %s. (Error code: %i)\n", m_id.data(), result);
	}
}

void VRInputAction::HandleInput(bool isInGame)
{
	if (!isInGame && !m_handleWhenNotInGame)
		return;

	vr::VRInputValueHandle_t leftHandHandle = vr::k_ulInvalidInputValueHandle;
	vr::VRInputValueHandle_t rightHandHandle = vr::k_ulInvalidInputValueHandle;
	vr::VRInput()->GetInputSourceHandle("/user/hand/left", &leftHandHandle);
	vr::VRInput()->GetInputSourceHandle("/user/hand/right", &rightHandHandle);

	switch (m_type)
	{
	case ActionType::DIGITAL:
		if (m_handAgnostic)
		{
			HandleDigitalInput(leftHandHandle);
			HandleDigitalInput(rightHandHandle);
		}
		else
		{
			HandleDigitalInput(vr::k_ulInvalidInputValueHandle);
		}
		break;
	case ActionType::ANALOG:
		if (m_handAgnostic)
		{
			HandleAnalogInput(leftHandHandle);
			HandleAnalogInput(rightHandHandle);
		}
		else
		{
			HandleAnalogInput(vr::k_ulInvalidInputValueHandle);
		}
		break;
	case ActionType::POSE:
		HandlePoseInput(vr::k_ulInvalidInputValueHandle);
		break;
	case ActionType::SKELETAL:
		HandleSkeletalInput();
		break;
	default:
		gEngfuncs.Con_DPrintf("VRInputAction::HandleInput: Invalid action type: %i\n", static_cast<int>(m_type));
		break;
	}
}
