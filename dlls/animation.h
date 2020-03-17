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
#ifndef ANIMATION_H
#define ANIMATION_H

#define ACTIVITY_NOT_AVAILABLE -1

#ifndef MONSTEREVENT_H
#include "monsterevent.h"
#endif

extern int IsSoundEvent(int eventNumber);

int LookupActivity(void* pmodel, entvars_t* pev, int activity);
int LookupActivityHeaviest(void* pmodel, entvars_t* pev, int activity);
int LookupSequence(void* pmodel, const char* label);
int GetNumSequences(void* pmodel);
int GetSequenceInfo(void* pmodel, entvars_t* pev, float* pflFrameRate, float* pflGroundSpeed, float* pflFPS = nullptr, int* piNumFrames = nullptr, bool* pfIsLooping = nullptr);
int GetSequenceInfo(void* pmodel, int sequence, float* pflFrameRate, float* pflGroundSpeed, float* pflFPS = nullptr, int* piNumFrames = nullptr, bool* pfIsLooping = nullptr);
int GetSequenceFlags(void* pmodel, entvars_t* pev);
int LookupAnimationEvents(void* pmodel, entvars_t* pev, float flStart, float flEnd);
float SetController(void* pmodel, entvars_t* pev, int iController, float flValue);
float SetBlending(void* pmodel, entvars_t* pev, int iBlender, float flValue);
void GetEyePosition(void* pmodel, float* vecEyePosition);
void SequencePrecache(void* pmodel, const char* pSequenceName);
int FindTransition(void* pmodel, int iEndingAnim, int iGoalAnim, int* piDir);
void SetBodygroup(void* pmodel, entvars_t* pev, int iGroup, int iValue);
int GetBodygroup(void* pmodel, entvars_t* pev, int iGroup);

int GetAnimationEvent(void* pmodel, entvars_t* pev, MonsterEvent_t* pMonsterEvent, float flStart, float flEnd, int index, ClientAnimEvent_t* pClientAnimEvent = nullptr);
int ExtractBbox(void* pmodel, int sequence, float* mins, float* maxs);


struct StudioHitBox
{
	vec3_t abscenter;
	vec3_t origin;
	vec3_t angles;
	vec3_t mins;
	vec3_t maxs;
	int hitgroup = 0;
};
struct StudioAttachment
{
	vec3_t pos;
};
int GetNumHitboxes(void* pmodel);
int GetNumBodies(void* pmodel);
int GetNumAttachments(void* pmodel);
int GetHitboxesAndAttachments(entvars_t* pev, void* pmodel, int sequence, float frame, StudioHitBox* hitboxes, StudioAttachment* attachments, bool mirrored = false);

// From /engine/studio.h
#define STUDIO_LOOPING 0x0001


#endif  //ANIMATION_H
