
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

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, DigitalActionHandler handler, bool handleWhenNotInGame) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::DIGITAL },
	m_digitalActionHandler{ handler },
	m_handleWhenNotInGame{ handleWhenNotInGame }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, AnalogActionHandler handler, bool handleWhenNotInGame) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::ANALOG },
	m_analogActionHandler{ handler },
	m_handleWhenNotInGame{ handleWhenNotInGame }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, PoseActionHandler handler, bool handleWhenNotInGame) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::POSE },
	m_poseActionHandler{ handler },
	m_handleWhenNotInGame{ handleWhenNotInGame }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, SkeletalActionHandler handler, bool handleWhenNotInGame) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::SKELETAL },
	m_skeletalActionHandler{ handler },
	m_handleWhenNotInGame{ handleWhenNotInGame }
{
}

void VRInputAction::HandleDigitalInput()
{
	vr::InputDigitalActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetDigitalActionData(m_handle, &data, sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidInputValueHandle);
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

void VRInputAction::HandleAnalogInput()
{
	vr::InputAnalogActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetAnalogActionData(m_handle, &data, sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidInputValueHandle);
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

void VRInputAction::HandlePoseInput()
{
	vr::InputPoseActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetPoseActionDataRelativeToNow(m_handle, vr::TrackingUniverseStanding, 0.f, &data, sizeof(vr::InputPoseActionData_t), vr::k_ulInvalidInputValueHandle);
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
			result = vr::VRInput()->GetSkeletalSummaryData(m_handle, vr::EVRSummaryType::VRSummaryType_FromAnimation, &summaryData);
			if (result == vr::VRInputError_None)
			{
				(m_skeletalActionHandler)(summaryData, m_id);
			}
			else
			{
				gEngfuncs.Con_DPrintf("Error while trying to get skeletal summary data from input action %s. (Error code: %i)\n", m_id.data(), result);
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

	switch (m_type)
	{
	case ActionType::DIGITAL:
		HandleDigitalInput();
		break;
	case ActionType::ANALOG:
		HandleAnalogInput();
		break;
	case ActionType::POSE:
		HandlePoseInput();
		break;
	case ActionType::SKELETAL:
		HandleSkeletalInput();
		break;
	default:
		gEngfuncs.Con_DPrintf("VRInputAction::HandleInput: Invalid action type: %i\n", static_cast<int>(m_type));
		break;
	}
}
