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
	inline bool IsLegacyInput() const
	{
		return m_isLegacyInput;
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

	void ExecuteCustomAction(const std::string& action);

	bool m_rotateLeft{ false };
	bool m_rotateRight{ false };

	float analogforward{ 0.f };
	float analogsidemove{ 0.f };
	float analogupmove{ 0.f };
	float analogturn{ 0.f };
	float analogfire{ 0.f };

private:
	struct ActionSet
	{
	public:
		vr::VRActionSetHandle_t									handle;
		std::unordered_map<std::string, VRInputAction>			actions;
		std::unordered_map<std::string, vr::VRActionHandle_t>	feedbackActions;
	};

	struct CustomAction
	{
	public:
		std::string					name;
		std::vector<std::string>	commands;
		size_t						currentCommand{ 0 };
	};

	void LoadCustomActions();
	void RegisterActionSets();
	bool RegisterActionSet(const std::string& actionSet);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::DigitalActionHandler handler);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::AnalogActionHandler handler);
	bool RegisterAction(const std::string& actionSet, const std::string& action, VRInputAction::PoseActionHandler handler);
	bool RegisterFeedback(const std::string& actionSet, const std::string& action);

	void FireDamageFeedback(const std::string& action, float durationInSeconds, float frequency, float amplitude);

	bool UpdateActionStates(const std::string& actionSetName, const ActionSet& actionSet);

	bool m_isDucking{ false };	// TODO: Controller support for ducking
	bool m_isLegacyInput{ false };

	std::unordered_map<std::string, ActionSet>				m_actionSets;
	std::unordered_map<std::string, CustomAction>			m_customActions;

	std::unordered_map<vr::ETrackedControllerRole, bool>	m_dragStates;
};

extern VRInput g_vrInput;
