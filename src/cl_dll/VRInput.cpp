
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>
#include <fstream>
#include <cctype>
#include <regex>
#include <algorithm>

namespace
{
	// For some reason the compiler finds a wrong isspace when we use std::isspace directly as predicate in std::find,
	// so we wrap it here
	// in addition, std::isspace asserts in debug if a character is out of range, so we catch this here
	bool IsSpace(const int c)
	{
		try
		{
			if (c >= -1 && c <= 255)
			{
				return std::isspace(c);
			}
		}
		catch (...) {}
		return false;
	}

	static inline void TrimString(std::string& s)
	{
		s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), IsSpace));
		s.erase(std::find_if_not(s.rbegin(), s.rend(), IsSpace).base(), s.end());
	}
}

#include "hud.h"
#include "cl_util.h"
#include "VRInput.h"
#include "eiface.h"
#include "pm_defs.h"

#include "VRCommon.h"
#include "VRRenderer.h"
#include "VRHelper.h"

VRInput g_vrInput;

// used by client weapon prediction code
void VRRegisterRecoil(float intensity)
{
	g_vrInput.recoilintensity = intensity;
}

void VRInput::Init()
{
	std::filesystem::path absoluteManifestPath = GetPathFor("/actions/actions.manifest");

	if (std::filesystem::exists(absoluteManifestPath))
	{
		vr::EVRInputError result = vr::VRInput()->SetActionManifestPath(absoluteManifestPath.string().data());
		if (result == vr::VRInputError_None)
		{
			LoadCustomActions();
			RegisterActionSets();
		}
		else
		{
			std::wstring path(absoluteManifestPath.c_str());
			std::string spath(path.begin(), path.end());
			gEngfuncs.Con_DPrintf("Error: Couldn't load %s. (Error code: %i)\n", path.c_str(), result);
		}
	}
	else
	{
		std::wstring path(absoluteManifestPath.c_str());
		std::string spath(path.begin(), path.end());
		gEngfuncs.Con_DPrintf("Error: %s doesn't exist.\n", spath.c_str());
	}
}

void VRInput::LoadCustomActions()
{
	std::filesystem::path absoluteCustomActionsPath = GetPathFor("/actions/customactions.cfg");
	if (std::filesystem::exists(absoluteCustomActionsPath))
	{
		std::ifstream infile(absoluteCustomActionsPath);
		int lineNum = 0;
		std::string line;
		while (std::getline(infile, line))
		{
			lineNum++;
			TrimString(line);

			// Skip empty lines
			if (line.empty())
				continue;

			// Skip comments
			if (line[0] == '#')
				continue;

			auto it = std::find_if(line.begin(), line.end(), IsSpace);
			try
			{
				if (it != line.end())
				{
					std::string customActionName = line.substr(0, std::distance(line.begin(), it));
					TrimString(customActionName);

					// action names must be whitespace only
					if (!std::regex_match(customActionName, std::regex("^[A-Za-z]+$")))
						throw 0;

					std::string customActionCommand = line.substr(std::distance(line.begin(), it), std::string::npos);
					TrimString(customActionCommand);

					// command can't be empty
					if (customActionCommand.empty())
						throw 0;

					m_customActions[customActionName].name = customActionName;
					m_customActions[customActionName].commands.push_back(customActionCommand);

					gEngfuncs.Con_DPrintf("Successfully loaded custom action %s: %s\n", customActionName.data(), customActionCommand.data());
				}
				else
				{
					throw 0;
				}
			}
			catch (...)
			{
				gEngfuncs.Con_DPrintf("Invalid custom command in /actions/customactions.cfg (line %i): %s\n", lineNum, line.data());
			}
		}
	}
}

