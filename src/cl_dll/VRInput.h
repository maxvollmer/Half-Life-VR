#pragma once

#include <unordered_map>
#include <string>
#include <memory>

#include "openvr/openvr.h"

#include "../vr_shared/VRShared.h"

#include "VRInputAction.h"
#include "VRInputHandlers.h"

#include "kbutton.h"

#ifndef MAXSTUDIOBONES
#define MAXSTUDIOBONES 128
#endif

class VRInput
{
public:
	void Init();

	void HandleInput(bool isInGame, bool isInMenu);

	inline bool RotateLeft() const
	{
		extern kbutton_t in_left;
		return m_rotateLeft || (in_left.state & 3);
	}
	inline bool RotateRight() const
	{
		extern kbutton_t in_right;
		return m_rotateRight || (in_right.state & 3);
	}
	inline bool IsDragOn(vr::ETrackedControllerRole controllerRole) const
	{
		const auto& state = m_dragStates.find(controllerRole);
		if (state != m_dragStates.end())
			return state->second;
		else
			return false;
	}
	void SetDrag(vr::ETrackedControllerRole controllerRole, bool on)
	{
		m_dragStates[controllerRole] = on;
	}
	inline bool IsVRDucking()
	{
		return m_isVRDucking;
	}
	inline void SetVRDucking(bool isVRDucking)
	{
		m_isVRDucking = isVRDucking;
	}
	inline bool ShouldLetGoOffLadder()
	{
		return m_letGoOffLadder;
	}
	inline void SetLetGoOffLadder(bool letGoOffLadder)
	{
		m_letGoOffLadder = letGoOffLadder;
	}
	inline void SetVRMenuClickStatus(bool vrMenuClickStatus)
	{
		m_vrMenuClickStatus = vrMenuClickStatus;
	}
	inline bool GetVRMenuClickStatus()
	{
		return m_vrMenuClickStatus;
	}
	inline void SetVRMenuUseStatus(bool vrMenuUseStatus)
	{
		m_vrMenuUseStatus = vrMenuUseStatus;
	}
	inline bool GetVRMenuUseStatus()
	{
		return m_vrMenuUseStatus;
	}
	inline void SetVRMenuFireStatus(bool vrMenuFireStatus)
	{
		m_vrMenuFireStatus = vrMenuFireStatus;
	}
	inline bool GetVRMenuFireStatus()
	{
		return m_vrMenuFireStatus;
	}

	enum class FeedbackType
	{
		LEFTTOUCH,
		RIGHTTOUCH,
		RECOIL,
		EARTHQUAKE,
		ONTRAIN,
		WATERSPLASH,
		DAMAGE
	};

	void SetFingerSkeletalData(vr::ETrackedControllerRole controllerRole, const float fingerCurl[vr::VRFinger_Count], const float fingerSplay[vr::VRFingerSplay_Count], const vr::VRBoneTransform_t* bones, bool hasFingers, bool hasBones);
	bool HasFingerDataForHand(vr::ETrackedControllerRole controllerRole, float fingerCurl[5]) const;
	bool HasSkeletalDataForHand(vr::ETrackedControllerRole controllerRole) const;
	bool HasSkeletalDataForHand(vr::ETrackedControllerRole controllerRole, VRQuaternion* bone_quaternions, Vector* bone_positions) const;

	void ExecuteCustomAction(const std::string& action);

	bool m_rotateLeft{ false };
	bool m_rotateRight{ false };

	float analogforward{ 0.f };
	float analogsidemove{ 0.f };
	float analogupmove{ 0.f };
	float analogturn{ 0.f };
	float analogfire{ 0.f };

	void ShowHLMenu();
	void HideHLMenu();

	void ForceWindowToForeground();
	void DisplayErrorPopup(const char* errorMessage);

	bool m_vrInputThisFrame = false;

	float damageintensity = 0.f;
	float recoilintensity = 0.f;

	inline bool IsInGame() { return m_isInGame; }
	inline bool IsInMenu() { return m_isInMenu; }

private:
	class FingerSkeletalData
	{
	public:
		float fingerCurl[vr::VRFinger_Count];
		float fingerSplay[vr::VRFingerSplay_Count];
		vr::VRBoneTransform_t bones[31];
		bool hasFingers{ false };
		bool hasBones{ false };
	};

	struct ActionSet
	{
	public:
		vr::VRActionSetHandle_t handle{ vr::k_ulInvalidActionSetHandle };
		std::unordered_map<std::string, VRInputAction> actions;
		std::unordered_map<std::string, vr::VRActionHandle_t> feedbackActions;
		bool handleWhenNotInGame{ false };
	};

	struct CustomAction
	{
	public:
		std::string name;
		std::vector<std::string> commands;
		size_t currentCommand{ 0 };
	};

	void LoadCustomActions();
	void RegisterActionSets();
	bool RegisterActionSet(const std::string& actionSet, bool handleWhenNotInGame);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::DigitalActionHandler handler, bool handleWhenNotInGame = false);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::AnalogActionHandler handler, bool handleWhenNotInGame = false);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::PoseActionHandler handler, bool handleWhenNotInGame = false);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::SkeletalActionHandler handler, bool handleWhenNotInGame = false);
	bool RegisterFeedback(const std::string& actionSet, const std::string& action);

	vr::VROverlayHandle_t m_hlMenu{ vr::k_ulOverlayHandleInvalid };
	bool m_isHLMenuShown{ false };
	bool m_vrMenuKeyboardOpen{ false };
	void CreateHLMenu();
	void HandleHLMenuInput();

	void SendMousePosToHLWindow(float x, float y);
	void SendMouseButtonClickToHLWindow(float x, float y);

	void FireFeedbacks();
	void FireFeedback(FeedbackType feedback, int damageType, float durationInSeconds, float frequency, float amplitude);
	void FireDamageFeedback(const std::string& action, float durationInSeconds, float frequency, float amplitude);

	bool UpdateActionStates(std::vector<vr::VRActiveActionSet_t>& actionSets);

	bool m_isDucking{ false };
	bool m_isVRDucking{ false };
	bool m_letGoOffLadder{ false };
	bool m_vrMenuClickStatus{ false };
	bool m_vrMenuUseStatus{ false };
	bool m_vrMenuFireStatus{ false };

	std::unordered_map<std::string, ActionSet> m_actionSets;
	std::unordered_map<std::string, CustomAction> m_customActions;

	std::unordered_map<vr::ETrackedControllerRole, bool> m_dragStates;

	std::unordered_map<vr::ETrackedControllerRole, std::shared_ptr<FingerSkeletalData>> m_fingerSkeletalData;

	bool m_isInGame{ false };
	bool m_isInMenu{ true };
};

extern VRInput g_vrInput;
