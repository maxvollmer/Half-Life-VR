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
#ifndef USERCMD_H
#define USERCMD_H
#ifdef _WIN32
	#pragma once
#endif

typedef struct usercmd_s
{
	short lerp_msec = 0;    // Interpolation time on client
	byte msec = 0;          // Duration in ms of command
	vec3_t viewangles;  // Command view angles.

	// intended velocities
	float forwardmove = 0.f;       // Forward velocity.
	float sidemove = 0.f;          // Sideways velocity.
	float upmove = 0.f;            // Upward velocity.
	byte lightlevel = 0;         // Light level at spot where we are standing.
	unsigned short buttons = 0;  // Attack buttons
	byte impulse = 0;            // Impulse command issued.
	byte weaponselect = 0;       // Current weapon id

	// Experimental player impact stuff.
	//int		impact_index;
	unsigned int buttons_ex = 0;  // renamed to "buttons_ex" and used now for adding more buttons in VR mode
	vec3_t impact_position;
} usercmd_t;

#endif  // USERCMD_H
