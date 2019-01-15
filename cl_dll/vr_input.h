#pragma once

#include "kbutton.h"

class VRInput
{
public:
	VRInput();
	~VRInput();

	void HandleButtonPress(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp);

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

private:
	bool m_rotateLeft{ false };
	bool m_rotateRight{ false };
	bool m_isDucking{ false };	// TODO: Controller support for ducking
};

extern VRInput g_vrInput;
