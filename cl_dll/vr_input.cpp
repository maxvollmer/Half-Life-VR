
#include <iostream>
#include <filesystem>

#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "vr_input.h"
#include "eiface.h"

VRInput g_vrInput;

void VRInput::Init()
{
	const std::string relativeGameDir{ gEngfuncs.pfnGetGameDirectory() };
	const std::string relativeManifestDir = relativeGameDir + "/actions/actions.manifest";
	std::filesystem::path relativeManifestPath = relativeManifestDir;
	std::filesystem::path absoluteManifestPath = std::filesystem::absolute(relativeManifestPath);

	vr::EVRInputError result = vr::VRInput()->SetActionManifestPath(absoluteManifestPath.string().data());
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Couldn't load actions.manifest, falling back to legay input. (Error code: %i)\n", result);
		m_legacyInput = true;
	}
	else
	{
		gEngfuncs.Con_DPrintf("Successfully loaded actions.manifest!\n");
		vr::EVRInputError result1 = vr::VRInput()->GetActionSetHandle("/actions/movement", &m_actionSets["movement"].handle);
		vr::EVRInputError result2 = vr::VRInput()->GetActionHandle("/actions/movement/in/Forward", &m_actionSets["movement"].actions["Forward"]);

		gEngfuncs.Con_DPrintf("result1: %i, result2: %i\n", result1, result2);
	}
}

void VRInput::HandleInput()
{
	// TODO: Awesomize this
	vr::VRActiveActionSet_t dings{ 0 };
	dings.ulActionSet = m_actionSets["movement"].handle;
	vr::EVRInputError result1 = vr::VRInput()->UpdateActionState(&dings, sizeof(vr::VRActiveActionSet_t), 1);

	vr::InputDigitalActionData_t data{ 0 };
	vr::EVRInputError result2 = vr::VRInput()->GetDigitalActionData(m_actionSets["movement"].actions["Forward"], &data, sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidInputValueHandle);
}
