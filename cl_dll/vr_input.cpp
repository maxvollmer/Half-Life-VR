
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>

#include "hud.h"
#include "cl_util.h"
#include "vr_input.h"
#include "eiface.h"

VRInput g_vrInput;

void VRInput::Init()
{
	const std::string relativeGameDir{ gEngfuncs.pfnGetGameDirectory() };
	const std::string relativeManifestDir = relativeGameDir + "/actions/actions.manifest";
	std::filesystem::path relativeManifestPath = relativeManifestDir;
	std::filesystem::path absoluteManifestPath = std::filesystem::absolute(relativeManifestPath);

	if (std::filesystem::exists(absoluteManifestPath))
	{
		vr::EVRInputError result = vr::VRInput()->SetActionManifestPath(absoluteManifestPath.string().data());
		if (result == vr::VRInputError_None)
		{
			RegisterActionSets();
			m_isLegacyInput = false;
		}
		else
		{
			gEngfuncs.Con_DPrintf("Error: Couldn't load actions.manifest, falling back to legacy input. (Error code: %i)\n", result);
			m_isLegacyInput = true;
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error: Couldn't load actions.manifest, because it doesn't exist. Falling back to legacy input.\n");
		m_isLegacyInput = true;
	}
}

void VRInput::RegisterActionSets()
{
	// TODO: Implement all actions
	if (RegisterActionSet("movement"))
	{
		RegisterAction("movement", "Forward", &VR::Input::Movement::HandleMoveForward);
		RegisterAction("movement", "Backward", &VR::Input::Movement::HandleMoveBackward);
		RegisterAction("movement", "Left", &VR::Input::Movement::HandleMoveLeft);
		RegisterAction("movement", "Right", &VR::Input::Movement::HandleMoveRight);
		RegisterAction("movement", "Up", &VR::Input::Movement::HandleMoveUp);
		RegisterAction("movement", "Down", &VR::Input::Movement::HandleMoveDown);
		RegisterAction("movement", "TurnLeft", &VR::Input::Movement::HandleTurnLeft);
		RegisterAction("movement", "TurnRight", &VR::Input::Movement::HandleTurnRight);
		RegisterAction("movement", "Turn90Left", &VR::Input::Movement::HandleTurn90Left);
		RegisterAction("movement", "Turn90Right", &VR::Input::Movement::HandleTurn90Right);
		RegisterAction("movement", "Turn180", &VR::Input::Movement::HandleTurn180);
		RegisterAction("movement", "Jump", &VR::Input::Movement::HandleJump);
		RegisterAction("movement", "Crouch", &VR::Input::Movement::HandleCrouch);
		RegisterAction("movement", "LongJump", &VR::Input::Movement::HandleLongJump);
	}
	if (RegisterActionSet("analogmovement"))
	{
		RegisterAction("analogmovement", "ForwardBackward", &VR::Input::Movement::HandleMoveForwardBackward);
		RegisterAction("analogmovement", "Sideways", &VR::Input::Movement::HandleMoveSideways);
		RegisterAction("analogmovement", "UpDown", &VR::Input::Movement::HandleMoveUpDown);
		RegisterAction("analogmovement", "Turn", &VR::Input::Movement::HandleMoveTurn);
		RegisterAction("analogmovement", "ForwardBackwardSideways", &VR::Input::Movement::HandleMoveForwardBackwardSideways);
		RegisterAction("analogmovement", "ForwardBackwardTurn", &VR::Input::Movement::HandleMoveForwardBackwardTurn);
		RegisterAction("analogmovement", "ForwardBackwardSidewaysUpDown", &VR::Input::Movement::HandleMoveForwardBackwardSidewaysUpDown);
		RegisterAction("analogmovement", "ForwardBackwardTurnUpDown", &VR::Input::Movement::HandleMoveForwardBackwardTurnUpDown);
	}
	if (RegisterActionSet("weapons"))
	{
		RegisterAction("weapons", "Fire", &VR::Input::Weapons::HandleFire);
		RegisterAction("weapons", "FireAlt", &VR::Input::Weapons::HandleFireAlt);
		RegisterAction("weapons", "Reload", &VR::Input::Weapons::HandleReload);
		RegisterAction("weapons", "Holster", &VR::Input::Weapons::HandleHolster);
		RegisterAction("weapons", "Next", &VR::Input::Weapons::HandleNext);
		RegisterAction("weapons", "Previous", &VR::Input::Weapons::HandlePrevious);
		RegisterFeedback("weapons", "Recoil");
	}
	if (RegisterActionSet("damagefeedback"))
	{
		RegisterFeedback("damagefeedback", "All");
		RegisterFeedback("damagefeedback", "Bullet");
		RegisterFeedback("damagefeedback", "Fall");
		RegisterFeedback("damagefeedback", "Blast");
		RegisterFeedback("damagefeedback", "Generic");
		RegisterFeedback("damagefeedback", "Crush");
		RegisterFeedback("damagefeedback", "Slash");
		RegisterFeedback("damagefeedback", "Burn");
		RegisterFeedback("damagefeedback", "Freeze");
		RegisterFeedback("damagefeedback", "Drown");
		RegisterFeedback("damagefeedback", "Club");
		RegisterFeedback("damagefeedback", "Shock");
		RegisterFeedback("damagefeedback", "Sonic");
		RegisterFeedback("damagefeedback", "EnergyBeam");
		RegisterFeedback("damagefeedback", "Nervegas");
		RegisterFeedback("damagefeedback", "Poison");
		RegisterFeedback("damagefeedback", "Radiation");
		RegisterFeedback("damagefeedback", "Acid");
		RegisterFeedback("damagefeedback", "SlowBurn");
		RegisterFeedback("damagefeedback", "SlowFreeze");
		RegisterFeedback("damagefeedback", "Mortar");
	}
	if (RegisterActionSet("other"))
	{
		RegisterAction("other", "Teleport", &VR::Input::Other::HandleTeleport);
		RegisterAction("other", "FlashLight", &VR::Input::Other::HandleFlashLight);
		RegisterAction("other", "Grab", &VR::Input::Other::HandleGrab);
		RegisterAction("other", "LegacyUse", &VR::Input::Other::HandleLegacyUse);
		RegisterFeedback("other", "Earthquake");
		RegisterFeedback("other", "TrainShake");
		RegisterFeedback("other", "WaterSplash");
	}
	if (RegisterActionSet("poses"))
	{
		RegisterAction("poses", "Flashlight", &VR::Input::Poses::HandleFlashlight);
		RegisterAction("poses", "Movement", &VR::Input::Poses::HandleMovement);
		RegisterAction("poses", "Teleporter", &VR::Input::Poses::HandleTeleporter);
		RegisterAction("poses", "Weapon", &VR::Input::Poses::HandleWeapon);
	}
}

bool VRInput::RegisterActionSet(const std::string& actionSet)
{
	vr::EVRInputError result = vr::VRInput()->GetActionSetHandle("/actions/movement", &m_actionSets[actionSet].handle);
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. (Error code: %i)\n", actionSet, result);
		return false;
	}
	return true;
}

