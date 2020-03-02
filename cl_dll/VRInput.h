#pragma once

#include <unordered_map>
#include <string>

#include "openvr/openvr.h"

#include "VRInputAction.h"
#include "VRInputHandlers.h"

#include "kbutton.h"

class VRInput
{
public:
	void Init();

	void HandleInput(bool isInGame);

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

	void SetFingerSkeletalData(vr::ETrackedControllerRole controllerRole, const float fingerCurl[vr::VRFinger_Count]);
	bool HasSkeletalDataForHand(vr::ETrackedControllerRole controllerRole, float fingerCurl[5]) const;

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

private:
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

	std::unordered_map<std::string, ActionSet> m_actionSets;
	std::unordered_map<std::string, CustomAction> m_customActions;

	std::unordered_map<vr::ETrackedControllerRole, bool> m_dragStates;

	std::unordered_map<vr::ETrackedControllerRole, float[5]> m_fingerSkeletalData;
};

extern VRInput g_vrInput;