void VRInput::RegisterActionSets()
{
	if (RegisterActionSet("haptic", false))
	{
		RegisterFeedback("haptic", "HapticVibration");
	}

	if (RegisterActionSet("poses", true))
	{
		RegisterAction("poses", "HandPoseLeft", &VR::Input::Poses::HandleHandPoseLeft, true);
		RegisterAction("poses", "HandPoseRight", &VR::Input::Poses::HandleHandPoseRight, true);
		RegisterAction("poses", "HandPointerLeft", &VR::Input::Poses::HandleHandPointerLeft, true);
		RegisterAction("poses", "HandPointerRight", &VR::Input::Poses::HandleHandPointerRight, true);
		RegisterAction("poses", "HandSkeletonLeft", &VR::Input::Poses::HandleHandSkeletonLeft, true);
		RegisterAction("poses", "HandSkeletonRight", &VR::Input::Poses::HandleHandSkeletonRight, true);
		RegisterAction("poses", "HandCurl", &VR::Input::Poses::HandleHandCurl, true);
		RegisterAction("poses", "TriggerPull", &VR::Input::Poses::HandleTriggerPull, true);
	}

	if (RegisterActionSet("move", false))
	{
		RegisterAction("move", "Move", &VR::Input::Movement::HandleMove);
		RegisterAction("move", "ContinuousTurn", &VR::Input::Movement::HandleContinuousTurn);
		RegisterAction("move", "TurnLeft", &VR::Input::Movement::HandleTurnLeft);
		RegisterAction("move", "TurnRight", &VR::Input::Movement::HandleTurnRight);
		RegisterAction("move", "Walk", &VR::Input::Movement::HandleWalk);
		RegisterAction("move", "Jump", &VR::Input::Movement::HandleJump);
		RegisterAction("move", "Crouch", &VR::Input::Movement::HandleCrouch);
	}

	if (RegisterActionSet("interact", false))
	{
		RegisterAction("interact", "Grab", &VR::Input::Interact::HandleGrab);
		RegisterAction("interact", "LegacyUse", &VR::Input::Interact::HandleLegacyUse);

		// TODO: MISSING IN VIVE BINDINGS!!!
		RegisterAction("interact", "Teleport", &VR::Input::Interact::HandleTeleport);

		// TODO: MISSING IN ALL BINDINGS!!!!
		RegisterAction("interact", "ToggleFlashlight", &VR::Input::Interact::HandleFlashlight);
	}

	if (RegisterActionSet("weapon", false))
	{
		RegisterAction("weapon", "Attack", &VR::Input::Weapons::HandleFire);
		RegisterAction("weapon", "Attack2", &VR::Input::Weapons::HandleAltFire);
		RegisterAction("weapon", "Next", &VR::Input::Weapons::HandleNext);
		RegisterAction("weapon", "Previous", &VR::Input::Weapons::HandlePrevious);
		RegisterAction("weapon", "Select", &VR::Input::Weapons::HandleSelect);
		RegisterAction("weapon", "Reload", &VR::Input::Weapons::HandleReload);

		RegisterAction("weapon", "Holster", &VR::Input::Weapons::HandleHolster);
	}

	if (RegisterActionSet("menu", true))
	{
		RegisterAction("menu", "ToggleVRMenu", &VR::Input::Other::HandleToggleVRMenu, true);
		RegisterAction("menu", "ClickVRMenu", &VR::Input::Other::HandleClickVRMenu, true);
	}

	if (RegisterActionSet("custom", true))
	{
		for (const auto& customAction : m_customActions)
		{
			RegisterAction("custom", customAction.first, &VR::Input::Other::HandleCustomAction, true);
		}
	}
}

bool VRInput::RegisterActionSet(const std::string& actionSet, bool handleWhenNotInGame)
{
	std::string actionSetName = "/actions/" + actionSet;
	vr::VRActionSetHandle_t handle{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetActionSetHandle(actionSetName.data(), &handle);
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. (Error code: %i)\n", actionSet, result);
		return false;
	}
	else
	{
		m_actionSets[actionSet].handle = handle;
		m_actionSets[actionSet].handleWhenNotInGame = handleWhenNotInGame;
		return true;
	}
}

bool VRInput::RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::DigitalActionHandler handler, bool handleWhenNotInGame)
{
	std::string actionName = "/actions/" + actionSet + "/in/" + action;
	vr::VRActionHandle_t handle{ 0 };
	vr::EVRInputError result = vr::VRInput()->GetActionHandle(actionName.data(), &handle);
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. (Error code: %i)\n", actionSet, result);
		return false;
	}
	else if (handle == vr::k_ulInvalidActionHandle)
	{
		if (actionSet == "custom")
		{
			gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. The action doesn't exist! (Did you define a custom action in customactions.cfg, but forgot to add it to actions.manifest?)\n", actionSet, result);
		}
		else
		{
			gEngfuncs.Con_DPrintf("Error while trying to register input action set /actions/%s. The action doesn't exist!\n", actionSet, result);
		}
		return false;
	}
	else
	{
		m_actionSets[actionSet].actions[action] = VRInputAction{ action, handle, handler, handleWhenNotInGame };
		return true;
	}
}

