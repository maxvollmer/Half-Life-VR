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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/nowin.h"

typedef int BOOL;
#define TRUE  1
#define FALSE 0

// hack into header files that we can ship
typedef int qboolean;
typedef unsigned char byte;
#include "../utils/common/mathlib.h"
#include "const.h"
#include "progdefs.h"
#include "edict.h"
#include "eiface.h"

#include "studio.h"

#ifndef ACTIVITY_H
#include "activity.h"
#endif

#include "activitymap.h"

#ifndef ANIMATION_H
#include "animation.h"
#endif

#ifndef SCRIPTEVENT_H
#include "scriptevent.h"
#endif

#ifndef ENGINECALLBACK_H
#include "enginecallback.h"
#endif

#include "com_model.h"

extern globalvars_t* gpGlobals;

#pragma warning(disable : 4244)

constexpr const float min(float a, float b)
{
	return (a < b) ? a : b;
}

constexpr const int VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE = 1024;

int ExtractBbox(void* pmodel, int sequence, float* mins, float* maxs)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	if (sequence < 0 || sequence >= pstudiohdr->numseq)
		return 0;

	if (pstudiohdr->numseq <= 0)
		return 0;

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex);

	VectorCopy(pseqdesc[sequence].bbmin, mins);
	VectorCopy(pseqdesc[sequence].bbmax, maxs);

	// Some of our weapons hide parts 9999 units off the model origin (e.g. the RPG rocket when firing)
	// The bounding boxes then are invalid, which we fix by simply taking the bounding box for the idle animation (index 0)
	vec3_t size;
	VectorSubtract(maxs, mins, size);
	if (VectorLength(size) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE || VectorLength(mins) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE || VectorLength(maxs) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE)
	{
		// idle bounding box is too big, print warning
		ALERT(at_console, "NOTICE: bounding box for model %s with animation %s is too big, trying to find smaller bbox!\n", pstudiohdr->name, pseqdesc[sequence].label);

		for (int i = 0; i < pstudiohdr->numseq; i++)
		{
			vec3_t cursize;
			VectorSubtract(pseqdesc[i].bbmax, pseqdesc[i].bbmin, cursize);
			if (VectorLength(cursize) < VectorLength(size))
			{
				VectorCopy(pseqdesc[i].bbmin, mins);
				VectorCopy(pseqdesc[i].bbmax, maxs);
				VectorCopy(cursize, size);
			}
		}
	}

	VectorSubtract(maxs, mins, size);
	if (VectorLength(size) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE || VectorLength(mins) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE || VectorLength(maxs) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE)
	{
		// idle bounding box is too big, print warning
		ALERT(at_console, "WARNING: smallest bounding box for model %s is too big, trying to create a fitting bbox!\n", pstudiohdr->name);

		vec3_t smallestbounds;
		smallestbounds[0] = 9999.f;
		smallestbounds[1] = 9999.f;
		smallestbounds[2] = 9999.f;
		for (int i = 0; i < pstudiohdr->numseq; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				smallestbounds[j] = min(smallestbounds[j], fabs(pseqdesc[i].bbmin[j]));
				smallestbounds[j] = min(smallestbounds[j], fabs(pseqdesc[i].bbmax[j]));
				mins[j] = -smallestbounds[j];
				maxs[j] = smallestbounds[j];
			}
		}
	}

	VectorSubtract(maxs, mins, size);
	if (VectorLength(size) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE || VectorLength(mins) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE || VectorLength(maxs) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE)
	{
		// idle bounding box is too big, print warning
		ALERT(at_console, "WARNING: created bounding box for model %s is still too big, falling back to 16x16x16!\n", pstudiohdr->name, pseqdesc[sequence].label);

		mins[0] = -8.f;
		mins[1] = -8.f;
		mins[2] = -8.f;
		maxs[0] = 8.f;
		maxs[1] = 8.f;
		maxs[2] = 8.f;
	}

	return 1;
}


int LookupActivity(void* pmodel, entvars_t* pev, int activity)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex);

	int weighttotal = 0;
	int seq = ACTIVITY_NOT_AVAILABLE;
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			weighttotal += pseqdesc[i].actweight;
			if (!weighttotal || RANDOM_LONG(0, weighttotal - 1) < pseqdesc[i].actweight)
				seq = i;
		}
	}

	return seq;
}

