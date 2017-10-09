#pragma once

class VRInput
{
public:
	VRInput();
	~VRInput();

	cvar_t	*vr_control_alwaysforward;
	cvar_t	*vr_control_deadzone;
	cvar_t	*vr_control_teleport;
	cvar_t	*vr_control_hand;


	void HandleButtonPress(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp);
	void HandleTrackpad(unsigned int button, vr::VRControllerState_t controllerState, bool leftOrRight, bool downOrUp);
private:
	bool isTeleActive;
};

extern VRInput g_vrInput;