bool VRInput::RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::AnalogActionHandler handler, bool handleWhenNotInGame)
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
		m_actionSets[actionSet].actions[action] = VRInputAction{ action, handle, handler, handleWhenNotInGame };
		return true;
	}
}

bool VRInput::RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::PoseActionHandler handler, bool handleWhenNotInGame)
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
		m_actionSets[actionSet].actions[action] = VRInputAction{ action, handle, handler, handleWhenNotInGame };
		return true;
	}
}

bool VRInput::RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::SkeletalActionHandler handler, bool handleWhenNotInGame)
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
		m_actionSets[actionSet].actions[action] = VRInputAction{ action, handle, handler, handleWhenNotInGame };
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

void VRInput::FireFeedbacks()
{
	extern playermove_t* pmove;

	// LEFTTOUCH
	if (gHUD.m_vrLeftHandTouchVibrateIntensity > 0.f)
	{
		FireFeedback(FeedbackType::LEFTTOUCH, 0, 0.1f, 1.f * (std::max)(1.f, gHUD.m_vrLeftHandTouchVibrateIntensity), (std::min)(1.f, gHUD.m_vrLeftHandTouchVibrateIntensity));
		gHUD.m_vrLeftHandTouchVibrateIntensity = 0.f;
	}

	// RIGHTTOUCH
	if (gHUD.m_vrRightHandTouchVibrateIntensity > 0.f)
	{
		FireFeedback(FeedbackType::LEFTTOUCH, 0, 0.1f, 1.f * (std::max)(1.f, gHUD.m_vrRightHandTouchVibrateIntensity), (std::min)(1.f, gHUD.m_vrRightHandTouchVibrateIntensity));
		gHUD.m_vrRightHandTouchVibrateIntensity = 0.f;
	}

	// RECOIL
	if (recoilintensity > 0.f)
	{
		FireFeedback(FeedbackType::RECOIL, gHUD.m_Health.m_bitsDamage, 0.1f, 4.f * (std::max)(1.f, recoilintensity), (std::min)(1.f, recoilintensity));
		recoilintensity = 0.f;
	}

	// EARTHQUAKE
	if (gHUD.m_hasScreenShake)
	{
		FireFeedback(FeedbackType::EARTHQUAKE, 0, gHUD.m_screenShakeDuration, gHUD.m_screenShakeFrequency, (std::min)(1.f, gHUD.m_screenShakeAmplitude));
		gHUD.m_hasScreenShake = false;
	}

	if (pmove)
	{
		// ONTRAIN
		if (pmove->onground > 0)
		{
			cl_entity_t* groundent = gEngfuncs.GetEntityByIndex(pmove->onground);
			if (groundent && groundent->curstate.solid == SOLID_BSP && groundent->curstate.velocity.Length() > 0.f)
			{
				float intensity = groundent->curstate.velocity.Length() / 100.f;
				FireFeedback(FeedbackType::ONTRAIN, 0, 0.1f, 1.f * (std::max)(1.f, intensity), (std::min)(1.f, intensity));
			}
		}

		// WATERSPLASH
		if ((pmove->oldwaterlevel == 0 && pmove->waterlevel != 0) ||
			(pmove->oldwaterlevel != 0 && pmove->waterlevel == 0))
		{
			FireFeedback(FeedbackType::WATERSPLASH, 0, 0.5f, 2.f, 0.5f);
		}
	}

	// DAMAGE
	if (damageintensity > 0.f)
	{
		FireFeedback(FeedbackType::DAMAGE, gHUD.m_Health.m_bitsDamage, 0.1f, 4.f * (std::max)(1.f, damageintensity), (std::min)(1.f, damageintensity));
	}
}

