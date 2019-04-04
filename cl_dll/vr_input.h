#pragma once

#include "kbutton.h"

#include <unordered_map>
#include <string>

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

private:
	bool m_rotateLeft{ false };
	bool m_rotateRight{ false };
	bool m_isDucking{ false };	// TODO: Controller support for ducking
	bool m_legacyInput{ false };

	struct ActionSet
	{
	public:
		vr::VRActionSetHandle_t										handle;
		std::unordered_map<std::string, vr::VRActionHandle_t>		actions;
	};

	std::unordered_map<std::string, ActionSet>		m_actionSets;
};

extern VRInput g_vrInput;
