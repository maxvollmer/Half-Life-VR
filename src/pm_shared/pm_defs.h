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
// pm_defs.h
#if !defined(PM_DEFSH)
#define PM_DEFSH
#pragma once

#define MAX_PHYSENTS    600  // Must have room for all entities in the world.
#define MAX_MOVEENTS    64
#define MAX_CLIP_PLANES 5

#define PM_NORMAL        0x00000000
#define PM_STUDIO_IGNORE 0x00000001  // Skip studio models
#define PM_STUDIO_BOX    0x00000002  // Use boxes for non-complex studio models (even in traceline)
#define PM_GLASS_IGNORE  0x00000004  // Ignore entities with non-normal rendermode
#define PM_WORLD_ONLY    0x00000008  // Only trace against the world

// Values for flags parameter of PM_TraceLine
#define PM_TRACELINE_PHYSENTSONLY 0
#define PM_TRACELINE_ANYVISIBLE   1


#include "pm_info.h"

// PM_PlayerTrace results.
#include "pmtrace.h"

#if !defined(USERCMD_H)
#include "usercmd.h"
#endif

// physent_t
typedef struct physent_s
{
	char name[32] = { 0 };  // Name of model, or "player" or "world".
	int player = 0;
	vec3_t origin;                // Model's origin in world coordinates.
	struct model_s* model = nullptr;        // only for bsp models
	struct model_s* studiomodel = nullptr;  // SOLID_BBOX, but studio clip intersections.
	vec3_t mins, maxs;            // only for non-bsp models
	int info = 0;                     // For client or server to use to identify (index into edicts or cl_entities)
	vec3_t angles;                // rotated entities need this info for hull testing to work.

	int solid = 0;       // Triggers and func_door type WATER brushes are SOLID_NOT
	int skin = 0;        // BSP Contents for such things like fun_door water brushes.
	int rendermode = 0;  // So we can ignore glass

	// Complex collision detection.
	float frame = 0.f;
	int sequence = 0;
	byte controller[4] = { 0 };
	byte blending[2] = { 0 };

	int movetype = 0;
	int takedamage = 0;
	int blooddecal = 0;
	int team = 0;
	int classnumber = 0;

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
} physent_t;


