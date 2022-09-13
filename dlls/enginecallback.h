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
#ifndef ENGINECALLBACK_H
#define ENGINECALLBACK_H
#pragma once

#include "event_flags.h"

// Must be provided by user of this code
extern enginefuncs_t g_engfuncs;

// The actual engine callbacks
#define GETPLAYERUSERID      (*g_engfuncs.pfnGetPlayerUserId)
#define PRECACHE_MODEL3      (*g_engfuncs.pfnPrecacheModel3)
#define PRECACHE_SOUND2      (*g_engfuncs.pfnPrecacheSound2)
#define PRECACHE_GENERIC3    (*g_engfuncs.pfnPrecacheGeneric3)
#define SET_MODEL2           (*g_engfuncs.pfnSetModel2)
#define MODEL_INDEX2         (*g_engfuncs.pfnModelIndex2)
#define MODEL_FRAMES         (*g_engfuncs.pfnModelFrames)
#define SET_SIZE             (*g_engfuncs.pfnSetSize)
//#define CHANGE_LEVEL         (*g_engfuncs.pfnChangeLevel)
void CHANGE_LEVEL(char* s1, char* s2);
#define GET_SPAWN_PARMS      (*g_engfuncs.pfnGetSpawnParms)
#define SAVE_SPAWN_PARMS     (*g_engfuncs.pfnSaveSpawnParms)
#define VEC_TO_YAW           (*g_engfuncs.pfnVecToYaw)
#define VEC_TO_ANGLES        (*g_engfuncs.pfnVecToAngles)
#define MOVE_TO_ORIGIN       (*g_engfuncs.pfnMoveToOrigin)
#define oldCHANGE_YAW        (*g_engfuncs.pfnChangeYaw)
#define CHANGE_PITCH         (*g_engfuncs.pfnChangePitch)
#define MAKE_VECTORS         (*g_engfuncs.pfnMakeVectors)
#define CREATE_ENTITY        (*g_engfuncs.pfnCreateEntity)
#define REMOVE_ENTITY        (*g_engfuncs.pfnRemoveEntity)
#define CREATE_NAMED_ENTITY  (*g_engfuncs.pfnCreateNamedEntity)
#define MAKE_STATIC          (*g_engfuncs.pfnMakeStatic)
#define ENT_IS_ON_FLOOR      (*g_engfuncs.pfnEntIsOnFloor)
#define DROP_TO_FLOOR        (*g_engfuncs.pfnDropToFloor)
#define WALK_MOVE            (*g_engfuncs.pfnWalkMove)
#define SET_ORIGIN           (*g_engfuncs.pfnSetOrigin)
#define EMIT_SOUND_DYN2      (*g_engfuncs.pfnEmitSound)
#define BUILD_SOUND_MSG      (*g_engfuncs.pfnBuildSoundMsg)
#define TRACE_LINE           (*g_engfuncs.pfnTraceLine)
#define TRACE_TOSS           (*g_engfuncs.pfnTraceToss)
#define TRACE_MONSTER_HULL   (*g_engfuncs.pfnTraceMonsterHull)
#define TRACE_HULL           (*g_engfuncs.pfnTraceHull)
#define GET_AIM_VECTOR       (*g_engfuncs.pfnGetAimVector)
#define SERVER_COMMAND       (*g_engfuncs.pfnServerCommand)
#define SERVER_EXECUTE       (*g_engfuncs.pfnServerExecute)
#define CLIENT_COMMAND       (*g_engfuncs.pfnClientCommand)
#define PARTICLE_EFFECT      (*g_engfuncs.pfnParticleEffect)
#define LIGHT_STYLE          (*g_engfuncs.pfnLightStyle)
#define DECAL_INDEX          (*g_engfuncs.pfnDecalIndex)
#define POINT_CONTENTS       (*g_engfuncs.pfnPointContents)
#define CRC32_INIT           (*g_engfuncs.pfnCRC32_Init)
#define CRC32_PROCESS_BUFFER (*g_engfuncs.pfnCRC32_ProcessBuffer)
#define CRC32_PROCESS_BYTE   (*g_engfuncs.pfnCRC32_ProcessByte)
#define CRC32_FINAL          (*g_engfuncs.pfnCRC32_Final)
#define RANDOM_LONG          (*g_engfuncs.pfnRandomLong)
#define RANDOM_FLOAT         (*g_engfuncs.pfnRandomFloat)
#define GETPLAYERAUTHID      (*g_engfuncs.pfnGetPlayerAuthId)

namespace
{
	bool _isBogusMessage = false;
}

