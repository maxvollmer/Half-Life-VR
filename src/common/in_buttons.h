/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef IN_BUTTONS_H
#define IN_BUTTONS_H
#ifdef _WIN32
	#pragma once
#endif

constexpr unsigned short IN_ATTACK    = (1 << 0);
constexpr unsigned short IN_JUMP      = (1 << 1);
constexpr unsigned short IN_DUCK      = (1 << 2);
constexpr unsigned short IN_FORWARD   = (1 << 3);
constexpr unsigned short IN_BACK      = (1 << 4);
constexpr unsigned short IN_USE       = (1 << 5);
constexpr unsigned short IN_CANCEL    = (1 << 6);
constexpr unsigned short IN_LEFT      = (1 << 7);
constexpr unsigned short IN_RIGHT     = (1 << 8);
constexpr unsigned short IN_MOVELEFT  = (1 << 9);
constexpr unsigned short IN_MOVERIGHT = (1 << 10);
constexpr unsigned short IN_ATTACK2   = (1 << 11);
constexpr unsigned short IN_RUN       = (1 << 12);
constexpr unsigned short IN_RELOAD    = (1 << 13);
constexpr unsigned short IN_ALT1      = (1 << 14);
constexpr unsigned short IN_SCORE     = (1 << 15);  // Used by client.dll for when scoreboard is held down

constexpr unsigned int X_IN_UP          = (1 << 0);  // Extended button command for VR movement - upmove in ladders or in water
constexpr unsigned int X_IN_DOWN        = (1 << 1);  // Extended button command for VR movement - downmove in ladders or in water
constexpr unsigned int X_IN_VRDUCK      = (1 << 2);  // Extended button command for VR movement - ducking using VR input
constexpr unsigned int X_IN_LETLADDERGO = (1 << 3);  // Extended button command for VR movement - let go off ladder


#endif  // IN_BUTTONS_H