typedef struct playermove_s
{
	int player_index = 0;  // So we don't try to run the PM_CheckStuck nudging too quickly.
	qboolean server = 0;   // For debugging, are we running physics code on server side?

	qboolean multiplayer = 0;  // 1 == multiplayer server
	float time = 0.f;            // realtime on host, for reckoning duck timing
	float frametime = 0.f;       // Duration of this frame

	vec3_t forward, right, up;  // Vectors for angles
	// player state
	vec3_t origin;        // Movement origin.
	vec3_t angles;        // Movement view angles.
	vec3_t oldangles;     // Angles before movement view angles were looked at.
	vec3_t velocity;      // Current movement direction.
	vec3_t movedir;       // For waterjumping, a forced forward velocity so we can fly over lip of ledge.
	vec3_t basevelocity;  // Velocity of the conveyor we are standing, e.g.

	// For ducking/dead
	vec3_t view_ofs;   // Our eye position.
	float flDuckTime = 0.f;  // Time we started duck
	qboolean bInDuck = 0;  // In process of ducking or ducked already?

	// For walking/falling
	int flTimeStepSound = 0;  // Next time we can play a step sound
	int iStepLeft = 0;

	float flFallVelocity = 0.f;
	vec3_t punchangle;

	float flSwimTime = 0.f;

	float flNextPrimaryAttack = 0.f;

	int effects = 0;  // MUZZLE FLASH, e.g.

	int flags = 0;      // FL_ONGROUND, FL_DUCKING, etc.
	int usehull = 0;    // 0 = regular player hull, 1 = ducked player hull, 2 = point hull
	float gravity = 0.f;  // Our current gravity and friction.
	float friction = 0.f;
	int oldbuttons = 0;       // Buttons last usercmd
	float waterjumptime = 0.f;  // Amount of time left in jumping out of water cycle.
	qboolean dead = 0;        // Are we a dead player?
	int deadflag = 0;
	int spectator = 0;  // Should we use spectator physics model?
	int movetype = 0;   // Our movement type, NOCLIP, WALK, FLY

	int onground = 0;
	int waterlevel = 0;
	int watertype = 0;
	int oldwaterlevel = 0;

	char sztexturename[256] = { 0 };
	char chtexturetype = 0;

	float maxspeed = 0.f;
	float clientmaxspeed = 0.f;  // Player specific maxspeed

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
	// world state
	// Number of entities to clip against.
	int numphysent = 0;
	physent_t physents[MAX_PHYSENTS] = { 0 };
	// Number of momvement entities (ladders)
	int nummoveent = 0;
	// just a list of ladders
	physent_t moveents[MAX_MOVEENTS] = { 0 };

	// All things being rendered, for tracing against things you don't actually collide with
	int numvisent = 0;
	physent_t visents[MAX_PHYSENTS] = { 0 };

	// input to run through physics.
	usercmd_t cmd;

	// Trace results for objects we collided with.
	int numtouch = 0;
	pmtrace_t touchindex[MAX_PHYSENTS] = { 0 };

	char physinfo[MAX_PHYSINFO_STRING] = { 0 };  // Physics info string

	struct movevars_s* movevars = nullptr;
	vec3_t player_mins[4] = { 0 };
	vec3_t player_maxs[4] = { 0 };

	// Common functions
	const char* (*PM_Info_ValueForKey)(const char* s, const char* key);
	void (*PM_Particle)(float* origin, int color, float life, int zpos, int zvel);
	int (*PM_TestPlayerPosition)(const float* pos, pmtrace_t* ptrace);
	void (*Con_NPrintf)(int idx, char* fmt, ...);
	void (*Con_DPrintf)(char* fmt, ...);
	void (*Con_Printf)(char* fmt, ...);
	double (*Sys_FloatTime)(void);
	void (*PM_StuckTouch)(int hitent, pmtrace_t* ptraceresult);
	int (*PM_PointContents)(float* p, int* truecontents /*filled in if this is non-null*/);
	int (*PM_TruePointContents)(float* p);
	int (*PM_HullPointContents)(struct hull_s* hull, int num, float* p);
	pmtrace_t(*PM_PlayerTrace)(float* start, float* end, int traceFlags, int ignore_pe);
	struct pmtrace_s* (*PM_TraceLine)(float* start, float* end, int flags, int usehulll, int ignore_pe);
	long (*RandomLong)(long lLow, long lHigh);
	float (*RandomFloat)(float flLow, float flHigh);
	int (*PM_GetModelType)(struct model_s* mod);
	void (*PM_GetModelBounds)(struct model_s* mod, float* mins, float* maxs);
	void* (*PM_HullForBsp)(physent_t* pe, float* offset);
	float (*PM_TraceModel)(physent_t* pEnt, float* start, float* end, trace_t* trace);
	int (*COM_FileSize)(char* filename);
	byte* (*COM_LoadFile)(char* path, int usehunk, int* pLength);
	void (*COM_FreeFile)(void* buffer);
	char* (*memfgets)(byte* pMemFile, int fileSize, int* pFilePos, char* pBuffer, int bufferSize);

	// Functions
	// Run functions for this frame?
	qboolean runfuncs;
	void (*PM_PlaySound)(int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch);
	const char* (*PM_TraceTexture)(int ground, float* vstart, float* vend);
	void (*PM_PlaybackEventFull)(int flags, int clientindex, unsigned short eventindex, float delay, float* origin, float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);

	pmtrace_t(*PM_PlayerTraceEx)(float* start, float* end, int traceFlags, int (*pfnIgnore)(physent_t* pe));
	int (*PM_TestPlayerPositionEx)(float* pos, pmtrace_t* ptrace, int (*pfnIgnore)(physent_t* pe));
	struct pmtrace_s* (*PM_TraceLineEx)(float* start, float* end, int flags, int usehulll, int (*pfnIgnore)(physent_t* pe));
} playermove_t;

#endif
