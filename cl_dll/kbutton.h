//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#if !defined(KBUTTONH)
#define KBUTTONH
#pragma once

typedef struct kbutton_s
{
	int down[2] = { 0 };  // key nums holding it down
	int state = 0;    // low bit is down state
} kbutton_t;

#endif  // !KBUTTONH
