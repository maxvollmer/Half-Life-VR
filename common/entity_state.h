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
#if !defined(ENTITY_STATEH)
	#define ENTITY_STATEH
	#ifdef _WIN32
		#pragma once
	#endif

    // For entityType below
	#define ENTITY_NORMAL (1 << 0)
	#define ENTITY_BEAM   (1 << 1)

// Entity state is used for the baseline and for delta compression of a packet of
//  entities that is sent to a client.
typedef struct entity_state_s entity_state_t;

struct entity_state_s
{
	// Fields which are filled in by routines outside of delta compression
	int entityType = 0;
	// Index into cl_entities array for this entity.
	int number = 0;
	float msg_time = 0.f;

	// Message number last time the player/entity state was updated.
	int messagenum = 0;

	// Fields which can be transitted and reconstructed over the network stream
	vec3_t origin;
	vec3_t angles;

	int modelindex = 0;
	int sequence = 0;
	float frame = 0.f;
	int colormap = 0;
	short skin = 0;
	short solid = 0;
	int effects = 0;
	float scale = 0.f;

	byte eflags = 0;

	// Render information
	int rendermode = 0;
	int renderamt = 0;
	color24 rendercolor;
	int renderfx = 0;

	int movetype = 0;
	float animtime = 0.f;
	float framerate = 0.f;
	int body = 0;
	byte controller[4] = { 0 };
	byte blending[4] = { 0 };
	vec3_t velocity;

	// Send bbox down to client for use during prediction.
	vec3_t mins;
	vec3_t maxs;

	int aiment = 0;
	// If owned by a player, the index of that player ( for projectiles ).
	int owner = 0;

	// Friction, for prediction.
	float friction = 0.f;
	// Gravity multiplier
	float gravity = 0.f;

	// PLAYER SPECIFIC
	int team = 0;
	int playerclass = 0;
	int health = 0;
	qboolean spectator;
	int weaponmodel = 0;
	int gaitsequence = 0;
	// If standing on conveyor, e.g.
	vec3_t basevelocity;
	// Use the crouched hull, or the regular player hull.
	int usehull = 0;
	// Latched buttons last time state updated.
	int oldbuttons = 0;
	// -1 = in air, else pmove entity number
	int onground = 0;
	int iStepLeft = 0;
	// How fast we are falling
	float flFallVelocity = 0.f;

	float fov = 0.f;
	int weaponanim = 0;

	// Parametric movement overrides
	vec3_t startpos;
	vec3_t endpos;
	float impacttime = 0.f;
	float starttime = 0.f;

	// For mods
	int iuser1 = 0;
	int iuser2 = 0;
	int iuser3 = 0;
	int iuser4 = 0;
	float fuser1 = 0.f;
	float fuser2 = 0.f;
	float fuser3 = 0.f;
	float fuser4 = 0.f;
	vec3_t vuser1;
	vec3_t vuser2;
	vec3_t vuser3;
	vec3_t vuser4;
};

	#include "pm_info.h"

typedef struct clientdata_s
{
	vec3_t origin;
	vec3_t velocity;

	int viewmodel = 0;
	vec3_t punchangle;
	int flags = 0;
	int waterlevel = 0;
	int watertype = 0;
	vec3_t view_ofs;
	float health = 0.f;

	int bInDuck = 0;

	int weapons = 0;  // remove?

	int flTimeStepSound = 0;
	int flDuckTime = 0;
	int flSwimTime = 0;
	int waterjumptime = 0;

	float maxspeed = 0.f;

	float fov = 0.f;
	int weaponanim = 0;

	int m_iId = 0;
	int ammo_shells = 0;
	int ammo_nails = 0;
	int ammo_cells = 0;
	int ammo_rockets = 0;
	float m_flNextAttack = 0.f;

	int tfstate = 0;

	int pushmsec = 0;

	int deadflag = 0;

	char physinfo[MAX_PHYSINFO_STRING];

	// For mods
	int iuser1 = 0;
	int iuser2 = 0;
	int iuser3 = 0;
	int iuser4 = 0;
	float fuser1 = 0.f;
	float fuser2 = 0.f;
	float fuser3 = 0.f;
	float fuser4 = 0.f;
	vec3_t vuser1;
	vec3_t vuser2;
	vec3_t vuser3;
	vec3_t vuser4;
} clientdata_t;

	#include "weaponinfo.h"

typedef struct local_state_s
{
	entity_state_t playerstate;
	clientdata_t client;
	weapon_data_t weapondata[32] = { 0 };
} local_state_t;

#endif  // !ENTITY_STATEH