void VRInput::FireFeedback(FeedbackType feedback, int damageType, float durationInSeconds, float frequency, float amplitude)
{
	bool leftHandMode = CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f;

	vr::VRInputValueHandle_t leftHandHandle = vr::k_ulInvalidInputValueHandle;
	vr::VRInputValueHandle_t rightHandHandle = vr::k_ulInvalidInputValueHandle;
	vr::VRInput()->GetInputSourceHandle("/user/hand/left", &leftHandHandle);
	vr::VRInput()->GetInputSourceHandle("/user/hand/right", &rightHandHandle);

	vr::VRActionHandle_t handle{ 0 };
	vr::VRInputValueHandle_t device = vr::k_ulInvalidInputValueHandle;

	switch (feedback)
	{
	case FeedbackType::LEFTTOUCH:
		handle = m_actionSets["feedback"].feedbackActions["LeftTouch"];
		device = leftHandHandle;
		break;
	case FeedbackType::RIGHTTOUCH:
		handle = m_actionSets["feedback"].feedbackActions["RightTouch"];
		device = rightHandHandle;
		break;
	case FeedbackType::RECOIL:
		handle = m_actionSets["feedback"].feedbackActions["Recoil"];
		device = (leftHandMode) ? leftHandHandle : rightHandHandle;
		break;
	case FeedbackType::EARTHQUAKE:
		handle = m_actionSets["feedback"].feedbackActions["Earthquake"];
		break;
	case FeedbackType::ONTRAIN:
		handle = m_actionSets["feedback"].feedbackActions["TrainShake"];
		break;
	case FeedbackType::WATERSPLASH:
		handle = m_actionSets["feedback"].feedbackActions["WaterSplash"];
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

	// TODO: Collect/Combine vibrations for left and right controllers, and then call 
	// TriggerHapticVibrationAction with /actions/haptic/in/HapticVibration for left and right controller

	//vr::VRInput()->TriggerHapticVibrationAction(handle, 0.f, durationInSeconds, frequency, amplitude, device);

	if (feedback != FeedbackType::DAMAGE)
	{
		//vr::VRInput()->TriggerHapticVibrationAction(m_actionSets["feedback"].feedbackActions["All"], 0.f, durationInSeconds, frequency, amplitude, device);
		if (feedback != FeedbackType::RECOIL)
		{
			//vr::VRInput()->TriggerHapticVibrationAction(m_actionSets["feedback"].feedbackActions["AllButRecoil"], 0.f, durationInSeconds, frequency, amplitude, device);
		}
	}
}

void VRInput::FireDamageFeedback(const std::string& action, float durationInSeconds, float frequency, float amplitude)
{
	//vr::VRInput()->TriggerHapticVibrationAction(m_actionSets["damagefeedback"].feedbackActions[action], 0.f, durationInSeconds, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
}

void VRInput::HandleInput(bool isInGame, bool isInMenu)
{
	m_isInGame = isInGame;
	m_isInMenu = isInMenu;

	m_vrInputThisFrame = false;
	m_fingerSkeletalData.clear();

	std::vector<vr::VRActiveActionSet_t> activeActionSets;
	std::vector<ActionSet*> actionSets;

	for (auto& [actionSetName, actionSet] : m_actionSets)
	{
		if (isInGame || actionSet.handleWhenNotInGame)
		{
			activeActionSets.push_back(vr::VRActiveActionSet_t{ actionSet.handle, vr::k_ulInvalidInputValueHandle, vr::k_ulInvalidActionSetHandle, 0, 0 });
			actionSets.push_back(&actionSet);
		}
	}

	bool thething = !actionSets.empty() && UpdateActionStates(activeActionSets);

	if (thething)
	{
		for (auto& actionSet : actionSets)
		{
			for (auto& [actionName, action] : actionSet->actions)
			{
				action.HandleInput(isInGame && !isInMenu);
			}
		}
	}

	if (isInGame && m_vrInputThisFrame)
	{
		ForceWindowToForeground();
	}

	FireFeedbacks();
}

bool VRInput::UpdateActionStates(std::vector<vr::VRActiveActionSet_t>& actionSets)
{
	vr::EVRInputError result = vr::VRInput()->UpdateActionState(actionSets.data(), sizeof(vr::VRActiveActionSet_t), actionSets.size());
	if (result != vr::VRInputError_None)
	{
		gEngfuncs.Con_DPrintf("Error while trying to get active state for input action sets. (Error code: %i)\n", result);
	}
	return result == vr::VRInputError_None;
}

void VRInput::ExecuteCustomAction(const std::string& action)
{
	if (m_customActions.count(action) == 0)
		return;

	if (m_customActions[action].commands.empty())
		return;

	if (m_customActions[action].currentCommand >= m_customActions[action].commands.size())
		m_customActions[action].currentCommand = 0;

	ClientCmd(m_customActions[action].commands[m_customActions[action].currentCommand].data());

	m_customActions[action].currentCommand++;
}


// Used by weapon code (extern)
float CalculateWeaponTimeOffset(float offset)
{
	float analogfire = fabs(g_vrInput.analogfire);
	if (analogfire > EPSILON&& analogfire < 1.f)
	{
		return offset * 1.f / analogfire;
	}
	return offset;
}

void VRInput::SetFingerSkeletalData(vr::ETrackedControllerRole controllerRole, const float fingerCurl[vr::VRFinger_Count], const float fingerSplay[vr::VRFingerSplay_Count], const vr::VRBoneTransform_t* bones, bool hasFingers, bool hasBones)
{
	std::shared_ptr<FingerSkeletalData> data = std::make_shared<FingerSkeletalData>();
	data->hasFingers = hasFingers;
	data->hasBones = hasBones;
	if (hasFingers)
	{
		for (int i = 0; i < vr::VRFinger_Count; i++)
		{
			data->fingerCurl[i] = fingerCurl[i];
		}
		for (int i = 0; i < vr::VRFingerSplay_Count; i++)
		{
			data->fingerSplay[i] = fingerSplay[i];
		}
	}
	if (hasBones)
	{
		for (int i = 0; i < 31; i++)
		{
			data->bones[i] = bones[i];
		}
	}
	m_fingerSkeletalData[controllerRole] = data;
}

bool VRInput::HasFingerDataForHand(vr::ETrackedControllerRole controllerRole, float fingerCurl[5]) const
{
	{
		auto it = m_fingerSkeletalData.find(controllerRole);
		if (it != m_fingerSkeletalData.end() && it->second && it->second->hasFingers)
		{
			for (int i = 0; i < 5; i++)
			{
				fingerCurl[i] = it->second->fingerCurl[i];
			}
			return true;
		}
	}

	{
		auto it = m_fallbackFingerCurlData.find(controllerRole);
		if (it != m_fallbackFingerCurlData.end())
		{
			for (int i = 0; i < 5; i++)
			{
				fingerCurl[i] = it->second[i];
			}
			return true;
		}
	}

	return false;
}

bool VRInput::HasSkeletalDataForHand(vr::ETrackedControllerRole controllerRole) const
{
	auto it = m_fingerSkeletalData.find(controllerRole);
	return it != m_fingerSkeletalData.end() && it->second && it->second->hasBones;
}

bool VRInput::HasSkeletalDataForHand(vr::ETrackedControllerRole controllerRole, VRQuaternion* bone_quaternions, Vector* bone_positions) const
{
	auto it = m_fingerSkeletalData.find(controllerRole);
	if (it != m_fingerSkeletalData.end() && it->second && it->second->hasBones)
	{
		const Vector3& vrToHL = gVRRenderer.GetHelper()->GetVRToHL();

		for (int i = 0; i < 31; i++)
		{
			vr::HmdQuaternionf_t& boneQuaternionInVRSpace = it->second->bones[i].orientation;
			bone_quaternions[i] = VRQuaternion{
				boneQuaternionInVRSpace.x * vrToHL.x,
				-boneQuaternionInVRSpace.z * vrToHL.z,
				boneQuaternionInVRSpace.y * vrToHL.y,
				boneQuaternionInVRSpace.w
			};

			Vector bonePositionInVRSpace = it->second->bones[i].position.v;
			bone_positions[i] = Vector{
				bonePositionInVRSpace.x * vrToHL.x,
				-bonePositionInVRSpace.z * vrToHL.z,
				bonePositionInVRSpace.y * vrToHL.y
			};
		}
		return true;
	}
	return false;
}

vr::ETrackedControllerRole VRInput::GetRole(vr::VRInputValueHandle_t origin)
{
	vr::InputOriginInfo_t originInfo{ 0 };
	vr::ETrackedControllerRole role = vr::TrackedControllerRole_Invalid;
	if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(origin, &originInfo, sizeof(vr::InputOriginInfo_t)))
	{
		role = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(originInfo.trackedDeviceIndex);
	}
	return role;
}