int LookupActivityHeaviest(void* pmodel, entvars_t* pev, int activity)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex);

	int weight = 0;
	int seq = ACTIVITY_NOT_AVAILABLE;
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			if (pseqdesc[i].actweight > weight)
			{
				weight = pseqdesc[i].actweight;
				seq = i;
			}
		}
	}

	return seq;
}

void GetEyePosition(void* pmodel, float* vecEyePosition)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);

	if (!pstudiohdr)
	{
		ALERT(at_console, "GetEyePosition() Can't get pstudiohdr ptr!\n");
		return;
	}

	VectorCopy(pstudiohdr->eyeposition, vecEyePosition);
}

int LookupSequence(void* pmodel, const char* label)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex);

	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (_stricmp(pseqdesc[i].label, label) == 0)
			return i;
	}

	return -1;
}


int IsSoundEvent(int eventNumber)
{
	if (eventNumber == SCRIPT_EVENT_SOUND || eventNumber == SCRIPT_EVENT_SOUND_VOICE)
		return 1;
	return 0;
}


void SequencePrecache(void* pmodel, const char* pSequenceName)
{
	int index = LookupSequence(pmodel, pSequenceName);
	if (index >= 0)
	{
		studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
		if (!pstudiohdr || index >= pstudiohdr->numseq)
			return;

		mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex) + index;
		mstudioevent_t* pevent = reinterpret_cast<mstudioevent_t*>(reinterpret_cast<byte*>(pstudiohdr) + pseqdesc->eventindex);

		for (int i = 0; i < pseqdesc->numevents; i++)
		{
			// Don't send client-side events to the server AI
			if (pevent[i].event >= EVENT_CLIENT)
				continue;

			// UNDONE: Add a callback to check to see if a sound is precached yet and don't allocate a copy
			// of it's name if it is.
			if (IsSoundEvent(pevent[i].event))
			{
				if (!strlen(pevent[i].options))
				{
					ALERT(at_error, "Bad sound event %d in sequence %s :: %s (sound is \"%s\")\n", pevent[i].event, pstudiohdr->name, pSequenceName, pevent[i].options);
				}

				extern int PRECACHE_SOUND(const char*);
				PRECACHE_SOUND(gpGlobals->pStringBase + ALLOC_STRING(pevent[i].options));
			}
		}
	}
}

int GetNumSequences(void* pmodel)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;
	return pstudiohdr->numseq;
}

int GetSequenceInfo(void* pmodel, entvars_t* pev, float* pflFrameRate, float* pflGroundSpeed, float* pflFPS, int* piNumFrames, bool* pfIsLooping)
{
	return GetSequenceInfo(pmodel, pev->sequence, pflFrameRate, pflGroundSpeed, pflFPS, piNumFrames, pfIsLooping);
}

int GetSequenceInfo(void* pmodel, int sequence, float* pflFrameRate, float* pflGroundSpeed, float* pflFPS, int* piNumFrames, bool* pfIsLooping)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	if (sequence >= pstudiohdr->numseq)
	{
		*pflFrameRate = 0.0;
		*pflGroundSpeed = 0.0;
		return 0;
	}

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex) + sequence;

	if (pflFPS)
		*pflFPS = pseqdesc->fps;
	if (piNumFrames)
		*piNumFrames = pseqdesc->numframes;
	if (pfIsLooping)
		*pfIsLooping = pseqdesc->flags & STUDIO_LOOPING;

	if (pseqdesc->numframes > 1)
	{
		*pflFrameRate = 256 * pseqdesc->fps / (pseqdesc->numframes - 1);
		*pflGroundSpeed = sqrtf(pseqdesc->linearmovement[0] * pseqdesc->linearmovement[0] + pseqdesc->linearmovement[1] * pseqdesc->linearmovement[1] + pseqdesc->linearmovement[2] * pseqdesc->linearmovement[2]);
		*pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
	}
	else
	{
		*pflFrameRate = 256.0;
		*pflGroundSpeed = 0.0;
	}

	return 1;
}


int GetSequenceFlags(void* pmodel, entvars_t* pev)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr || pev->sequence >= pstudiohdr->numseq)
		return 0;

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex) + pev->sequence;

	return pseqdesc->flags;
}


