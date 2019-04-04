
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>

#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "vr_input.h"
#include "eiface.h"

VRInputAction::VRInputAction() :
	m_type{ ActionType::INVALID }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, DigitalActionHandler handler) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::DIGITAL },
	m_digitalActionHandler{ handler }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, AnalogActionHandler handler) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::ANALOG },
	m_analogActionHandler{ handler }
{
}

VRInputAction::VRInputAction(const std::string& id, vr::VRActionHandle_t handle, PoseActionHandler handler) :
	m_id{ id },
	m_handle{ handle },
	m_type{ ActionType::POSE },
	m_poseActionHandler{ handler }
{
}

void VRInputAction::HandleDigitalInput()
{
	vr::InputDigitalActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetDigitalActionData(m_handle, &data, sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidInputValueHandle);
	if (result == vr::VRInputError_None)
	{
		(m_digitalActionHandler)(data, m_id);
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error while trying to get input action %s. (Error code: %i)\n", m_id, result);
	}
}

void VRInputAction::HandleAnalogInput()
{
	vr::InputAnalogActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetAnalogActionData(m_handle, &data, sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidInputValueHandle);
	if (result == vr::VRInputError_None)
	{
		(m_analogActionHandler)(data, m_id);
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error while trying to get input action %s. (Error code: %i)\n", m_id, result);
	}
}

void VRInputAction::HandlePoseInput()
{
	vr::InputPoseActionData_t data{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetPoseActionData(m_handle, vr::TrackingUniverseStanding, 0.f, &data, sizeof(vr::InputPoseActionData_t), vr::k_ulInvalidInputValueHandle);
	if (result == vr::VRInputError_None)
	{
		(m_poseActionHandler)(data, m_id);
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error while trying to get input action %s. (Error code: %i)\n", m_id, result);
	}
}

void VRInputAction::HandleInput()
{
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
	default:
		gEngfuncs.Con_DPrintf("VRInputAction::HandleInput: Invalid action type: %i\n", static_cast<int>(m_type));
		break;
	}
}
