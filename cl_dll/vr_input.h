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

	void LegacyHandleButtonPress(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp);
	void LegacyHandleButtonTouch(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp);

	void HandleInput();

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
	inline bool IsDucking() const
	{
		extern kbutton_t in_duck;
		return m_isDucking || (in_duck.state & 3);
	}
	inline bool IsDragOn(vr::TrackedDeviceIndex_t controllerIndex) const
	{
		// TODO
		return false;
	}
	inline bool IsLegacyInput() const
	{
		return m_legacyInput;
	}

	enum class FeedbackType
	{
		RECOIL,
		EARTHQUAKE,
		ONTRAIN,
		WATERSPLASH,
		DAMAGE
	};

	void FireFeedback(FeedbackType feedback, int damageType, float durationInSeconds, float frequency, float amplitude);

private:
	void RegisterActionSets();
	bool RegisterActionSet(const std::string& actionSet);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::DigitalActionHandler handler);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::AnalogActionHandler handler);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::PoseActionHandler handler);
	bool RegisterFeedback(const std::string& actionSet, const std::string& action);

	void FireDamageFeedback(const std::string& action, float durationInSeconds, float frequency, float amplitude);

	void UpdateActionStates();

	bool m_rotateLeft{ false };
	bool m_rotateRight{ false };
	bool m_isDucking{ false };	// TODO: Controller support for ducking
	bool m_legacyInput{ false };

	struct ActionSet
	{
	public:
		vr::VRActionSetHandle_t									handle;
		std::unordered_map<std::string, VRInputAction>			actions;
		std::unordered_map<std::string, vr::VRActionHandle_t>	feedbackActions;
	};

	std::unordered_map<std::string, ActionSet>		m_actionSets;
};

extern VRInput g_vrInput;