int GetAnimationEvent(void* pmodel, entvars_t* pev, MonsterEvent_t* pMonsterEvent, float flStart, float flEnd, int index, ClientAnimEvent_t* pClientAnimEvent)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr || pev->sequence >= pstudiohdr->numseq || !pMonsterEvent)
		return 0;

	int events = 0;

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex) + pev->sequence;
	mstudioevent_t* pevent = reinterpret_cast<mstudioevent_t*>(reinterpret_cast<byte*>(pstudiohdr) + pseqdesc->eventindex);

	if (pseqdesc->numevents == 0 || index > pseqdesc->numevents)
		return 0;

	if (pseqdesc->numframes > 1)
	{
		flStart *= (pseqdesc->numframes - 1) / 256.f;
		flEnd *= (pseqdesc->numframes - 1) / 256.f;
	}
	else
	{
		flStart = 0;
		flEnd = 1.f;
	}

	for (; index < pseqdesc->numevents; index++)
	{
		if ((pevent[index].frame >= flStart && pevent[index].frame < flEnd) ||
			((pseqdesc->flags & STUDIO_LOOPING) && flEnd >= pseqdesc->numframes - 1 && pevent[index].frame < flEnd - pseqdesc->numframes + 1))
		{
			// Don't send client-side events to the server AI
			if (pevent[index].event >= EVENT_CLIENT)
			{
				if (pClientAnimEvent)
				{
					pClientAnimEvent->isSet = true;
					pClientAnimEvent->event = pevent[index].event;
					pClientAnimEvent->options = pevent[index].options;
					return index + 1;
				}
			}
			else
			{
				if (pClientAnimEvent)
					pClientAnimEvent->isSet = false;
				pMonsterEvent->event = pevent[index].event;
				pMonsterEvent->options = pevent[index].options;
				return index + 1;
			}
		}
	}
	return 0;
}

float SetController(void* pmodel, entvars_t* pev, int iController, float flValue)
{
	studiohdr_t* pstudiohdr;

	pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return flValue;

	mstudiobonecontroller_t* pbonecontroller = reinterpret_cast<mstudiobonecontroller_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->bonecontrollerindex);

	// find first controller that matches the index
	int i = 0;
	for (; i < pstudiohdr->numbonecontrollers; i++, pbonecontroller++)
	{
		if (pbonecontroller->index == iController)
			break;
	}
	if (i >= pstudiohdr->numbonecontrollers)
		return flValue;

	// wrap 0..360 if it's a rotational controller

	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.f >= pbonecontroller->end)
		{
			if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.f) + 180.f)
				flValue = flValue - 360.f;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.f) - 180.f)
				flValue = flValue + 360.f;
		}
		else
		{
			if (flValue > 360)
				flValue = flValue - (int)(flValue / 360.f) * 360.f;
			else if (flValue < 0)
				flValue = flValue + (int)((flValue / -360.f) + 1.f) * 360.f;
		}
	}

	int setting = 255 * (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start);

	if (setting < 0)
		setting = 0;
	if (setting > 255)
		setting = 255;
	pev->controller[iController] = setting;

	return setting * (1.f / 255.f) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}


float SetBlending(void* pmodel, entvars_t* pev, int iBlender, float flValue)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return flValue;

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex) + pev->sequence;

	if (pseqdesc->blendtype[iBlender] == 0)
		return flValue;

	if (pseqdesc->blendtype[iBlender] & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pseqdesc->blendend[iBlender] < pseqdesc->blendstart[iBlender])
			flValue = -flValue;

		// does the controller not wrap?
		if (pseqdesc->blendstart[iBlender] + 359.f >= pseqdesc->blendend[iBlender])
		{
			if (flValue > ((pseqdesc->blendstart[iBlender] + pseqdesc->blendend[iBlender]) / 2.f) + 180.f)
				flValue = flValue - 360;
			if (flValue < ((pseqdesc->blendstart[iBlender] + pseqdesc->blendend[iBlender]) / 2.f) - 180.f)
				flValue = flValue + 360;
		}
	}

	int setting = 255 * (flValue - pseqdesc->blendstart[iBlender]) / (pseqdesc->blendend[iBlender] - pseqdesc->blendstart[iBlender]);

	if (setting < 0)
		setting = 0;
	if (setting > 255)
		setting = 255;

	pev->blending[iBlender] = setting;

	return setting * (1.f / 255.f) * (pseqdesc->blendend[iBlender] - pseqdesc->blendstart[iBlender]) + pseqdesc->blendstart[iBlender];
}



