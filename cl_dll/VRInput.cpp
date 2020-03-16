
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>
#include <fstream>
#include <cctype>
#include <regex>

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


static inline void TrimString(std::string& s)
{
	s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), std::isspace));
	s.erase(std::find_if_not(s.rbegin(), s.rend(), std::isspace).base(), s.end());
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
			gEngfuncs.Con_DPrintf("Error: Couldn't load actions.manifest. (Error code: %i)\n", result);
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("Error: actions.manifest doesn't exist.\n");
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

			auto it = std::find_if(line.begin(), line.end(), std::isspace);
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
				gEngfuncs.Con_DPrintf("Invalid custom command in /vr/actions/customactions.cfg (line %i): %s\n", lineNum, line.data());
			}
		}
	}
}

void VRInput::RegisterActionSets()
{
	if (RegisterActionSet("input", true))
	{
		RegisterAction("input", "MoveForward", &VR::Input::Movement::HandleMoveForward);
		RegisterAction("input", "MoveBackward", &VR::Input::Movement::HandleMoveBackward);
		RegisterAction("input", "MoveLeft", &VR::Input::Movement::HandleMoveLeft);
		RegisterAction("input", "MoveRight", &VR::Input::Movement::HandleMoveRight);
		RegisterAction("input", "MoveUp", &VR::Input::Movement::HandleMoveUp);
		RegisterAction("input", "MoveDown", &VR::Input::Movement::HandleMoveDown);
		RegisterAction("input", "TurnLeft", &VR::Input::Movement::HandleTurnLeft);
		RegisterAction("input", "TurnRight", &VR::Input::Movement::HandleTurnRight);
		RegisterAction("input", "Turn90Left", &VR::Input::Movement::HandleTurn90Left);
		RegisterAction("input", "Turn90Right", &VR::Input::Movement::HandleTurn90Right);
		RegisterAction("input", "Turn45Left", &VR::Input::Movement::HandleTurn45Left);
		RegisterAction("input", "Turn45Right", &VR::Input::Movement::HandleTurn45Right);
		RegisterAction("input", "Turn180", &VR::Input::Movement::HandleTurn180);
		RegisterAction("input", "Jump", &VR::Input::Movement::HandleJump);
		RegisterAction("input", "Crouch", &VR::Input::Movement::HandleCrouch);
		RegisterAction("input", "LongJump", &VR::Input::Movement::HandleLongJump);
		RegisterAction("input", "AnalogJump", &VR::Input::Movement::HandleAnalogJump);
		RegisterAction("input", "AnalogCrouch", &VR::Input::Movement::HandleAnalogCrouch);
		RegisterAction("input", "AnalogLongJump", &VR::Input::Movement::HandleAnalogLongJump);
		RegisterAction("input", "MoveForwardBackward", &VR::Input::Movement::HandleMoveForwardBackward);
		RegisterAction("input", "MoveSideways", &VR::Input::Movement::HandleMoveSideways);
		RegisterAction("input", "MoveUpDown", &VR::Input::Movement::HandleMoveUpDown);
		RegisterAction("input", "Walk", &VR::Input::Movement::HandleWalk);
		RegisterAction("input", "Turn", &VR::Input::Movement::HandleTurn);
		RegisterAction("input", "MoveForwardBackwardSideways", &VR::Input::Movement::HandleMoveForwardBackwardSideways);
		RegisterAction("input", "MoveForwardBackwardTurn", &VR::Input::Movement::HandleMoveForwardBackwardTurn);
		RegisterAction("input", "MoveForwardBackwardSidewaysUpDown", &VR::Input::Movement::HandleMoveForwardBackwardSidewaysUpDown);
		RegisterAction("input", "MoveForwardBackwardTurnUpDown", &VR::Input::Movement::HandleMoveForwardBackwardTurnUpDown);

		RegisterAction("input", "FireWeapon", &VR::Input::Weapons::HandleFire);
		RegisterAction("input", "AltFireWeapon", &VR::Input::Weapons::HandleAltFire);
		RegisterAction("input", "AnalogFireWeapon", &VR::Input::Weapons::HandleAnalogFire);
		RegisterAction("input", "ReloadWeapon", &VR::Input::Weapons::HandleReload);
		RegisterAction("input", "HolsterWeapon", &VR::Input::Weapons::HandleHolster);
		RegisterAction("input", "NextWeapon", &VR::Input::Weapons::HandleNext);
		RegisterAction("input", "PreviousWeapon", &VR::Input::Weapons::HandlePrevious);

		RegisterAction("input", "Teleport", &VR::Input::Other::HandleTeleport);
		RegisterAction("input", "ToggleFlashlight", &VR::Input::Other::HandleFlashlight);
		RegisterAction("input", "LeftGrab", &VR::Input::Other::HandleLeftGrab);
		RegisterAction("input", "RightGrab", &VR::Input::Other::HandleRightGrab);
		RegisterAction("input", "LegacyUse", &VR::Input::Other::HandleLegacyUse);
		RegisterAction("input", "LetGoOffLadder", &VR::Input::Other::HandleLetGoOffLadder);

		RegisterAction("input", "QuickSave", &VR::Input::Other::HandleQuickSave, true);
		RegisterAction("input", "QuickLoad", &VR::Input::Other::HandleQuickLoad, true);
		RegisterAction("input", "RestartCurrentMap", &VR::Input::Other::HandleRestartCurrentMap, true);
		RegisterAction("input", "PauseGame", &VR::Input::Other::HandlePauseGame, true);
		RegisterAction("input", "ExitGame", &VR::Input::Other::HandleExitGame, true);

		RegisterAction("input", "LeftHandSkeleton", &VR::Input::Other::HandleLeftHandSkeleton);
		RegisterAction("input", "RightHandSkeleton", &VR::Input::Other::HandleRightHandSkeleton);
	}
	if (RegisterActionSet("feedback", false))
	{
		RegisterFeedback("feedback", "All");
		RegisterFeedback("feedback", "AllButRecoil");
		RegisterFeedback("feedback", "Touch");
		RegisterFeedback("feedback", "Recoil");
		RegisterFeedback("feedback", "Earthquake");
		RegisterFeedback("feedback", "TrainShake");
		RegisterFeedback("feedback", "WaterSplash");
	}
	if (RegisterActionSet("damagefeedback", false))
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
	if (RegisterActionSet("poses", false))
	{
		RegisterAction("poses", "Flashlight", &VR::Input::Poses::HandleFlashlight);
		RegisterAction("poses", "Movement", &VR::Input::Poses::HandleMovement);
		RegisterAction("poses", "Teleporter", &VR::Input::Poses::HandleTeleporter);
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
	vr::VRActionHandle_t handle{ 0 };

	switch (feedback)
	{
	case FeedbackType::LEFTTOUCH:
		handle = m_actionSets["feedback"].feedbackActions["LeftTouch"];
		break;
	case FeedbackType::RIGHTTOUCH:
		handle = m_actionSets["feedback"].feedbackActions["RightTouch"];
		break;
	case FeedbackType::RECOIL:
		handle = m_actionSets["feedback"].feedbackActions["Recoil"];
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

	vr::VRInput()->TriggerHapticVibrationAction(handle, 0.f, durationInSeconds, frequency, amplitude, vr::k_ulInvalidInputValueHandle);

	if (feedback != FeedbackType::DAMAGE)
	{
		vr::VRInput()->TriggerHapticVibrationAction(m_actionSets["feedback"].feedbackActions["All"], 0.f, durationInSeconds, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
		if (feedback != FeedbackType::RECOIL)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_actionSets["feedback"].feedbackActions["AllButRecoil"], 0.f, durationInSeconds, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
		}
	}
}

void VRInput::FireDamageFeedback(const std::string& action, float durationInSeconds, float frequency, float amplitude)
{
	vr::VRInput()->TriggerHapticVibrationAction(m_actionSets["damagefeedback"].feedbackActions[action], 0.f, durationInSeconds, frequency, amplitude, vr::k_ulInvalidInputValueHandle);
}

void VRInput::HandleInput(bool isInGame)
{
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
				action.HandleInput(isInGame);
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

void VRInput::SetFingerSkeletalData(vr::ETrackedControllerRole controllerRole, const float fingerCurl[vr::VRFinger_Count])
{
	for (int i = 0; i < 5; i++)
		m_fingerSkeletalData[controllerRole][i] = fingerCurl[i];
}

bool VRInput::HasSkeletalDataForHand(vr::ETrackedControllerRole controllerRole, float fingerCurl[5]) const
{
	auto it = m_fingerSkeletalData.find(controllerRole);
	if (it != m_fingerSkeletalData.end())
	{
		for (int i = 0; i < 5; i++)
			fingerCurl[i] = it->second[i];
		return true;
	}
	return false;
}