bool VRInput::RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::DigitalActionHandler handler)
{
	std::string actionName = "/actions/" + actionSet + "/in/" + action;
	vr::VRActionHandle_t handle{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetActionHandle(actionName.data(), &handle);
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. (Error code: %i)\n", actionSet, result);
		return false;
	}
	else
	{
		m_actionSets[actionSet].actions[action] = VRInputAction{ action, handle, handler };
		return true;
	}
}

bool VRInput::RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::AnalogActionHandler handler)
{
	std::string actionName = "/actions/" + actionSet + "/in/" + action;
	vr::VRActionHandle_t handle{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetActionHandle(actionName.data(), &handle);
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. (Error code: %i)\n", actionSet, result);
		return false;
	}
	else
	{
		m_actionSets[actionSet].actions[action] = VRInputAction{ action, handle, handler };
		return true;
	}
}

bool VRInput::RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::PoseActionHandler handler)
{
	std::string actionName = "/actions/" + actionSet + "/in/" + action;
	vr::VRActionHandle_t handle{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetActionHandle(actionName.data(), &handle);
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. (Error code: %i)\n", actionSet, result);
		return false;
	}
	else
	{
		m_actionSets[actionSet].actions[action] = VRInputAction{ action, handle, handler };
		return true;
	}
}