int FindTransition(void* pmodel, int iEndingAnim, int iGoalAnim, int* piDir)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return iGoalAnim;

	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex);

	// bail if we're going to or from a node 0
	if (pseqdesc[iEndingAnim].entrynode == 0 || pseqdesc[iGoalAnim].entrynode == 0)
	{
		return iGoalAnim;
	}

	int iEndNode = 0;

	// ALERT( at_console, "from %d to %d: ", pEndNode->iEndNode, pGoalNode->iStartNode );

	if (*piDir > 0)
	{
		iEndNode = pseqdesc[iEndingAnim].exitnode;
	}
	else
	{
		iEndNode = pseqdesc[iEndingAnim].entrynode;
	}

	if (iEndNode == pseqdesc[iGoalAnim].entrynode)
	{
		*piDir = 1;
		return iGoalAnim;
	}

	byte* pTransition = (reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->transitionindex);

	int iInternNode = pTransition[(iEndNode - 1) * pstudiohdr->numtransitions + (pseqdesc[iGoalAnim].entrynode - 1)];

	if (iInternNode == 0)
		return iGoalAnim;

	int i = 0;

	// look for someone going
	for (i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].entrynode == iEndNode && pseqdesc[i].exitnode == iInternNode)
		{
			*piDir = 1;
			return i;
		}
		if (pseqdesc[i].nodeflags)
		{
			if (pseqdesc[i].exitnode == iEndNode && pseqdesc[i].entrynode == iInternNode)
			{
				*piDir = -1;
				return i;
			}
		}
	}

	ALERT(at_console, "error in transition graph");
	return iGoalAnim;
}

void SetBodygroup(void* pmodel, entvars_t* pev, int iGroup, int iValue)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return;

	if (iGroup > pstudiohdr->numbodyparts)
		return;

	mstudiobodyparts_t* pbodypart = reinterpret_cast<mstudiobodyparts_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->bodypartindex) + iGroup;

	if (iValue >= pbodypart->nummodels)
		return;

	int iCurrent = (pev->body / pbodypart->base) % pbodypart->nummodels;

	pev->body = (pev->body - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));
}


int GetBodygroup(void* pmodel, entvars_t* pev, int iGroup)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	if (iGroup > pstudiohdr->numbodyparts)
		return 0;

	mstudiobodyparts_t* pbodypart = reinterpret_cast<mstudiobodyparts_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->bodypartindex) + iGroup;

	if (pbodypart->nummodels <= 1)
		return 0;

	int iCurrent = (pev->body / pbodypart->base) % pbodypart->nummodels;

	return iCurrent;
}


// Blatantly copied from client.dll's StudioSetupBones, modified for our VR weapons - Max Vollmer, 2019-07-13
namespace
{
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef PITCH
	// up / down
#define PITCH 0
#endif
#ifndef YAW
	// left / right
#define YAW 1
#endif
#ifndef ROLL
	// fall over
#define ROLL 2
#endif
	void StudioCalcBoneAdj(studiohdr_t* pstudiohdr, float* adj, const byte* pcontroller)
	{
		int i, j;
		float value = 0.f;
		mstudiobonecontroller_t* pbonecontroller = reinterpret_cast<mstudiobonecontroller_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->bonecontrollerindex);

