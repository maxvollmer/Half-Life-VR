#pragma once

class VRInput
{
public:
	VRInput();
	~VRInput();

	void HandleButtonPress(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp);
};

extern VRInput g_vrInput;
