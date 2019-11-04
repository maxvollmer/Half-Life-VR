//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

// Completely cleaned out this file as all movement and input comes from the VR setup - Max Vollmer, 2017-08-17

#include "in_defs.h"

extern "C"
{
	void DLLEXPORT IN_ActivateMouse(void);
	void DLLEXPORT IN_DeactivateMouse(void);
	void DLLEXPORT IN_MouseEvent(int mstate);
	void DLLEXPORT IN_Accumulate(void);
	void DLLEXPORT IN_ClearStates(void);
}

void IN_ActivateMouse() {}
void IN_DeactivateMouse() {}
void IN_MouseEvent(int mstate) {}
void IN_Accumulate() {}
void IN_ClearStates() {}
void IN_ResetMouse() {}
void IN_Commands() {}
void IN_Init() {}
void IN_Move(float f, struct usercmd_s* cmd) {}
void IN_Shutdown(void) {}
