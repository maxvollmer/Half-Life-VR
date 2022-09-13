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
#ifndef PROGDEFS_H
#define PROGDEFS_H
#ifdef _WIN32
	#pragma once
#endif

typedef struct
{
	float time = 0.f;
	float frametime = 0.f;
	float force_retouch = 0.f;
	string_t mapname = 0;
	string_t startspot = 0;
	float deathmatch = 0.f;
	float coop = 0.f;
	float teamplay = 0.f;
	float serverflags = 0.f;
	float found_secrets = 0.f;
	vec3_t v_forward;
	vec3_t v_up;
	vec3_t v_right;
	float trace_allsolid = 0.f;
	float trace_startsolid = 0.f;
	float trace_fraction = 0.f;
	vec3_t trace_endpos;
	vec3_t trace_plane_normal;
	float trace_plane_dist = 0.f;
	edict_t* trace_ent = nullptr;
	float trace_inopen = 0.f;
	float trace_inwater = 0.f;
	int trace_hitgroup = 0;
	int trace_flags = 0;
	int msg_entity = 0;
	int cdAudioTrack = 0;
	int maxClients = 0;
	int maxEntities = 0;
	const char* pStringBase = nullptr;

	void* pSaveData = nullptr;
	vec3_t vecLandmarkOffset;
} globalvars_t;


typedef struct entvars_s
{
	string_t classname = 0;
	string_t globalname = 0;

	vec3_t origin;
	vec3_t oldorigin;
	vec3_t velocity;
	vec3_t basevelocity;
	vec3_t clbasevelocity;  // Base velocity that was passed in to server physics so
	                        //  client can predict conveyors correctly.  Server zeroes it, so we need to store here, too.
	vec3_t movedir;

	vec3_t angles;      // Model angles
	vec3_t avelocity;   // angle velocity (degrees per second)
	vec3_t punchangle;  // auto-decaying view angle adjustment
	vec3_t v_angle;     // Viewing angle (player only)

	// For parametric entities
	vec3_t endpos;
	vec3_t startpos;
	float impacttime = 0.f;
	float starttime = 0.f;

	int fixangle = 0;  // 0:nothing, 1:force view angles, 2:add avelocity
	float idealpitch = 0.f;
	float pitch_speed = 0.f;
	float ideal_yaw = 0.f;
	float yaw_speed = 0.f;

	int modelindex = 0;
	string_t model = 0;

	int viewmodel = 0;    // player's viewmodel
	int weaponmodel = 0;  // what other players see

	vec3_t absmin;  // BB max translated to world coord
	vec3_t absmax;  // BB max translated to world coord
	vec3_t mins;    // local BB min
	vec3_t maxs;    // local BB max
	vec3_t size;    // maxs - mins

	float ltime = 0.f;
	float nextthink = 0.f;

	int movetype = 0;
	int solid = 0;

	int skin = 0;
	int body = 0;  // sub-model selection for studiomodels
	int effects = 0;

	float gravity = 0.f;   // % of "normal" gravity
	float friction = 0.f;  // inverse elasticity of MOVETYPE_BOUNCE

	int light_level = 0;

	int sequence = 0;        // animation sequence
	int gaitsequence = 0;    // movement animation sequence for player (0 for none)
	float frame = 0.f;         // % playback position in animation sequences (0..255)
	float animtime = 0.f;      // world time when frame was set
	float framerate = 0.f;     // animation playback rate (-8x to 8x)
	byte controller[4] = { 0 };  // bone controller setting (0..255)
	byte blending[2] = { 0 };    // blending amount between sub-sequences (0..255)

	float scale = 0.f;  // sprite rendering scale (0..255)

	int rendermode = 0;
	float renderamt = 0.f;
	vec3_t rendercolor;
	int renderfx = 0;

	float health = 0.f;
	float frags = 0.f;
	int weapons = 0;  // bit mask for available weapons
	float takedamage = 0.f;

	int deadflag = 0;
	vec3_t view_ofs;  // eye position

	int button = 0;
	int impulse = 0;

	edict_t* chain = nullptr;  // Entity pointer when linked into a linked list
	edict_t* dmg_inflictor = nullptr;
	edict_t* enemy = nullptr;
	edict_t* aiment = nullptr;  // entity pointer when MOVETYPE_FOLLOW
	edict_t* owner = nullptr;
	edict_t* groundentity = nullptr;

	int spawnflags = 0;
	int flags = 0;

	int colormap = 0;  // lowbyte topcolor, highbyte bottomcolor
	int team = 0;

	float max_health = 0.f;
	float teleport_time = 0.f;
	float armortype = 0.f;
	float armorvalue = 0.f;
	int waterlevel = 0;
	int watertype = 0;

	string_t target = 0;
	string_t targetname = 0;
	string_t netname = 0;
	string_t message = 0;

	float dmg_take = 0.f;
	float dmg_save = 0.f;
	float dmg = 0.f;
	float dmgtime = 0.f;

	string_t noise = 0;
	string_t noise1 = 0;
	string_t noise2 = 0;
	string_t noise3 = 0;

	float speed = 0.f;
	float air_finished = 0.f;
	float pain_finished = 0.f;
	float radsuit_finished = 0.f;

	edict_t* pContainingEntity = nullptr;

	int playerclass = 0;
	float maxspeed = 0.f;

	float fov = 0.f;
	int weaponanim = 0;

	int pushmsec = 0;

	int bInDuck = 0;
	int flTimeStepSound = 0;
	int flSwimTime = 0;
	int flDuckTime = 0;
	int iStepLeft = 0;
	float flFallVelocity = 0.f;

	int gamestate = 0;

	int oldbuttons = 0;

	int groupinfo = 0;

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
	edict_t* euser1 = nullptr;
	edict_t* euser2 = nullptr;
	edict_t* euser3 = nullptr;
	edict_t* euser4 = nullptr;
} entvars_t;


#endif  // PROGDEFS_H