		for (j = 0; j < pstudiohdr->numbonecontrollers; j++)
		{
			i = pbonecontroller[j].index;
			if (i <= 3)
			{
				// check for 360% wrapping
				if (pbonecontroller[j].type & STUDIO_RLOOP)
				{
					value = pcontroller[i] * (360.0 / 256.0) + pbonecontroller[j].start;
				}
				else
				{
					value = pcontroller[i] / 255.f;
					if (value < 0)
						value = 0;
					if (value > 1.f)
						value = 1.f;
					value = (1.f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
				}
			}
			else
			{
				value = 0.f;
				value = (1.f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
			switch (pbonecontroller[j].type & STUDIO_TYPES)
			{
			case STUDIO_XR:
			case STUDIO_YR:
			case STUDIO_ZR:
				adj[j] = value * (M_PI / 180.0);
				break;
			case STUDIO_X:
			case STUDIO_Y:
			case STUDIO_Z:
				adj[j] = value;
				break;
			}
		}
	}
	int VectorCompare(const float* v1, const float* v2)
	{
		int i = 0;

		for (i = 0; i < 3; i++)
			if (v1[i] != v2[i])
				return 0;

		return 1;
	}
	void StudioCalcBoneQuaterion(int frame, float s, mstudiobone_t* pbone, mstudioanim_t* panim, float* adj, float* q)
	{
		int j, k;
		vec4_t q1, q2;
		vec3_t angle1, angle2;

		for (j = 0; j < 3; j++)
		{
			if (panim->offset[j + 3] == 0)
			{
				angle2[j] = angle1[j] = pbone->value[j + 3];  // default;
			}
			else
			{
				mstudioanimvalue_t* panimvalue = reinterpret_cast<mstudioanimvalue_t*>(reinterpret_cast<byte*>(panim) + panim->offset[j + 3]);
				k = frame;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
				while (panimvalue->num.total <= k)
				{
					k -= panimvalue->num.total;
					panimvalue += panimvalue->num.valid + 1;
					// DEBUG
					if (panimvalue->num.total < panimvalue->num.valid)
						k = 0;
				}
				// Bah, missing blend!
				if (panimvalue->num.valid > k)
				{
					angle1[j] = panimvalue[k + 1].value;

					if (panimvalue->num.valid > k + 1)
					{
						angle2[j] = panimvalue[k + 2].value;
					}
					else
					{
						if (panimvalue->num.total > k + 1)
							angle2[j] = angle1[j];
						else
							angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
					}
				}
				else
				{
					angle1[j] = panimvalue[panimvalue->num.valid].value;
					if (panimvalue->num.total > k + 1)
					{
						angle2[j] = angle1[j];
					}
					else
					{
						angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
					}
				}
				angle1[j] = pbone->value[j + 3] + angle1[j] * pbone->scale[j + 3];
				angle2[j] = pbone->value[j + 3] + angle2[j] * pbone->scale[j + 3];
			}

			if (pbone->bonecontroller[j + 3] != -1)
			{
				angle1[j] += adj[pbone->bonecontroller[j + 3]];
				angle2[j] += adj[pbone->bonecontroller[j + 3]];
			}
		}

		if (!VectorCompare(angle1, angle2))
		{
			AngleQuaternion(angle1, q1);
			AngleQuaternion(angle2, q2);
			QuaternionSlerp(q1, q2, s, q);
		}
		else
		{
			AngleQuaternion(angle1, q);
		}
	}
	void StudioCalcBonePosition(int frame, float s, mstudiobone_t* pbone, mstudioanim_t* panim, float* adj, float* pos)
	{
		int j, k;

		for (j = 0; j < 3; j++)
		{
			pos[j] = pbone->value[j];  // default;
			if (panim->offset[j] != 0)
			{
				mstudioanimvalue_t* panimvalue = reinterpret_cast<mstudioanimvalue_t*>(reinterpret_cast<byte*>(panim) + panim->offset[j]);
				/*
				if (i == 0 && j == 0)
					Con_DPrintf("%d  %d:%d  %f\n", frame, panimvalue->num.valid, panimvalue->num.total, s );
				*/

				k = frame;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
				// find span of values that includes the frame we want
				while (panimvalue->num.total <= k)
				{
					k -= panimvalue->num.total;
					panimvalue += panimvalue->num.valid + 1;
					// DEBUG
					if (panimvalue->num.total < panimvalue->num.valid)
						k = 0;
				}
				// if we're inside the span
				if (panimvalue->num.valid > k)
				{
					// and there's more data in the span
					if (panimvalue->num.valid > k + 1)
					{
						pos[j] += (panimvalue[k + 1].value * (1.f - s) + s * panimvalue[k + 2].value) * pbone->scale[j];
					}
					else
					{
						pos[j] += panimvalue[k + 1].value * pbone->scale[j];
					}
				}
				else
				{
					// are we at the end of the repeating values section and there's another section with data?
					if (panimvalue->num.total <= k + 1)
					{
						pos[j] += (panimvalue[panimvalue->num.valid].value * (1.f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
					}
					else
					{
						pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
					}
				}
			}
			if (pbone->bonecontroller[j] != -1 && adj)
			{
				pos[j] += adj[pbone->bonecontroller[j]];
			}
		}
	}
	void StudioCalcRotations(entvars_t* pev, studiohdr_t* pstudiohdr, float pos[][3], vec4_t* q, mstudioseqdesc_t* pseqdesc, mstudioanim_t* panim, float f)
	{
		int i = 0;
		int frame = 0;

		float s = 0.f;
		float adj[MAXSTUDIOCONTROLLERS];
		float dadt = 0.f;

		if (f > pseqdesc->numframes - 1)
		{
			f = 0;
		}
		else if (f < -0.01f)
		{
			f = -0.01f;
		}

		frame = (int)f;

		dadt = 1.f;
		s = (f - frame);

		// add in programtic controllers
		mstudiobone_t* pbone = reinterpret_cast<mstudiobone_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->boneindex);

		StudioCalcBoneAdj(pstudiohdr, adj, pev->controller);

		for (i = 0; i < pstudiohdr->numbones; i++, pbone++, panim++)
		{
			StudioCalcBoneQuaterion(frame, s, pbone, panim, adj, q[i]);

			StudioCalcBonePosition(frame, s, pbone, panim, adj, pos[i]);
			// if (0 && i == 0)
			//	Con_DPrintf("%d %d %d %d\n", m_pCurrentEntity->curstate.sequence, frame, j, k );
		}

		if (pseqdesc->motiontype & STUDIO_X)
		{
			pos[pseqdesc->motionbone][0] = 0.0;
		}
		if (pseqdesc->motiontype & STUDIO_Y)
		{
			pos[pseqdesc->motionbone][1] = 0.0;
		}
		if (pseqdesc->motiontype & STUDIO_Z)
		{
			pos[pseqdesc->motionbone][2] = 0.0;
		}

		s = 0 * ((1.f - (f - (int)(f))) / (pseqdesc->numframes)) * pev->framerate;

		if (pseqdesc->motiontype & STUDIO_LX)
		{
			pos[pseqdesc->motionbone][0] += s * pseqdesc->linearmovement[0];
		}
		if (pseqdesc->motiontype & STUDIO_LY)
		{
			pos[pseqdesc->motionbone][1] += s * pseqdesc->linearmovement[1];
		}
		if (pseqdesc->motiontype & STUDIO_LZ)
		{
			pos[pseqdesc->motionbone][2] += s * pseqdesc->linearmovement[2];
		}
	}
	void StudioSlerpBones(studiohdr_t* pstudiohdr, vec4_t q1[], float pos1[][3], vec4_t q2[], float pos2[][3], float s)
	{
		int i = 0;
		vec4_t q3;
		float s1 = 0.f;

		if (s < 0)
			s = 0;
		else if (s > 1.0)
			s = 1.0;

		s1 = 1.0 - s;

		for (i = 0; i < pstudiohdr->numbones; i++)
		{
			QuaternionSlerp(q1[i], q2[i], s, q3);
			q1[i][0] = q3[0];
			q1[i][1] = q3[1];
			q1[i][2] = q3[2];
			q1[i][3] = q3[3];
			pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
			pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
			pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
		}
	}
	void AngleMatrix(const float* angles, float(*matrix)[4])
	{
		float angle = 0.f;
		float sr, sp, sy, cr, cp, cy;

		angle = angles[YAW] * (M_PI * 2 / 360);
		sy = sin(angle);
		cy = cos(angle);
		angle = angles[PITCH] * (M_PI * 2 / 360);
		sp = sin(angle);
		cp = cos(angle);
		angle = angles[ROLL] * (M_PI * 2 / 360);
		sr = sin(angle);
		cr = cos(angle);

		// matrix = (YAW * PITCH) * ROLL
		matrix[0][0] = cp * cy;
		matrix[1][0] = cp * sy;
		matrix[2][0] = -sp;
		matrix[0][1] = sr * sp * cy + cr * -sy;
		matrix[1][1] = sr * sp * sy + cr * cy;
		matrix[2][1] = sr * cp;
		matrix[0][2] = cr * sp * cy + -sr * -sy;
		matrix[1][2] = cr * sp * sy + -sr * cy;
		matrix[2][2] = cr * cp;
		matrix[0][3] = 0.0;
		matrix[1][3] = 0.0;
		matrix[2][3] = 0.0;
	}
	void MatrixAngles(const float(*matrix)[4], float* angles)
	{
		float cp = sqrtf(matrix[0][0] * matrix[0][0] + matrix[1][0] * matrix[1][0]);
		if (cp < 0.000001f)
		{
			angles[YAW] = 0;
			angles[PITCH] = atan2(-matrix[2][0], cp) * 180.f / M_PI;
			angles[ROLL] = atan2(-matrix[1][2], matrix[1][1]) * 180.f / M_PI;
		}
		else
		{
			angles[YAW] = atan2(matrix[1][0], matrix[0][0]) * 180.f / M_PI;
			angles[PITCH] = atan2(-matrix[2][0], cp) * 180.f / M_PI;
			angles[ROLL] = atan2(matrix[2][1], matrix[2][2]) * 180.f / M_PI;
		}
	}
	void MatrixCopy(float in[3][4], float out[3][4])
	{
		memcpy(out, in, sizeof(float) * 3 * 4);
	}
	void ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4])
	{
		out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
			in1[0][2] * in2[2][0];
		out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
			in1[0][2] * in2[2][1];
		out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
			in1[0][2] * in2[2][2];
		out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
			in1[0][2] * in2[2][3] + in1[0][3];
		out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
			in1[1][2] * in2[2][0];
		out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
			in1[1][2] * in2[2][1];
		out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
			in1[1][2] * in2[2][2];
		out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
			in1[1][2] * in2[2][3] + in1[1][3];
		out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
			in1[2][2] * in2[2][0];
		out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
			in1[2][2] * in2[2][1];
		out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
			in1[2][2] * in2[2][2];
		out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
			in1[2][2] * in2[2][3] + in1[2][3];
	}
	void StudioSetUpTransform(entvars_t* pev, float(*modeltransform)[3][4], bool mirrored)
	{
		vec3_t angles;
		vec3_t modelpos;

		VectorCopy(pev->origin, modelpos);
		VectorCopy(pev->angles, angles);

		angles[PITCH] = -angles[PITCH];
		AngleMatrix(angles, (*modeltransform));

		// Mirror hand model
		if (mirrored)
		{
			// create mirror matrix
			static float mirrormatrix[3][4] = {
				{ 1.f, 0.f, 0.f, 1.f },
				{ 0.f, -1.f, 0.f, 1.f },
				{ 0.f, 0.f, 1.f, 1.f },
			};

			// copy rotation matrix
			float modeltransform_copy[3][4] = { 0 };
			MatrixCopy(*modeltransform, modeltransform_copy);

			// concat mirror and rotation matrix
			ConcatTransforms(modeltransform_copy, mirrormatrix, (*modeltransform));
		}

		(*modeltransform)[0][3] = modelpos[0];
		(*modeltransform)[1][3] = modelpos[1];
		(*modeltransform)[2][3] = modelpos[2];
	}
	void VectorTransform(const vec3_t in1, float in2[3][4], vec3_t out)
	{
		out[0] = DotProduct(in1, in2[0]) + in2[0][3];
		out[1] = DotProduct(in1, in2[1]) + in2[1][3];
		out[2] = DotProduct(in1, in2[2]) + in2[2][3];
	}
	struct BoneTransform
	{
		vec3_t origin;
		vec3_t angles;
	};
	int GetBoneTransforms(entvars_t* pev, studiohdr_t* pstudiohdr, int sequence, float frame, BoneTransform* bonetransforms, StudioAttachment* attachments, bool mirrored)
	{
		if (pstudiohdr->numbones <= 0)
			return 0;

		if (pstudiohdr->numbones > MAXSTUDIOBONES)
			return 0;

		float modeltransform[3][4];
		StudioSetUpTransform(pev, &modeltransform, mirrored);

		int i = 0;

		static float pos[MAXSTUDIOBONES][3];
		static vec4_t q[MAXSTUDIOBONES];
		float bonematrix[3][4];

		static float pos2[MAXSTUDIOBONES][3];
		static vec4_t q2[MAXSTUDIOBONES];
		static float pos3[MAXSTUDIOBONES][3];
		static vec4_t q3[MAXSTUDIOBONES];
		static float pos4[MAXSTUDIOBONES][3];
		static vec4_t q4[MAXSTUDIOBONES];

		if (sequence >= pstudiohdr->numseq)
		{
			sequence = 0;
			frame = 0.f;
		}

		mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex) + sequence;
		if (pseqdesc->seqgroup != 0)
		{
			sequence = 0;
			frame = 0.f;
			pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqindex);
		}

