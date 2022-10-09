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
#if !defined(PMTRACEH)
	#define PMTRACEH
	#ifdef _WIN32
		#pragma once
	#endif

typedef struct
{
	vec3_t normal = { 0 };
	float dist = 0.f;
} pmplane_t;

typedef struct pmtrace_s pmtrace_t;

struct pmtrace_s
{
	qboolean allsolid = 0;         // if true, plane is not valid
	qboolean startsolid = 0;       // if true, the initial point was in a solid area
	qboolean inopen = 0, inwater = 0;  // End point is in empty space or in water
	float fraction = 0.f;            // time completed, 1.0 = didn't hit anything
	vec3_t endpos = { 0 };             // final position
	pmplane_t plane;           // surface normal at impact
	int ent = 0;                   // entity at impact
	vec3_t deltavelocity = { 0 };      // Change in player's velocity caused by impact.
	                           // Only run on server.
	int hitgroup = 0;
};

#endif
