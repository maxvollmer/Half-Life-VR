//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "in_defs.h"

extern "C"
{
	void DLLEXPORT CAM_Think(void);
	int DLLEXPORT CL_IsThirdPerson(void);
	void DLLEXPORT CL_CameraOffset(float* ofs);
}

void DLLEXPORT CAM_Think(void)
{
}

int DLLEXPORT CL_IsThirdPerson(void)
{
	return 0;
}

void DLLEXPORT CL_CameraOffset(float* ofs)
{
}