		if (pseqdesc->seqgroup != 0)
		{
			return 0;
		}

		mstudioseqgroup_t* pseqgroup = reinterpret_cast<mstudioseqgroup_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->seqgroupindex) + pseqdesc->seqgroup;
		mstudioanim_t* panim = reinterpret_cast<mstudioanim_t*>(reinterpret_cast<byte*>(pstudiohdr) + pseqgroup->data + pseqdesc->animindex);

		StudioCalcRotations(pev, pstudiohdr, pos, q, pseqdesc, panim, frame);

		if (pseqdesc->numblends > 1)
		{
			float s = 0.f;

			panim += pstudiohdr->numbones;
			StudioCalcRotations(pev, pstudiohdr, pos2, q2, pseqdesc, panim, frame);

			s = (pev->blending[0]) / 255.f;

			StudioSlerpBones(pstudiohdr, q, pos, q2, pos2, s);

			if (pseqdesc->numblends == 4)
			{
				panim += pstudiohdr->numbones;
				StudioCalcRotations(pev, pstudiohdr, pos3, q3, pseqdesc, panim, frame);

				panim += pstudiohdr->numbones;
				StudioCalcRotations(pev, pstudiohdr, pos4, q4, pseqdesc, panim, frame);

				s = (pev->blending[0]) / 255.f;
				StudioSlerpBones(pstudiohdr, q3, pos3, q4, pos4, s);

				s = (pev->blending[1]) / 255.f;
				StudioSlerpBones(pstudiohdr, q, pos, q3, pos3, s);
			}
		}

		mstudiobone_t* pbones = reinterpret_cast<mstudiobone_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->boneindex);

		if (pev->scale != 0.f)
		{
			for (int j = 0; j < 3; j++)
			{
				for (int k = 0; k < 3; k++)
				{
					modeltransform[j][k] *= pev->scale;
				}
			}
		}

		float bonetransform[MAXSTUDIOBONES][3][4];

		for (i = 0; i < pstudiohdr->numbones; i++)
		{
			QuaternionMatrix(q[i], bonematrix);

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];

			if (pbones[i].parent == -1)
			{
				ConcatTransforms(modeltransform, bonematrix, bonetransform[i]);
			}
			else
			{
				ConcatTransforms(bonetransform[pbones[i].parent], bonematrix, bonetransform[i]);
			}

			bonetransforms[i].origin[0] = bonetransform[i][0][3];
			bonetransforms[i].origin[1] = bonetransform[i][1][3];
			bonetransforms[i].origin[2] = bonetransform[i][2][3];
			MatrixAngles(bonetransform[i], bonetransforms[i].angles);
			if (mirrored)
			{
				bonetransforms[i].angles[ROLL] = -bonetransforms[i].angles[ROLL];
			}
		}

		// calculate attachment points
		if (attachments)
		{
			mstudioattachment_t* pattachment = reinterpret_cast<mstudioattachment_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->attachmentindex);
			for (int i = 0; i < pstudiohdr->numattachments; i++)
			{
				VectorTransform(pattachment[i].org, bonetransform[pattachment[i].bone], attachments[i].pos);
			}
		}

		return 1;
	}

}  // namespace
int GetNumHitboxes(void* pmodel)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	return pstudiohdr->numhitboxes;
}
int GetNumBodies(void* pmodel)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	return pstudiohdr->numbodyparts;
}
int GetNumAttachments(void* pmodel)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	return pstudiohdr->numattachments;
}
int GetHitboxesAndAttachments(entvars_t* pev, void* pmodel, int sequence, float frame, StudioHitBox* hitboxes, StudioAttachment* attachments, bool mirrored)
{
	studiohdr_t* pstudiohdr = static_cast<studiohdr_t*>(pmodel);
	if (!pstudiohdr)
		return 0;

	BoneTransform bonetransforms[MAXSTUDIOBONES];
	if (!GetBoneTransforms(pev, pstudiohdr, sequence, frame, bonetransforms, attachments, mirrored))
		return 0;

	mstudiobbox_t* pbbox = reinterpret_cast<mstudiobbox_t*>(reinterpret_cast<byte*>(pstudiohdr) + pstudiohdr->hitboxindex);

	for (int i = 0; i < pstudiohdr->numhitboxes; i++)
	{
		auto& bbox = pbbox[i];
		auto& hitbox = hitboxes[i];

		if (pev->scale != 0.f)
		{
			VectorScale(bbox.bbmin, pev->scale, hitbox.mins);
			VectorScale(bbox.bbmax, pev->scale, hitbox.maxs);
		}
		else
		{
			VectorCopy(bbox.bbmin, hitbox.mins);
			VectorCopy(bbox.bbmax, hitbox.maxs);
		}

		VectorCopy(bonetransforms[bbox.bone].origin, hitbox.origin);
		VectorCopy(bonetransforms[bbox.bone].angles, hitbox.angles);
		hitbox.hitgroup = bbox.group;

		// Some models have hitboxes with size 0. Fix that here
		vec3_t size;
		VectorSubtract(hitbox.maxs, hitbox.mins, size);
		for (int j = 0; j < 3; j++)
		{
			if (fabs(size[j]) < 0.1f)
			{
				hitbox.mins[j] = -4.f;
				hitbox.maxs[j] = 4.f;
			}
		}

		// Defined in VRPhysicsHelper.cpp
		extern void CalculateHitboxAbsCenter(StudioHitBox& hitbox);
		CalculateHitboxAbsCenter(hitbox);
	}

	return 1;
}
