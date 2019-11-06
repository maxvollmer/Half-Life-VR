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
// cl_entity.h
#if !defined(CL_ENTITYH)
	#define CL_ENTITYH
	#ifdef _WIN32
		#pragma once
	#endif

typedef struct efrag_s
{
	struct mleaf_s* leaf = nullptr;
	struct efrag_s* leafnext = nullptr;
	struct cl_entity_s* entity = nullptr;
	struct efrag_s* entnext = nullptr;
} efrag_t;

typedef struct
{
	byte mouthopen = 0;  // 0 = mouth closed, 255 = mouth agape
	byte sndcount = 0;   // counter for running average
	int sndavg = 0;      // running average
} mouth_t;

typedef struct
{
	float prevanimtime = 0.f;
	float sequencetime = 0.f;
	byte prevseqblending[2];
	vec3_t prevorigin;
	vec3_t prevangles;

	int prevsequence = 0;
	float prevframe = 0.f;

	byte prevcontroller[4] = { 0 };
	byte prevblending[2] = { 0 };
} latchedvars_t;

typedef struct
{
	// Time stamp for this movement
	float animtime = 0.f;

	vec3_t origin;
	vec3_t angles;
} position_history_t;

typedef struct cl_entity_s cl_entity_t;

	#define HISTORY_MAX  64  // Must be power of 2
	#define HISTORY_MASK (HISTORY_MAX - 1)


	#if !defined(ENTITY_STATEH)
		#include "entity_state.h"
	#endif

	#if !defined(PROGS_H)
		#include "progs.h"
	#endif

struct cl_entity_s
{
	int index = 0;  // Index into cl_entities ( should match actual slot, but not necessarily )

	qboolean player = 0;  // True if this entity is a "player"

	entity_state_t baseline;   // The original state from which to delta during an uncompressed message
	entity_state_t prevstate;  // The state information from the penultimate message received from the server
	entity_state_t curstate;   // The state information from the last message received from server

	int current_position = 0;                // Last received history update index
	position_history_t ph[HISTORY_MAX];  // History of position and angle updates for this player

	mouth_t mouth;  // For synchronizing mouth movements.

	latchedvars_t latched;  // Variables used by studio model rendering routines

	// Information based on interplocation, extrapolation, prediction, or just copied from last msg received.
	//
	float lastmove = 0.f;

	// Actual render position and angles
	vec3_t origin;
	vec3_t angles;

	// Attachment points
	vec3_t attachment[4];

	// Other entity local information
	int trivial_accept = 0;

	struct model_s* model = nullptr;    // cl.model_precache[ curstate.modelindes ];  all visible entities have a model
	struct efrag_s* efrag = nullptr;    // linked list of efrags
	struct mnode_s* topnode = nullptr;  // for bmodels, first world node that splits bmodel, or NULL if not split

	float syncbase = 0.f;  // for client-side animations -- used by obsolete alias animation system, remove?
	int visframe = 0;    // last frame this entity was found in an active leaf
	colorVec cvFloorColor;
};

#endif  // !CL_ENTITYH