inline void MESSAGE_BEGIN_IMPL(const char* msg_args, int msg_dest, int msg_type, const float* pOrigin = nullptr, edict_t* ed = nullptr)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Last message was bogus and not finished before new message!\n");
		_isBogusMessage = false;
	}

	// Very very very rarely for some reason the game might crash with
	// "Tried to create a message with a bogus message type ( 0 )".
	// We catch that here and log it instead of crashing.
	if (msg_type == 0)
	{
		_isBogusMessage = true;
		g_engfuncs.pfnAlertMessage(at_error, "Tried to send a bogus message, intercepted: %s\n", msg_args);
		if (pOrigin)
		{
			g_engfuncs.pfnAlertMessage(at_error, "Bogus message pOrigin: %f %f %f\n", pOrigin[0], pOrigin[1], pOrigin[2]);
		}
		if (ed)
		{
			g_engfuncs.pfnAlertMessage(at_error, "Bogus message ed\n");
		}
		return;
	}

	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}

inline void MESSAGE_BEGIN_IMPL(const char* msg_args, int msg_dest, int msg_type, const float* pOrigin, entvars_t* pev)
{
	MESSAGE_BEGIN_IMPL(msg_args, msg_dest, msg_type, pOrigin, pev->pContainingEntity);
}

#define MESSAGE_BEGIN(...) MESSAGE_BEGIN_IMPL(#__VA_ARGS__, __VA_ARGS__)

inline void MESSAGE_END()
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message end.\n");
		_isBogusMessage = false;
		return;
	}
	(*g_engfuncs.pfnMessageEnd)();
}
inline void WRITE_BYTE(int iValue)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message byte: %i\n", iValue);
		return;
	}
	(*g_engfuncs.pfnWriteByte)(iValue);
}
inline void WRITE_CHAR(int iValue)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message char: %i\n", iValue);
		return;
	}
	(*g_engfuncs.pfnWriteChar)(iValue);
}
inline void WRITE_SHORT(int iValue)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message short: %i\n", iValue);
		return;
	}
	(*g_engfuncs.pfnWriteShort)(iValue);
}
inline void WRITE_LONG(int iValue)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message long: %i\n", iValue);
		return;
	}
	(*g_engfuncs.pfnWriteLong)(iValue);
}
inline void WRITE_ANGLE(float flValue)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message angle: %f\n", flValue);
		return;
	}
	(*g_engfuncs.pfnWriteAngle)(flValue);
}
inline void WRITE_COORD(float flValue)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message coord: %f\n", flValue);
		return;
	}
	(*g_engfuncs.pfnWriteCoord)(flValue);
}
inline void WRITE_STRING(const char* sz)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message string: %s\n", sz);
		return;
	}
	(*g_engfuncs.pfnWriteString)(sz);
}
inline void WRITE_ENTITY(int iValue)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message entity: %i\n", iValue);
		return;
	}
	(*g_engfuncs.pfnWriteEntity)(iValue);
}
inline void WRITE_COORDS(const float* v)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message coords: %f %f %f\n", v[0], v[1], v[2]);
		return;
	}
	WRITE_COORD(v[0]);
	WRITE_COORD(v[1]);
	WRITE_COORD(v[2]);
}
inline void WRITE_ANGLES(const float* v)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message angles: %f %f %f\n", v[0], v[1], v[2]);
		return;
	}
	WRITE_ANGLE(v[0]);
	WRITE_ANGLE(v[1]);
	WRITE_ANGLE(v[2]);
}
inline void WRITE_FLOAT(float f)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message float: %f\n", f);
		return;
	}
	unsigned char floatbytes[sizeof(float)];
	memcpy(floatbytes, &f, sizeof(float));
	for (size_t i = 0; i < sizeof(float); i++)
	{
		WRITE_BYTE(floatbytes[i]);
	}
}
inline void WRITE_PRECISE_VECTOR(const float* v)
{
	if (_isBogusMessage)
	{
		g_engfuncs.pfnAlertMessage(at_error, "Bogus message precise vector: %f %f %f\n", v[0], v[1], v[2]);
		return;
	}
	WRITE_FLOAT(v[0]);
	WRITE_FLOAT(v[1]);
	WRITE_FLOAT(v[2]);
}
#define CVAR_REGISTER    (*g_engfuncs.pfnCVarRegister)
//#define CVAR_GET_FLOAT   (*g_engfuncs.pfnCVarGetFloat)
//#define CVAR_GET_STRING  (*g_engfuncs.pfnCVarGetString)
#define CVAR_SET_FLOAT   (*g_engfuncs.pfnCVarSetFloat)
#define CVAR_SET_STRING  (*g_engfuncs.pfnCVarSetString)
#define CVAR_GET_POINTER (*g_engfuncs.pfnCVarGetPointer)
#define ALERT            (*g_engfuncs.pfnAlertMessage)
#define ENGINE_FPRINTF   (*g_engfuncs.pfnEngineFprintf)
#define ALLOC_PRIVATE    (*g_engfuncs.pfnPvAllocEntPrivateData)
inline void* GET_PRIVATE(const edict_t* pent)
{
	if (pent)
		return pent->pvPrivateData;
	return nullptr;
}