bool VRInput::RegisterFeedback(const std::string& actionSet, const std::string& action)
{
	std::string actionName = "/actions/" + actionSet + "/out/" + action;
	vr::VRActionHandle_t handle{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetActionHandle(actionName.data(), &handle);
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. (Error code: %i)\n", actionSet, result);
		return false;
	}
	else
	{
		m_actionSets[actionSet].feedbackActions[action] = handle;
		return true;
	}
}

void VRInput::FireFeedback(FeedbackType feedback, int damageType, float durationInSeconds, float frequency, float amplitude)
{
	vr::VRActionHandle_t handle{ 0 };

	switch (feedback)
	{
	case FeedbackType::RECOIL:
		handle = m_actionSets["weapons"].feedbackActions["Recoil"];
		break;
	case FeedbackType::EARTHQUAKE:
		handle = m_actionSets["other"].feedbackActions["Earthquake"];
		break;
	case FeedbackType::ONTRAIN:
		handle = m_actionSets["other"].feedbackActions["TrainShake"];
		break;
	case FeedbackType::WATERSPLASH:
		handle = m_actionSets["other"].feedbackActions["WaterSplash"];
		break;
	case FeedbackType::DAMAGE:
		handle = m_actionSets["damagefeedback"].feedbackActions["All"];
		if (damageType == DMG_GENERIC)
		{
			FireDamageFeedback("Generic", durationInSeconds, frequency, amplitude);
		}
		else
		{
			if (damageType & DMG_CRUSH)
			{
				FireDamageFeedback("Crush", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_BULLET)
			{
				FireDamageFeedback("Bullet", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_SLASH)
			{
				FireDamageFeedback("Slash", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_BURN)
			{
				FireDamageFeedback("Burn", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_FREEZE)
			{
				FireDamageFeedback("Freeze", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_FALL)
			{
				FireDamageFeedback("Fall", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_BLAST)
			{
				FireDamageFeedback("Blast", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_CLUB)
			{
				FireDamageFeedback("Club", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_SHOCK)
			{
				FireDamageFeedback("Shock", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_SONIC)
			{
				FireDamageFeedback("Sonic", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_ENERGYBEAM)
			{
				FireDamageFeedback("EnergyBeam", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_DROWN)
			{
				FireDamageFeedback("Drown", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_NERVEGAS)
			{
				FireDamageFeedback("Nervegas", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_POISON)
			{
				FireDamageFeedback("Poison", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_RADIATION)
			{
				FireDamageFeedback("Radiation", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_ACID)
			{
				FireDamageFeedback("Acid", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_SLOWBURN)
			{
				FireDamageFeedback("SlowBurn", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_SLOWFREEZE)
			{
				FireDamageFeedback("SlowFreeze", durationInSeconds, frequency, amplitude);
			}
			if (damageType & DMG_MORTAR)
			{
				FireDamageFeedback("Mortar", durationInSeconds, frequency, amplitude);
			}
		}
		break;
	}

	vr::VRInput()->TriggerHapticVibrationAction(handle, 0.f, durationInSeconds, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
}

void VRInput::FireDamageFeedback(const std::string& action, float durationInSeconds, float frequency, float amplitude)
{
	vr::VRInput()->TriggerHapticVibrationAction(m_actionSets["damagefeedback"].feedbackActions[action], 0.f, durationInSeconds, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
}

void VRInput::HandleInput()
{
	if (IsLegacyInput())
		return;

	UpdateActionStates();
	for (auto &[actionSetName, actionSet] : m_actionSets)
	{
		for (auto &[actionName, action] : actionSet.actions)
		{
			action.HandleInput();
		}
	}
}

void VRInput::UpdateActionStates()
{
	for (auto& actionSet : m_actionSets)
	{
		vr::VRActiveActionSet_t activeActionSet{ 0 };
		activeActionSet.ulActionSet = actionSet.second.handle;
		vr::EVRInputError result = vr::VRInput()->UpdateActionState(&activeActionSet, sizeof(vr::VRActiveActionSet_t), 1);
		if (result != vr::VRInputError_None)
		{
			gEngfuncs.Con_DPrintf("Error while trying to get active state for input action set /actions/%s. (Error code: %i)\n", actionSet.first, result);
		}
	}
}