#define FREE_PRIVATE (*g_engfuncs.pfnFreeEntPrivateData)
//#define STRING					(*g_engfuncs.pfnSzFromIndex)
#define ALLOC_STRING          (*g_engfuncs.pfnAllocString)
#define FIND_ENTITY_BY_STRING (*g_engfuncs.pfnFindEntityByString)
#define GETENTITYILLUM        (*g_engfuncs.pfnGetEntityIllum)
#define FIND_ENTITY_IN_SPHERE (*g_engfuncs.pfnFindEntityInSphere)
#define FIND_CLIENT_IN_PVS    (*g_engfuncs.pfnFindClientInPVS)
#define EMIT_AMBIENT_SOUND2   (*g_engfuncs.pfnEmitAmbientSound)
//#define GET_MODEL_PTR         (*g_engfuncs.pfnGetModelPtr)
#define REG_USER_MSG          (*g_engfuncs.pfnRegUserMsg)
#define GET_BONE_POSITION     (*g_engfuncs.pfnGetBonePosition)
#define FUNCTION_FROM_NAME    (*g_engfuncs.pfnFunctionFromName)
#define NAME_FOR_FUNCTION     (*g_engfuncs.pfnNameForFunction)
#define TRACE_TEXTURE         (*g_engfuncs.pfnTraceTexture)
#define CLIENT_PRINTF         (*g_engfuncs.pfnClientPrintf)
#define CMD_ARGS              (*g_engfuncs.pfnCmd_Args)
#define CMD_ARGC              (*g_engfuncs.pfnCmd_Argc)
#define CMD_ARGV              (*g_engfuncs.pfnCmd_Argv)
#define GET_ATTACHMENT        (*g_engfuncs.pfnGetAttachment)
#define SET_VIEW              (*g_engfuncs.pfnSetView)
#define SET_CROSSHAIRANGLE    (*g_engfuncs.pfnCrosshairAngle)
#define LOAD_FILE_FOR_ME      (*g_engfuncs.pfnLoadFileForMe)
#define FREE_FILE             (*g_engfuncs.pfnFreeFile)
#define COMPARE_FILE_TIME     (*g_engfuncs.pfnCompareFileTime)
#define GET_GAME_DIR          (*g_engfuncs.pfnGetGameDir)
#define IS_MAP_VALID          (*g_engfuncs.pfnIsMapValid)
#define NUMBER_OF_ENTITIES    (*g_engfuncs.pfnNumberOfEntities)
#define IS_DEDICATED_SERVER   (*g_engfuncs.pfnIsDedicatedServer)

#define PRECACHE_EVENT      (*g_engfuncs.pfnPrecacheEvent)
#define PLAYBACK_EVENT_FULL (*g_engfuncs.pfnPlaybackEvent)

#define ENGINE_SET_PVS (*g_engfuncs.pfnSetFatPVS)
#define ENGINE_SET_PAS (*g_engfuncs.pfnSetFatPAS)

#define ENGINE_CHECK_VISIBILITY (*g_engfuncs.pfnCheckVisibility)

#define DELTA_SET             (*g_engfuncs.pfnDeltaSetField)
#define DELTA_UNSET           (*g_engfuncs.pfnDeltaUnsetField)
#define DELTA_ADDENCODER      (*g_engfuncs.pfnDeltaAddEncoder)
#define ENGINE_CURRENT_PLAYER (*g_engfuncs.pfnGetCurrentPlayer)

#define ENGINE_CANSKIP (*g_engfuncs.pfnCanSkipPlayer)

#define DELTA_FINDFIELD    (*g_engfuncs.pfnDeltaFindField)
#define DELTA_SETBYINDEX   (*g_engfuncs.pfnDeltaSetFieldByIndex)
#define DELTA_UNSETBYINDEX (*g_engfuncs.pfnDeltaUnsetFieldByIndex)

#define ENGINE_GETPHYSINFO (*g_engfuncs.pfnGetPhysicsInfoString)

#define ENGINE_SETGROUPMASK (*g_engfuncs.pfnSetGroupMask)

#define ENGINE_INSTANCE_BASELINE (*g_engfuncs.pfnCreateInstancedBaseline)

#define ENGINE_FORCE_UNMODIFIED (*g_engfuncs.pfnForceUnmodified)

#define PLAYER_CNX_STATS (*g_engfuncs.pfnGetPlayerStats)

#endif  //ENGINECALLBACK_H
