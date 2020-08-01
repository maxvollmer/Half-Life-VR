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
/*

===== util.cpp ========================================================

  Utility code.  Really not optional after all.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include <time.h>
#include "shake.h"
#include "decals.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "VRPhysicsHelper.h"


// Returns the current game directory as std::string (encapsulates g_engfuncs.pfnGetGameDir)
std::string UTIL_GetGameDir()
{
	char gameDir[1024] = {};
	g_engfuncs.pfnGetGameDir(gameDir);
	return std::string{ gameDir };
}



float UTIL_WeaponTimeBase(void)
{
#if defined(CLIENT_WEAPONS)
	return 0.0;
#else
	return gpGlobals->time;
#endif
}

static unsigned int glSeed = 0;

unsigned int seed_table[256] =
{
	28985, 27138, 26457, 9451, 17764, 10909, 28790, 8716, 6361, 4853, 17798, 21977, 19643, 20662, 10834, 20103, 27067, 28634, 18623, 25849, 8576, 26234, 23887, 18228, 32587, 4836, 3306, 1811, 3035, 24559, 18399, 315, 26766, 907, 24102, 12370, 9674, 2972, 10472, 16492, 22683, 11529, 27968, 30406, 13213, 2319, 23620, 16823, 10013, 23772, 21567, 1251, 19579, 20313, 18241, 30130, 8402, 20807, 27354, 7169, 21211, 17293, 5410, 19223, 10255, 22480, 27388, 9946, 15628, 24389, 17308, 2370, 9530, 31683, 25927, 23567, 11694, 26397, 32602, 15031, 18255, 17582, 1422, 28835, 23607, 12597, 20602, 10138, 5212, 1252, 10074, 23166, 19823, 31667, 5902, 24630, 18948, 14330, 14950, 8939, 23540, 21311, 22428, 22391, 3583, 29004, 30498, 18714, 4278, 2437, 22430, 3439, 28313, 23161, 25396, 13471, 19324, 15287, 2563, 18901, 13103, 16867, 9714, 14322, 15197, 26889, 19372, 26241, 31925, 14640, 11497, 8941, 10056, 6451, 28656, 10737, 13874, 17356, 8281, 25937, 1661, 4850, 7448, 12744, 21826, 5477, 10167, 16705, 26897, 8839, 30947, 27978, 27283, 24685, 32298, 3525, 12398, 28726, 9475, 10208, 617, 13467, 22287, 2376, 6097, 26312, 2974, 9114, 21787, 28010, 4725, 15387, 3274, 10762, 31695, 17320, 18324, 12441, 16801, 27376, 22464, 7500, 5666, 18144, 15314, 31914, 31627, 6495, 5226, 31203, 2331, 4668, 12650, 18275, 351, 7268, 31319, 30119, 7600, 2905, 13826, 11343, 13053, 15583, 30055, 31093, 5067, 761, 9685, 11070, 21369, 27155, 3663, 26542, 20169, 12161, 15411, 30401, 7580, 31784, 8985, 29367, 20989, 14203, 29694, 21167, 10337, 1706, 28578, 887, 3373, 19477, 14382, 675, 7033, 15111, 26138, 12252, 30996, 21409, 25678, 18555, 13256, 23316, 22407, 16727, 991, 9236, 5373, 29402, 6117, 15241, 27715, 19291, 19888, 19847 };

unsigned int U_Random(void)
{
	glSeed *= 69069;
	glSeed += seed_table[glSeed & 0xff];

	return (++glSeed & 0x0fffffff);
}

void U_Srand(unsigned int seed)
{
	glSeed = seed_table[seed & 0xff];
}

/*
=====================
UTIL_SharedRandomLong
=====================
*/
int UTIL_SharedRandomLong(unsigned int seed, int low, int high)
{
	if (low == high)
		return low;

	U_Srand(seed + static_cast<unsigned int>(low) + static_cast<unsigned int>(high));

	unsigned int range = static_cast<unsigned int>(high - low + 1);
	unsigned int rnum = U_Random();

	unsigned int offset = rnum % range;

	return (low + static_cast<int>(offset));
}

/*
=====================
UTIL_SharedRandomFloat
=====================
*/
float UTIL_SharedRandomFloat(unsigned int seed, float low, float high)
{
	if (low == high)
		return low;

	U_Srand(seed + *reinterpret_cast<unsigned int*>(&low) + *reinterpret_cast<unsigned int*>(&high));

	U_Random();
	U_Random();

	float range = high - low;
	unsigned int tensixrand = U_Random() & 65535U;
	float offset = static_cast<float>(tensixrand) / 65536.f;
	return (low + offset * range);
}

void UTIL_ParametricRocket(entvars_t* pev, Vector vecOrigin, Vector vecAngles, edict_t* owner)
{
	pev->startpos = vecOrigin;
	// Trace out line to end pos
	TraceResult tr;
	UTIL_MakeVectors(vecAngles);
	UTIL_TraceLine(pev->startpos, pev->startpos + gpGlobals->v_forward * 8192, ignore_monsters, owner, &tr);
	pev->endpos = tr.vecEndPos;

	// Now compute how long it will take based on current velocity
	Vector vecTravel = pev->endpos - pev->startpos;
	float travelTime = 0.0;
	if (pev->velocity.Length() > 0)
	{
		travelTime = vecTravel.Length() / pev->velocity.Length();
	}
	pev->starttime = gpGlobals->time;
	pev->impacttime = gpGlobals->time + travelTime;
}

int g_groupmask = 0;
int g_groupop = 0;

// Normal overrides
void UTIL_SetGroupTrace(int groupmask, int op)
{
	g_groupmask = groupmask;
	g_groupop = op;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

void UTIL_UnsetGroupTrace(void)
{
	g_groupmask = 0;
	g_groupop = 0;

	ENGINE_SETGROUPMASK(0, 0);
}

// Smart version, it'll clean itself up when it pops off stack
UTIL_GroupTrace::UTIL_GroupTrace(int groupmask, int op)
{
	m_oldgroupmask = g_groupmask;
	m_oldgroupop = g_groupop;

	g_groupmask = groupmask;
	g_groupop = op;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

UTIL_GroupTrace::~UTIL_GroupTrace(void)
{
	g_groupmask = m_oldgroupmask;
	g_groupop = m_oldgroupop;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

TYPEDESCRIPTION gEntvarsDescription[] =
{
	DEFINE_ENTITY_FIELD(classname, FIELD_STRING),
	DEFINE_ENTITY_GLOBAL_FIELD(globalname, FIELD_STRING),

	DEFINE_ENTITY_FIELD(origin, FIELD_POSITION_VECTOR),
	DEFINE_ENTITY_FIELD(oldorigin, FIELD_POSITION_VECTOR),
	DEFINE_ENTITY_FIELD(velocity, FIELD_VECTOR),
	DEFINE_ENTITY_FIELD(basevelocity, FIELD_VECTOR),
	DEFINE_ENTITY_FIELD(movedir, FIELD_VECTOR),

	DEFINE_ENTITY_FIELD(angles, FIELD_VECTOR),
	DEFINE_ENTITY_FIELD(avelocity, FIELD_VECTOR),
	DEFINE_ENTITY_FIELD(punchangle, FIELD_VECTOR),
	DEFINE_ENTITY_FIELD(v_angle, FIELD_VECTOR),
	DEFINE_ENTITY_FIELD(fixangle, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(idealpitch, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(pitch_speed, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(ideal_yaw, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(yaw_speed, FIELD_FLOAT),

	DEFINE_ENTITY_FIELD(modelindex, FIELD_INTEGER),
	DEFINE_ENTITY_GLOBAL_FIELD(model, FIELD_MODELNAME),

	DEFINE_ENTITY_FIELD(viewmodel, FIELD_MODELNAME),
	DEFINE_ENTITY_FIELD(weaponmodel, FIELD_MODELNAME),

	DEFINE_ENTITY_FIELD(absmin, FIELD_POSITION_VECTOR),
	DEFINE_ENTITY_FIELD(absmax, FIELD_POSITION_VECTOR),
	DEFINE_ENTITY_GLOBAL_FIELD(mins, FIELD_VECTOR),
	DEFINE_ENTITY_GLOBAL_FIELD(maxs, FIELD_VECTOR),
	DEFINE_ENTITY_GLOBAL_FIELD(size, FIELD_VECTOR),

	DEFINE_ENTITY_FIELD(ltime, FIELD_TIME),
	DEFINE_ENTITY_FIELD(nextthink, FIELD_TIME),

	DEFINE_ENTITY_FIELD(solid, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(movetype, FIELD_INTEGER),

	DEFINE_ENTITY_FIELD(skin, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(body, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(effects, FIELD_INTEGER),

	DEFINE_ENTITY_FIELD(gravity, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(friction, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(light_level, FIELD_FLOAT),

	DEFINE_ENTITY_FIELD(frame, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(scale, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(sequence, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(animtime, FIELD_TIME),
	DEFINE_ENTITY_FIELD(framerate, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(controller, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(blending, FIELD_INTEGER),

	DEFINE_ENTITY_FIELD(rendermode, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(renderamt, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(rendercolor, FIELD_VECTOR),
	DEFINE_ENTITY_FIELD(renderfx, FIELD_INTEGER),

	DEFINE_ENTITY_FIELD(health, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(frags, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(weapons, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(takedamage, FIELD_FLOAT),

	DEFINE_ENTITY_FIELD(deadflag, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(view_ofs, FIELD_VECTOR),
	DEFINE_ENTITY_FIELD(button, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(impulse, FIELD_INTEGER),

	DEFINE_ENTITY_FIELD(chain, FIELD_EDICT),
	DEFINE_ENTITY_FIELD(dmg_inflictor, FIELD_EDICT),
	DEFINE_ENTITY_FIELD(enemy, FIELD_EDICT),
	DEFINE_ENTITY_FIELD(aiment, FIELD_EDICT),
	DEFINE_ENTITY_FIELD(owner, FIELD_EDICT),
	DEFINE_ENTITY_FIELD(groundentity, FIELD_EDICT),

	DEFINE_ENTITY_FIELD(spawnflags, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(flags, FIELD_FLOAT),

	DEFINE_ENTITY_FIELD(colormap, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(team, FIELD_INTEGER),

	DEFINE_ENTITY_FIELD(max_health, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(teleport_time, FIELD_TIME),
	DEFINE_ENTITY_FIELD(armortype, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(armorvalue, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(waterlevel, FIELD_INTEGER),
	DEFINE_ENTITY_FIELD(watertype, FIELD_INTEGER),

	// Having these fields be local to the individual levels makes it easier to test those levels individually.
	DEFINE_ENTITY_GLOBAL_FIELD(target, FIELD_STRING),
	DEFINE_ENTITY_GLOBAL_FIELD(targetname, FIELD_STRING),
	DEFINE_ENTITY_FIELD(netname, FIELD_STRING),
	DEFINE_ENTITY_FIELD(message, FIELD_STRING),

	DEFINE_ENTITY_FIELD(dmg_take, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(dmg_save, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(dmg, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(dmgtime, FIELD_TIME),

	DEFINE_ENTITY_FIELD(noise, FIELD_SOUNDNAME),
	DEFINE_ENTITY_FIELD(noise1, FIELD_SOUNDNAME),
	DEFINE_ENTITY_FIELD(noise2, FIELD_SOUNDNAME),
	DEFINE_ENTITY_FIELD(noise3, FIELD_SOUNDNAME),
	DEFINE_ENTITY_FIELD(speed, FIELD_FLOAT),
	DEFINE_ENTITY_FIELD(air_finished, FIELD_TIME),
	DEFINE_ENTITY_FIELD(pain_finished, FIELD_TIME),
	DEFINE_ENTITY_FIELD(radsuit_finished, FIELD_TIME),
};

#define ENTVARS_COUNT (sizeof(gEntvarsDescription) / sizeof(gEntvarsDescription[0]))


#ifdef	DEBUG
void
DBG_AssertFunction(
	BOOL		fExpr,
	const char* szExpr,
	const char* szFile,
	int			szLine,
	const char* szMessage)
{
	if (fExpr)
		return;
	char szOut[512];
	if (szMessage != NULL)
		sprintf_s(szOut, "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage);
	else
		sprintf_s(szOut, "ASSERT FAILED:\n %s \n(%s@%d)", szExpr, szFile, szLine);
	//	ALERT(at_console, szOut);
}
#endif	// DEBUG


BOOL UTIL_GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pCurrentWeapon)
{
	return g_pGameRules->GetNextBestWeapon(pPlayer, pCurrentWeapon);
}


float UTIL_AngleMod(float a)
{
	if (a < 0)
	{
		return std::fmod(a, 360.f) + 360.f;
	}
	else
	{
		return std::fmod(a, 360.f);
	}
}

float UTIL_AngleDiff(float destAngle, float srcAngle)
{
	float delta = 0.f;

	delta = destAngle - srcAngle;
	if (destAngle > srcAngle)
	{
		if (delta >= 180)
			delta -= 360;
	}
	else
	{
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}

Vector UTIL_VecToAngles(const Vector& vec)
{
	float rgflVecOut[3];
	VEC_TO_ANGLES(vec, rgflVecOut);
	return Vector(rgflVecOut);
}

Vector& UTIL_AnglesMod(Vector& angles)
{
	angles.x = UTIL_AngleMod(angles.x);
	angles.y = UTIL_AngleMod(angles.y);
	angles.z = UTIL_AngleMod(angles.z);
	return angles;
}

//	float UTIL_MoveToOrigin( edict_t *pent, const Vector vecGoal, float flDist, int iMoveType )
void UTIL_MoveToOrigin(edict_t* pent, const Vector& vecGoal, float flDist, int iMoveType)
{
	float rgfl[3];
	vecGoal.CopyToArray(rgfl);
	//		return MOVE_TO_ORIGIN ( pent, rgfl, flDist, iMoveType );
	MOVE_TO_ORIGIN(pent, rgfl, flDist, iMoveType);
}


int UTIL_EntitiesInBox(CBaseEntity** pList, int listMax, const Vector& mins, const Vector& maxs, int flagMask)
{
	edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(1);
	CBaseEntity* pEntity;
	int count = 0;

	count = 0;

	if (!pEdict)
		return count;

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (pEdict->free)  // Not in use
			continue;

		if (flagMask && !(pEdict->v.flags & flagMask))  // Does it meet the criteria?
			continue;

		if (mins.x > pEdict->v.absmax.x ||
			mins.y > pEdict->v.absmax.y ||
			mins.z > pEdict->v.absmax.z ||
			maxs.x < pEdict->v.absmin.x ||
			maxs.y < pEdict->v.absmin.y ||
			maxs.z < pEdict->v.absmin.z)
			continue;

		pEntity = CBaseEntity::SafeInstance<CBaseEntity>(pEdict);
		if (!pEntity)
			continue;

		pList[count] = pEntity;
		count++;

		if (count >= listMax)
			return count;
	}

	return count;
}


int UTIL_MonstersInSphere(CBaseEntity** pList, int listMax, const Vector& center, float radius)
{
	edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(1);
	CBaseEntity* pEntity;
	int count = 0;
	float distance, delta;

	count = 0;
	float radiusSquared = radius * radius;

	if (!pEdict)
		return count;

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (pEdict->free)  // Not in use
			continue;

		if (!(pEdict->v.flags & (FL_CLIENT | FL_MONSTER)))  // Not a client/monster ?
			continue;

		// Use origin for X & Y since they are centered for all monsters
		// Now X
		delta = center.x - pEdict->v.origin.x;  //(pEdict->v.absmin.x + pEdict->v.absmax.x)*0.5;
		delta *= delta;

		if (delta > radiusSquared)
			continue;
		distance = delta;

		// Now Y
		delta = center.y - pEdict->v.origin.y;  //(pEdict->v.absmin.y + pEdict->v.absmax.y)*0.5;
		delta *= delta;

		distance += delta;
		if (distance > radiusSquared)
			continue;

		// Now Z
		delta = center.z - (pEdict->v.absmin.z + pEdict->v.absmax.z) * 0.5f;
		delta *= delta;

		distance += delta;
		if (distance > radiusSquared)
			continue;

		pEntity = CBaseEntity::SafeInstance<CBaseEntity>(pEdict);
		if (!pEntity)
			continue;

		pList[count] = pEntity;
		count++;

		if (count >= listMax)
			return count;
	}


	return count;
}


CBaseEntity* UTIL_FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius)
{
	edict_t* pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = nullptr;

	pentEntity = FIND_ENTITY_IN_SPHERE(pentEntity, vecCenter, flRadius);

	if (!FNullEnt(pentEntity))
		return CBaseEntity::SafeInstance<CBaseEntity>(pentEntity);
	return nullptr;
}



CBaseEntity* UTIL_FindEntityByString(CBaseEntity* pStartEntity, const char* szKeyword, const char* szValue)
{
	edict_t* pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = nullptr;

	pentEntity = FIND_ENTITY_BY_STRING(pentEntity, szKeyword, szValue);

	if (!FNullEnt(pentEntity))
		return CBaseEntity::SafeInstance<CBaseEntity>(pentEntity);
	return nullptr;
}

CBaseEntity* UTIL_FindEntityByClassname(CBaseEntity* pStartEntity, const char* szName)
{
	return UTIL_FindEntityByString(pStartEntity, "classname", szName);
}

CBaseEntity* UTIL_FindEntityByTargetname(CBaseEntity* pStartEntity, const char* szName)
{
	return UTIL_FindEntityByString(pStartEntity, "targetname", szName);
}


CBaseEntity* UTIL_FindEntityGeneric(const char* szWhatever, Vector& vecSrc, float flRadius)
{
	CBaseEntity* pEntity = nullptr;

	pEntity = UTIL_FindEntityByTargetname(nullptr, szWhatever);
	if (pEntity)
		return pEntity;

	CBaseEntity* pSearch = nullptr;
	float flMaxDist2 = flRadius * flRadius;
	while ((pSearch = UTIL_FindEntityByClassname(pSearch, szWhatever)) != nullptr)
	{
		float flDist2 = (pSearch->pev->origin - vecSrc).Length();
		flDist2 = flDist2 * flDist2;
		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}
	return pEntity;
}


// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns nullptr
// Index is 1 based
CBasePlayer* UTIL_PlayerByIndex(int playerIndex)
{
	CBasePlayer* pPlayer = nullptr;
	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(INDEXENT(playerIndex));
	}
	return pPlayer;
}

void UTIL_MakeVectors(const Vector& vecAngles)
{
	MAKE_VECTORS(vecAngles);
}


void UTIL_MakeAimVectors(const Vector& vecAngles)
{
	float rgflVec[3];
	vecAngles.CopyToArray(rgflVec);
	rgflVec[0] = -rgflVec[0];
	MAKE_VECTORS(rgflVec);
}


#define SWAP(a, b, temp) ((temp) = (a), (a) = (b), (b) = (temp))

void UTIL_MakeInvVectors(const Vector& vec, globalvars_t* pgv)
{
	MAKE_VECTORS(vec);

	float tmp = 0.f;
	pgv->v_right = pgv->v_right * -1;

	SWAP(pgv->v_forward.y, pgv->v_right.x, tmp);
	SWAP(pgv->v_forward.z, pgv->v_up.x, tmp);
	SWAP(pgv->v_right.z, pgv->v_up.y, tmp);
}


void UTIL_EmitAmbientSound(edict_t* entity, const Vector& vecOrigin, const char* samp, float vol, float attenuation, int fFlags, int pitch)
{
	float rgfl[3];
	vecOrigin.CopyToArray(rgfl);

	EMIT_AMBIENT_SOUND(entity, rgfl, samp, vol, attenuation, fFlags, pitch);
}

static unsigned short FixedUnsigned16(float value, float scale)
{
	int output = 0;

	output = value * scale;
	if (output < 0)
		output = 0;
	if (output > 0xFFFF)
		output = 0xFFFF;

	return (unsigned short)output;
}

static short FixedSigned16(float value, float scale)
{
	int output = 0;

	output = value * scale;

	if (output > 32767)
		output = 32767;

	if (output < -32768)
		output = -32768;

	return (short)output;
}

// Shake the screen of all clients within radius
// radius == 0, shake all clients
// UNDONE: Allow caller to shake clients not ONGROUND?
// UNDONE: Fix falloff model (disabled)?
// UNDONE: Affect user controls?
void UTIL_ScreenShake(const Vector& center, float amplitude, float frequency, float duration, float radius)
{
	int i = 0;
	float localAmplitude = 0.f;
	ScreenShake shake;

	shake.duration = FixedUnsigned16(duration, 1 << 12);  // 4.12 fixed
	shake.frequency = FixedUnsigned16(frequency, 1 << 8);  // 8.8 fixed

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		if (!pPlayer || !(pPlayer->pev->flags & FL_ONGROUND))  // Don't shake if not onground
			continue;

		localAmplitude = 0;

		if (radius <= 0)
			localAmplitude = amplitude;
		else
		{
			Vector delta = center - pPlayer->pev->origin;
			float distance = delta.Length();

			// Had to get rid of this falloff - it didn't work well
			if (distance < radius)
				localAmplitude = amplitude;  //radius - distance;
		}
		if (localAmplitude)
		{
			shake.amplitude = FixedUnsigned16(localAmplitude, 1 << 12);  // 4.12 fixed

			MESSAGE_BEGIN(MSG_ONE, gmsgShake, nullptr, pPlayer->edict());  // use the magic #1 for "one client"
			WRITE_SHORT(shake.amplitude);  // shake amount
			WRITE_SHORT(shake.duration);   // shake lasts this long
			WRITE_SHORT(shake.frequency);  // shake noise frequency
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_ONE, gmsgVRScreenShake, nullptr, pPlayer->edict());
			WRITE_FLOAT(localAmplitude / 25.f);
			WRITE_FLOAT(duration);
			WRITE_FLOAT(100.f / frequency);
			MESSAGE_END();
		}
	}
}



void UTIL_ScreenShakeAll(const Vector& center, float amplitude, float frequency, float duration)
{
	UTIL_ScreenShake(center, amplitude, frequency, duration, 0);
}


void UTIL_ScreenFadeBuild(ScreenFade& fade, const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	fade.duration = FixedUnsigned16(fadeTime, 1 << 12);  // 4.12 fixed
	fade.holdTime = FixedUnsigned16(fadeHold, 1 << 12);  // 4.12 fixed
	fade.r = (int)color.x;
	fade.g = (int)color.y;
	fade.b = (int)color.z;
	fade.a = alpha;
	fade.fadeFlags = flags;
}


void UTIL_ScreenFadeWrite(const ScreenFade& fade, CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgFade, nullptr, pEntity->edict());  // use the magic #1 for "one client"

	WRITE_SHORT(fade.duration);   // fade lasts this long
	WRITE_SHORT(fade.holdTime);   // fade lasts this long
	WRITE_SHORT(fade.fadeFlags);  // fade type (in / out)
	WRITE_BYTE(fade.r);           // fade red
	WRITE_BYTE(fade.g);           // fade green
	WRITE_BYTE(fade.b);           // fade blue
	WRITE_BYTE(fade.a);           // fade blue

	MESSAGE_END();
}


void UTIL_ScreenFadeAll(const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	int i = 0;
	ScreenFade fade;


	UTIL_ScreenFadeBuild(fade, color, fadeTime, fadeHold, alpha, flags);

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		UTIL_ScreenFadeWrite(fade, pPlayer);
	}
}


void UTIL_ScreenFade(CBaseEntity* pEntity, const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	ScreenFade fade;

	UTIL_ScreenFadeBuild(fade, color, fadeTime, fadeHold, alpha, flags);
	UTIL_ScreenFadeWrite(fade, pEntity);
}


void UTIL_HudMessage(CBaseEntity* pEntity, const hudtextparms_t& textparms, const char* pMessage)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, pEntity->edict());
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(textparms.channel & 0xFF);

	WRITE_SHORT(FixedSigned16(textparms.x, 1 << 13));
	WRITE_SHORT(FixedSigned16(textparms.y, 1 << 13));
	WRITE_BYTE(textparms.effect);

	WRITE_BYTE(textparms.r1);
	WRITE_BYTE(textparms.g1);
	WRITE_BYTE(textparms.b1);
	WRITE_BYTE(textparms.a1);

	WRITE_BYTE(textparms.r2);
	WRITE_BYTE(textparms.g2);
	WRITE_BYTE(textparms.b2);
	WRITE_BYTE(textparms.a2);

	WRITE_SHORT(FixedUnsigned16(textparms.fadeinTime, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(textparms.fadeoutTime, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(textparms.holdTime, 1 << 8));

	if (textparms.effect == 2)
		WRITE_SHORT(FixedUnsigned16(textparms.fxTime, 1 << 8));

	if (strlen(pMessage) < 512)
	{
		WRITE_STRING(pMessage);
	}
	else
	{
		char tmp[512];
		strncpy_s(tmp, pMessage, 511);
		tmp[511] = 0;
		WRITE_STRING(tmp);
	}
	MESSAGE_END();
}

void UTIL_HudMessageAll(const hudtextparms_t& textparms, const char* pMessage)
{
	int i = 0;

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer)
			UTIL_HudMessage(pPlayer, textparms, pMessage);
	}
}


extern int gmsgTextMsg, gmsgSayText;
void UTIL_ClientPrintAll(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgTextMsg);
	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);

	if (param1)
		WRITE_STRING(param1);
	if (param2)
		WRITE_STRING(param2);
	if (param3)
		WRITE_STRING(param3);
	if (param4)
		WRITE_STRING(param4);

	MESSAGE_END();
}

void ClientPrint(entvars_t* client, int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, nullptr, client);
	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);

	if (param1)
		WRITE_STRING(param1);
	if (param2)
		WRITE_STRING(param2);
	if (param3)
		WRITE_STRING(param3);
	if (param4)
		WRITE_STRING(param4);

	MESSAGE_END();
}

void UTIL_SayText(const char* pText, CBaseEntity* pEntity)
{
	if (!pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, pEntity->edict());
	WRITE_BYTE(pEntity->entindex());
	WRITE_STRING(pText);
	MESSAGE_END();
}

void UTIL_SayTextAll(const char* pText, CBaseEntity* pEntity)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgSayText, nullptr);
	WRITE_BYTE(pEntity->entindex());
	WRITE_STRING(pText);
	MESSAGE_END();
}


char* UTIL_dtos1(int d)
{
	static char buf[8];
	sprintf_s(buf, "%d", d);
	return buf;
}

char* UTIL_dtos2(int d)
{
	static char buf[8];
	sprintf_s(buf, "%d", d);
	return buf;
}

char* UTIL_dtos3(int d)
{
	static char buf[8];
	sprintf_s(buf, "%d", d);
	return buf;
}

char* UTIL_dtos4(int d)
{
	static char buf[8];
	sprintf_s(buf, "%d", d);
	return buf;
}

void UTIL_ShowMessage(const char* pString, CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgHudText, nullptr, pEntity->edict());
	WRITE_STRING(pString);
	MESSAGE_END();
}


void UTIL_ShowMessageAll(const char* pString)
{
	int i = 0;

	// loop through all players

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer)
			UTIL_ShowMessage(pString, pPlayer);
	}
}

// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t* pentIgnore, TraceResult* ptr)
{
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass ? 0x100 : 0), pentIgnore, ptr);
}


void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, edict_t* pentIgnore, TraceResult* ptr)
{
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr);
}


void UTIL_TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t* pentIgnore, TraceResult* ptr)
{
	TRACE_HULL(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr);
}

void UTIL_TraceModel(const Vector& vecStart, const Vector& vecEnd, int hullNumber, edict_t* pentModel, TraceResult* ptr)
{
	g_engfuncs.pfnTraceModel(vecStart, vecEnd, hullNumber, pentModel, ptr);
}

bool UTIL_CheckClearSight(const Vector& pos1, const Vector& pos2, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t* pentIgnore)
{
	TraceResult tr;
	UTIL_TraceLine(pos1, pos2, igmon, ignoreGlass, pentIgnore, &tr);
	return tr.flFraction == 1.f;
}


// Taken from jyk's separating axis theorem code in https://www.gamedev.net/forums/topic/338987-aabb---line-segment-intersection-test/?tab=comments#comment-3209917 - Max Vollmer, 2018-01-10
bool UTIL_TraceBBox(const Vector& vecStart, const Vector& vecEnd, const Vector& absmin, const Vector& absmax)
{
	const Vector vecHalfDir = (vecEnd - vecStart) * 0.5f;
	const Vector vecabsHalfDir(fabs(vecHalfDir.x), fabs(vecHalfDir.y), fabs(vecHalfDir.z));

	const Vector vecBBoxDir = (absmax - absmin) * 0.5f;

	const Vector vecBBoxCenter = (absmin + absmax) * 0.5f;
	const Vector vecStartInLocalSpace = vecStart - vecBBoxCenter;
	const Vector vecHalfEndInLocalSpace = vecStartInLocalSpace + vecHalfDir;

	if (fabs(vecHalfEndInLocalSpace[0]) > vecBBoxDir[0] + vecabsHalfDir[0])
		return false;
	if (fabs(vecHalfEndInLocalSpace[1]) > vecBBoxDir[1] + vecabsHalfDir[1])
		return false;
	if (fabs(vecHalfEndInLocalSpace[2]) > vecBBoxDir[2] + vecabsHalfDir[2])
		return false;

	if (fabs(vecHalfDir[1] * vecHalfEndInLocalSpace[2] - vecHalfDir[2] * vecHalfEndInLocalSpace[1]) > vecBBoxDir[1] * vecabsHalfDir[2] + vecBBoxDir[2] * vecabsHalfDir[1] + EPSILON)
		return false;
	if (fabs(vecHalfDir[2] * vecHalfEndInLocalSpace[0] - vecHalfDir[0] * vecHalfEndInLocalSpace[2]) > vecBBoxDir[2] * vecabsHalfDir[0] + vecBBoxDir[0] * vecabsHalfDir[2] + EPSILON)
		return false;
	if (fabs(vecHalfDir[0] * vecHalfEndInLocalSpace[1] - vecHalfDir[1] * vecHalfEndInLocalSpace[0]) > vecBBoxDir[0] * vecabsHalfDir[1] + vecBBoxDir[1] * vecabsHalfDir[0] + EPSILON)
		return false;

	return true;
}

bool UTIL_IsPointInEntity(CBaseEntity* pEntity, const Vector& p)
{
	if (!pEntity)
		return false;

	// No model or model is studio model
	if (!pEntity->pev->model || STRING(pEntity->pev->model)[0] == 0 || STRING(pEntity->pev->model)[0] != '*')
	{
		// TODO: Do proper hitbox check
		return UTIL_PointInsideBBox(p, pEntity->pev->origin + pEntity->pev->mins, pEntity->pev->origin + pEntity->pev->maxs);
	}

	int backupskin = pEntity->pev->skin;
	pEntity->pev->skin = CONTENTS_VR_TEMP_HACK;

	edict_t* pent = nullptr;
	int contents = UTIL_PointContents(p, true, &pent);

	pEntity->pev->skin = backupskin;

	return (contents == CONTENTS_VR_TEMP_HACK) || (pEntity->edict() == pent);
}


void TODO_TraceClipNodes()
{
	ENT(0)->v.model;
}


TraceResult UTIL_GetGlobalTrace()
{
	TraceResult tr;

	tr.fAllSolid = gpGlobals->trace_allsolid;
	tr.fStartSolid = gpGlobals->trace_startsolid;
	tr.fInOpen = gpGlobals->trace_inopen;
	tr.fInWater = gpGlobals->trace_inwater;
	tr.flFraction = gpGlobals->trace_fraction;
	tr.flPlaneDist = gpGlobals->trace_plane_dist;
	tr.pHit = gpGlobals->trace_ent;
	tr.vecEndPos = gpGlobals->trace_endpos;
	tr.vecPlaneNormal = gpGlobals->trace_plane_normal;
	tr.iHitgroup = gpGlobals->trace_hitgroup;
	return tr;
}


void UTIL_SetSize(entvars_t* pev, const Vector& vecMin, const Vector& vecMax)
{
	SET_SIZE(ENT(pev), vecMin, vecMax);
}


float UTIL_VecToYaw(const Vector& vec)
{
	return VEC_TO_YAW(vec);
}


void UTIL_SetOrigin(entvars_t* pev, const Vector& vecOrigin)
{
	SET_ORIGIN(ENT(pev), vecOrigin);
}

void UTIL_ParticleEffect(const Vector& vecOrigin, const Vector& vecDirection, ULONG ulColor, ULONG ulCount)
{
	PARTICLE_EFFECT(vecOrigin, vecDirection, (float)ulColor, (float)ulCount);
}


float UTIL_Approach(float target, float value, float speed)
{
	float delta = target - value;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}


float UTIL_ApproachAngle(float target, float value, float speed)
{
	target = UTIL_AngleMod(target);
	value = UTIL_AngleMod(target);

	float delta = target - value;

	// Speed is assumed to be positive
	if (speed < 0)
		speed = -speed;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}


float UTIL_AngleDistance(float next, float cur)
{
	float delta = next - cur;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	return delta;
}


float UTIL_SplineFraction(float value, float scale)
{
	value = scale * value;
	float valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return 3 * valueSquared - 2 * valueSquared * value;
}


char* UTIL_VarArgs(char* format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf_s(string, format, argptr);
	va_end(argptr);

	return string;
}

Vector UTIL_GetAimVector(edict_t* pent, float flSpeed)
{
	Vector tmp;
	GET_AIM_VECTOR(pent, flSpeed, tmp);
	return tmp;
}

int UTIL_IsMasterTriggered(string_t sMaster, CBaseEntity* pActivator)
{
	if (sMaster)
	{
		edict_t* pentTarget = FIND_ENTITY_BY_TARGETNAME(nullptr, STRING(sMaster));

		if (!FNullEnt(pentTarget))
		{
			CBaseEntity* pMaster = CBaseEntity::SafeInstance<CBaseEntity>(pentTarget);
			if (pMaster && (pMaster->ObjectCaps() & FCAP_MASTER))
				return pMaster->IsTriggered(pActivator);
		}

		ALERT(at_console, "Master was null or not a master!\n");
	}

	// if this isn't a master entity, just say yes.
	return 1;
}

BOOL UTIL_ShouldShowBlood(int color)
{
	if (color != DONT_BLEED)
	{
		if (color == BLOOD_COLOR_RED)
		{
			if (CVAR_GET_FLOAT("violence_hblood") != 0)
				return TRUE;
		}
		else
		{
			if (CVAR_GET_FLOAT("violence_ablood") != 0)
				return TRUE;
		}
	}
	return FALSE;
}



// Convenience method to check if a point is inside a bbox - Max Vollmer, 2017-12-27
bool UTIL_PointInsideBBox(const Vector& vec, const Vector& absmin, const Vector& absmax)
{
	return absmin.x < vec.x && absmin.y < vec.y && absmin.z < vec.z && absmax.x > vec.x && absmax.y > vec.y && absmax.z > vec.z;
}

// From https://stackoverflow.com/a/3235902/9199167
bool GetIntersection(float fDst1, float fDst2, const Vector& vec1, const Vector& vec2, Vector& result)
{
	if ((fDst1 * fDst2) >= 0.0f)
		return false;
	if (fDst1 == fDst2)
		return false;
	result = vec1 + (vec2 - vec1) * (-fDst1 / (fDst2 - fDst1));
	return true;
}

bool InBox(const Vector& v, const Vector& absmin, const Vector& absmax, int axis)
{
	if (axis == 1 && v.z > absmin.z&& v.z < absmax.z && v.y > absmin.y&& v.y < absmax.y)
		return true;
	if (axis == 2 && v.z > absmin.z&& v.z < absmax.z && v.x > absmin.x&& v.x < absmax.x)
		return true;
	if (axis == 3 && v.x > absmin.x&& v.x < absmax.x && v.y > absmin.y&& v.y < absmax.y)
		return true;
	return false;
}

bool UTIL_GetLineIntersectionWithBBox(const Vector& vec1, const Vector& vec2, const Vector& absmin, const Vector& absmax, Vector& result)
{
	if (vec2.x < absmin.x && vec1.x < absmin.x)
		return false;
	if (vec2.x > absmax.x&& vec1.x > absmax.x)
		return false;
	if (vec2.y < absmin.y && vec1.y < absmin.y)
		return false;
	if (vec2.y > absmax.y&& vec1.y > absmax.y)
		return false;
	if (vec2.z < absmin.z && vec1.z < absmin.z)
		return false;
	if (vec2.z > absmax.z&& vec1.z > absmax.z)
		return false;
	if (vec1.x > absmin.x&& vec1.x < absmax.x &&
		vec1.y > absmin.y&& vec1.y < absmax.y &&
		vec1.z > absmin.z&& vec1.z < absmax.z)
	{
		result = vec1;
		return true;
	}

	if ((GetIntersection(vec1.x - absmin.x, vec2.x - absmin.x, vec1, vec2, result) && InBox(result, absmin, absmax, 1)) || (GetIntersection(vec1.y - absmin.y, vec2.y - absmin.y, vec1, vec2, result) && InBox(result, absmin, absmax, 2)) || (GetIntersection(vec1.z - absmin.z, vec2.z - absmin.z, vec1, vec2, result) && InBox(result, absmin, absmax, 3)) || (GetIntersection(vec1.x - absmax.x, vec2.x - absmax.x, vec1, vec2, result) && InBox(result, absmin, absmax, 1)) || (GetIntersection(vec1.y - absmax.y, vec2.y - absmax.y, vec1, vec2, result) && InBox(result, absmin, absmax, 2)) || (GetIntersection(vec1.z - absmax.z, vec2.z - absmax.z, vec1, vec2, result) && InBox(result, absmin, absmax, 3)))
	{
		return true;
	}

	return false;
}

// Convenience method to check if a point is inside a brush model (Algorithm could be more performant, but this is HL after all) - Max Vollmer, 2017-08-26
bool UTIL_PointInsideBSPModel(const Vector& vec, const Vector& absmin, const Vector& absmax)
{
	bool inside = false;
	if (UTIL_PointInsideBBox(vec, absmin, absmax))
	{
		TraceResult tr;
		Vector startPos = absmin;
		Vector vecDirEpsilon = (vec - startPos).Normalize() * 0.01f;
		startPos = startPos - vecDirEpsilon;
		while (startPos.x < vec.x && startPos.y < vec.y && startPos.z < vec.z)
		{
			if (inside)
			{
				UTIL_TraceLine(vec, startPos, ignore_monsters, nullptr, &tr);
			}
			else
			{
				UTIL_TraceLine(startPos, vec, ignore_monsters, nullptr, &tr);
			}
			if (tr.flFraction < 1.f)
			{
				inside = !inside;
				startPos = tr.vecEndPos + vecDirEpsilon;
			}
			else
			{
				break;
			}
		}
	}
	return inside;
}

// Extended UTIL_PointContents to also detect solid entities - Max Vollmer, 2017-08-26
int UTIL_PointContents(const Vector& vec, bool detectSolidEntities, edict_t** pPent)
{
	int pointContents = POINT_CONTENTS(vec);
	if (detectSolidEntities)
	{
		edict_t* pEnt = g_engfuncs.pfnPEntityOfEntIndex(1);
		if (pEnt != nullptr)
		{
			for (int i = 1; i < gpGlobals->maxEntities; i++, pEnt++)
			{
				if (pEnt->free)
				{
					continue;
				}
				if (vec.x > pEnt->v.absmax.x ||
					vec.y > pEnt->v.absmax.y ||
					vec.z > pEnt->v.absmax.z ||
					vec.x < pEnt->v.absmin.x ||
					vec.y < pEnt->v.absmin.y ||
					vec.z < pEnt->v.absmin.z)
				{
					continue;
				}
				if (pEnt->v.solid == SOLID_BSP && UTIL_PointInsideBSPModel(vec, pEnt->v.absmin, pEnt->v.absmax))  // Trace actual brush model to see if point really is inside the entity!
				{
					if (pPent)
						*pPent = pEnt;
					return CONTENTS_SOLID;
				}
				else if (pEnt->v.movetype == MOVETYPE_PUSHSTEP && pEnt->v.solid != SOLID_NOT && pEnt->v.solid != SOLID_TRIGGER)
				{
					if (pPent)
						*pPent = pEnt;
					return CONTENTS_SOLID;
				}
			}
		}
	}
	return pointContents;
}

#include "com_model.h"

/*
EHANDLE<CBaseEntity> hFakeMonster;

// Returns true if the given bbox intersects with (or is completely enclosed by) something solid - Max Vollmer, 2017-12-27
bool UTIL_BBoxIntersectsBSPModel(const Vector &origin, const Vector &mins, const Vector &maxs)
{
	if (!hFakeMonster)
	{
		hFakeMonster = CBaseEntity::Create<CBaseEntity>("info_target", origin, Vector());
	}
	CBaseEntity *pFakeMonster = hFakeMonster;
	if (pFakeMonster != nullptr)
	{
		UTIL_SetOrigin(pFakeMonster->pev, origin);

		const Vector absminsmaxs[2] = { origin + mins, origin + maxs };

		for (int i = 0; i < 6; i++)
		{
			Vector curminsmaxs[2] = { mins, maxs };
			curminsmaxs[0][i / 2] /= 2;
			curminsmaxs[1][i / 2] /= 2;
			UTIL_SetSize(pFakeMonster->pev, curminsmaxs[0], curminsmaxs[1]);

			Vector start = origin;
			Vector end = origin;
			start[i / 2] += curminsmaxs[i % 2][i / 2];
			end[i / 2] += curminsmaxs[1 - i % 2][i / 2];
			Vector dirEpsilon = (end - start).Normalize() * 0.01f;

			TraceResult tr;
			tr.flFraction = 0.0f;

			Vector nextStart = start + dirEpsilon;

			while (tr.flFraction < 1.0f && ((start[i / 2] < end[i / 2]) == (nextStart[i / 2] < end[i / 2])))
			{
				TRACE_MONSTER_HULL(pFakeMonster->edict(), nextStart, end, ignore_monsters, pFakeMonster->edict(), &tr);
				if (tr.flFraction > 0.0f && tr.flFraction < 1.0f && UTIL_PointInsideBBox(tr.vecEndPos, absminsmaxs[0], absminsmaxs[1]))
				{
					return true;
				}
				nextStart = tr.vecEndPos + dirEpsilon;
			}
		}
	}
	int contents = UTIL_PointContents(origin, true);
	return contents == CONTENTS_SOLID || contents == CONTENTS_SKY;
}
*/

// Returns true if the given bboxes intersect - Max Vollmer, 2018-02-11
bool UTIL_BBoxIntersectsBBox(const Vector& absmins1, const Vector& absmaxs1, const Vector& absmins2, const Vector& absmaxs2)
{
	if (absmaxs1.x < absmins2.x || absmins1.x > absmaxs2.x)
		return false;
	if (absmaxs1.y < absmins2.y || absmins1.y > absmaxs2.y)
		return false;
	if (absmaxs1.z < absmins2.z || absmins1.z > absmaxs2.z)
		return false;
	return true;
}

// Returns true if the point is inside the rotated bbox - Max Vollmer, 2018-02-11
bool UTIL_PointInsideRotatedBBox(const Vector& bboxCenter, const Vector& bboxAngles, const Vector& bboxMins, const Vector& bboxMaxs, const Vector& checkVec)
{
	Vector rotatedLocalCheckVec = checkVec - bboxCenter;
	VRPhysicsHelper::Instance().RotateVector(rotatedLocalCheckVec, -bboxAngles, Vector(), true);
	return UTIL_PointInsideBBox(rotatedLocalCheckVec, bboxMins, bboxMaxs);
}

/*
// Returns true if the given bboxes intersect - Max Vollmer, 2018-02-11
bool UTIL_RotatedBBoxIntersectsBBox(const Vector & bboxCenter, const Vector & bboxAngles, const Vector & bboxMins, const Vector & bboxMaxs, const Vector & absmin, const Vector & absmax)
{
	if (bboxAngles.Length() < 0.01f)
	{
		return UTIL_BBoxIntersectsBBox(absmin, absmax, bboxCenter + bboxMins, bboxCenter + bboxMaxs);
	}
	else
	{
		Vector absminmax[2] = { absmin, absmax };
		for (int x = 0; x < 2; ++x)
		{
			for (int y = 0; y < 2; ++y)
			{
				for (int z = 0; z < 2; ++z)
				{
					Vector currentCorner{ absminmax[x].x, absminmax[y].y, absminmax[z].z };
					if (UTIL_PointInsideRotatedBBox(bboxCenter, bboxAngles, bboxMins, bboxMaxs, currentCorner))
					{
						return true;
					}
				}
			}
		}
		Vector rotatedBBoxMins = bboxMins;
		Vector rotatedBBoxMaxs = bboxMaxs;
		VRPhysicsHelper::Instance().RotateBBox(rotatedBBoxMins, rotatedBBoxMaxs, bboxAngles);
		Vector rotatedBBoxAbsMinmax[2] = { bboxCenter + rotatedBBoxMins, bboxCenter + rotatedBBoxMaxs };
		for (int x = 0; x < 2; ++x)
		{
			for (int y = 0; y < 2; ++y)
			{
				for (int z = 0; z < 2; ++z)
				{
					Vector currentCorner{ rotatedBBoxAbsMinmax[x].x, rotatedBBoxAbsMinmax[y].y, rotatedBBoxAbsMinmax[z].z };
					if (UTIL_PointInsideBBox(currentCorner, absmin, absmax))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}
*/

// Copied from studio_util.cpp (note: expects radians!)
void UTIL_AngleQuaternion(const Vector& angles, float quaternion[4])
{
	float angle = 0.f;
	float sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5;
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[1] * 0.5;
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[0] * 0.5;
	sr = sin(angle);
	cr = cos(angle);

	quaternion[0] = sr * cp * cy - cr * sp * sy;  // X
	quaternion[1] = cr * sp * cy + sr * cp * sy;  // Y
	quaternion[2] = cr * cp * sy - sr * sp * cy;  // Z
	quaternion[3] = cr * cp * cy + sr * sp * sy;  // W
}

// Method that returns the exact angles for a complete set of vectors
void UTIL_GetAnglesFromVectors(const Vector& forward, const Vector& right, const Vector& up, Vector& angles)
{
	float sr, sp, sy, cr, cp, cy;

	sp = -forward[2];

	float cp_x_cy = forward[0];
	float cp_x_sy = forward[1];
	float cp_x_sr = -right[2];
	float cp_x_cr = up[2];

	float yaw = atan2(cp_x_sy, cp_x_cy);
	float roll = atan2(cp_x_sr, cp_x_cr);

	cy = cos(yaw);
	sy = sin(yaw);
	cr = cos(roll);
	sr = sin(roll);

	if (abs(cy) > EPSILON)
	{
		cp = cp_x_cy / cy;
	}
	else if (abs(sy) > EPSILON)
	{
		cp = cp_x_sy / sy;
	}
	else if (abs(sr) > EPSILON)
	{
		cp = cp_x_sr / sr;
	}
	else if (abs(cr) > EPSILON)
	{
		cp = cp_x_cr / cr;
	}
	else
	{
		cp = cos(asin(sp));
	}

	float pitch = atan2(sp, cp);

	angles[0] = pitch / (M_PI * 2.f / 360.f);
	angles[1] = yaw / (M_PI * 2.f / 360.f);
	angles[2] = roll / (M_PI * 2.f / 360.f);
}

void UTIL_BloodStream(const Vector& origin, const Vector& direction, int color, int amount)
{
	if (!UTIL_ShouldShowBlood(color))
		return;

	if (g_Language == LANGUAGE_GERMAN && color == BLOOD_COLOR_RED)
		color = 0;


	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_BLOODSTREAM);
	WRITE_COORD(origin.x);
	WRITE_COORD(origin.y);
	WRITE_COORD(origin.z);
	WRITE_COORD(direction.x);
	WRITE_COORD(direction.y);
	WRITE_COORD(direction.z);
	WRITE_BYTE(color);
	WRITE_BYTE(min(amount, 255));
	MESSAGE_END();
}

void UTIL_BloodDrips(const Vector& origin, const Vector& direction, int color, int amount)
{
	if (!UTIL_ShouldShowBlood(color))
		return;

	if (color == DONT_BLEED || amount == 0)
		return;

	if (g_Language == LANGUAGE_GERMAN && color == BLOOD_COLOR_RED)
		color = 0;

	if (g_pGameRules->IsMultiplayer())
	{
		// scale up blood effect in multiplayer for better visibility
		amount *= 2;
	}

	if (amount > 255)
		amount = 255;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_BLOODSPRITE);
	WRITE_COORD(origin.x);  // pos
	WRITE_COORD(origin.y);
	WRITE_COORD(origin.z);
	WRITE_SHORT(g_sModelIndexBloodSpray);      // initial sprite model
	WRITE_SHORT(g_sModelIndexBloodDrop);       // droplet sprite models
	WRITE_BYTE(color);                         // color index into host_basepal
	WRITE_BYTE(min(max(3, amount / 10), 16));  // size
	MESSAGE_END();
}

Vector UTIL_RandomBloodVector(void)
{
	Vector direction;

	direction.x = RANDOM_FLOAT(-1, 1);
	direction.y = RANDOM_FLOAT(-1, 1);
	direction.z = RANDOM_FLOAT(0, 1);

	return direction;
}


void UTIL_BloodDecalTrace(TraceResult* pTrace, int bloodColor)
{
	if (UTIL_ShouldShowBlood(bloodColor))
	{
		if (bloodColor == BLOOD_COLOR_RED)
			UTIL_DecalTrace(pTrace, DECAL_BLOOD1 + RANDOM_LONG(0, 5));
		else
			UTIL_DecalTrace(pTrace, DECAL_YBLOOD1 + RANDOM_LONG(0, 5));
	}
}


void UTIL_DecalTrace(TraceResult* pTrace, int decalNumber)
{
	short entityIndex = 0;
	int index = 0;
	int message = 0;

	if (decalNumber < 0)
		return;

	index = gDecals[decalNumber].index;

	if (index < 0)
		return;

	if (pTrace->flFraction == 1.0)
		return;

	// Only decal BSP models
	if (pTrace->pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::SafeInstance<CBaseEntity>(pTrace->pHit);
		if (!pEntity)
			entityIndex = 0;
		else if (!pEntity->IsBSPModel())
			return;
		else
			entityIndex = ENTINDEX(pTrace->pHit);
	}
	else
		entityIndex = 0;

	message = TE_DECAL;
	if (entityIndex != 0)
	{
		if (index > 255)
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;
		if (index > 255)
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(message);
	WRITE_COORD(pTrace->vecEndPos.x);
	WRITE_COORD(pTrace->vecEndPos.y);
	WRITE_COORD(pTrace->vecEndPos.z);
	WRITE_BYTE(index);
	if (entityIndex)
		WRITE_SHORT(entityIndex);
	MESSAGE_END();
}

/*
==============
UTIL_PlayerDecalTrace

A player is trying to apply their custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void UTIL_PlayerDecalTrace(TraceResult* pTrace, int playernum, int decalNumber, BOOL bIsCustom)
{
	int index = 0;

	if (!bIsCustom)
	{
		if (decalNumber < 0)
			return;

		index = gDecals[decalNumber].index;
		if (index < 0)
			return;
	}
	else
		index = decalNumber;

	if (pTrace->flFraction == 1.0)
		return;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_PLAYERDECAL);
	WRITE_BYTE(playernum);
	WRITE_COORD(pTrace->vecEndPos.x);
	WRITE_COORD(pTrace->vecEndPos.y);
	WRITE_COORD(pTrace->vecEndPos.z);
	WRITE_SHORT((short)ENTINDEX(pTrace->pHit));
	WRITE_BYTE(index);
	MESSAGE_END();
}

void UTIL_GunshotDecalTrace(TraceResult* pTrace, int decalNumber)
{
	if (decalNumber < 0)
		return;

	int index = gDecals[decalNumber].index;
	if (index < 0)
		return;

	if (pTrace->flFraction == 1.0)
		return;

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pTrace->vecEndPos);
	WRITE_BYTE(TE_GUNSHOTDECAL);
	WRITE_COORD(pTrace->vecEndPos.x);
	WRITE_COORD(pTrace->vecEndPos.y);
	WRITE_COORD(pTrace->vecEndPos.z);
	WRITE_SHORT((short)ENTINDEX(pTrace->pHit));
	WRITE_BYTE(index);
	MESSAGE_END();
}


void UTIL_Sparks(const Vector& position)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, position);
	WRITE_BYTE(TE_SPARKS);
	WRITE_COORD(position.x);
	WRITE_COORD(position.y);
	WRITE_COORD(position.z);
	MESSAGE_END();
}


void UTIL_Ricochet(const Vector& position, float scale)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, position);
	WRITE_BYTE(TE_ARMOR_RICOCHET);
	WRITE_COORD(position.x);
	WRITE_COORD(position.y);
	WRITE_COORD(position.z);
	WRITE_BYTE((int)(scale * 10));
	MESSAGE_END();
}


BOOL UTIL_TeamsMatch(const char* pTeamName1, const char* pTeamName2)
{
	// Everyone matches unless it's teamplay
	if (!g_pGameRules->IsTeamplay())
		return TRUE;

	// Both on a team?
	if (*pTeamName1 != 0 && *pTeamName2 != 0)
	{
		if (!_stricmp(pTeamName1, pTeamName2))  // Same Team?
			return TRUE;
	}

	return FALSE;
}


void UTIL_StringToVector(float* pVector, const char* pString)
{
	char* pstr, * pfront, tempString[128];
	int j = 0;

	strcpy_s(tempString, pString);
	pstr = pfront = tempString;

	for (j = 0; j < 3; j++)  // lifted from pr_edict.c
	{
		pVector[j] = atof(pfront);

		while (*pstr && *pstr != ' ')
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}
	if (j < 2)
	{
		/*
		ALERT( at_error, "Bad field in entity!! %s:%s == \"%s\"\n",
			pkvd->szClassName, pkvd->szKeyName, pkvd->szValue );
		*/
		for (j = j + 1; j < 3; j++)
			pVector[j] = 0;
	}
}


void UTIL_StringToIntArray(int* pVector, int count, const char* pString)
{
	char* pstr, * pfront, tempString[128];
	int j = 0;

	strcpy_s(tempString, pString);
	pstr = pfront = tempString;

	for (j = 0; j < count; j++)  // lifted from pr_edict.c
	{
		pVector[j] = atoi(pfront);

		while (*pstr && *pstr != ' ')
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for (j++; j < count; j++)
	{
		pVector[j] = 0;
	}
}

Vector UTIL_ClampVectorToBox(const Vector& input, const Vector& clampSize)
{
	Vector sourceVector = input;

	if (sourceVector.x > clampSize.x)
		sourceVector.x -= clampSize.x;
	else if (sourceVector.x < -clampSize.x)
		sourceVector.x += clampSize.x;
	else
		sourceVector.x = 0;

	if (sourceVector.y > clampSize.y)
		sourceVector.y -= clampSize.y;
	else if (sourceVector.y < -clampSize.y)
		sourceVector.y += clampSize.y;
	else
		sourceVector.y = 0;

	if (sourceVector.z > clampSize.z)
		sourceVector.z -= clampSize.z;
	else if (sourceVector.z < -clampSize.z)
		sourceVector.z += clampSize.z;
	else
		sourceVector.z = 0;

	return sourceVector.Normalize();
}


float UTIL_WaterLevel(const Vector& position, float minz, float maxz)
{
	Vector midUp = position;
	midUp.z = minz;

	if (UTIL_PointContents(midUp) != CONTENTS_WATER)
		return minz;

	midUp.z = maxz;
	if (UTIL_PointContents(midUp) == CONTENTS_WATER)
		return maxz;

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff / 2.0;
		if (UTIL_PointContents(midUp) == CONTENTS_WATER)
		{
			minz = midUp.z;
		}
		else
		{
			maxz = midUp.z;
		}
		diff = maxz - minz;
	}

	return midUp.z;
}


const Vector UTIL_WaterLevelPos(const Vector& start, const Vector& end)
{
	if (UTIL_PointContents(start) != CONTENTS_WATER)
	{
		return start;
	}

	if (UTIL_PointContents(end) == CONTENTS_WATER)
	{
		return end;
	}

	const Vector middle = (start + end) * 0.5f;
	const float dist = (end - start).Length();
	if (dist > 1.0f)
	{
		if (UTIL_PointContents(middle) == CONTENTS_WATER)
		{
			return UTIL_WaterLevelPos(middle, end);
		}
		else
		{
			return UTIL_WaterLevelPos(start, middle);
		}
	}
	else
	{
		return middle;
	}
}


extern DLL_GLOBAL short g_sModelIndexBubbles;  // holds the index for the bubbles model

void UTIL_Bubbles(Vector mins, Vector maxs, int count)
{
	Vector mid = (mins + maxs) * 0.5;

	float flHeight = UTIL_WaterLevel(mid, mid.z, mid.z + 1024);
	flHeight = flHeight - mins.z;

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, mid);
	WRITE_BYTE(TE_BUBBLES);
	WRITE_COORD(mins.x);  // mins
	WRITE_COORD(mins.y);
	WRITE_COORD(mins.z);
	WRITE_COORD(maxs.x);  // maxz
	WRITE_COORD(maxs.y);
	WRITE_COORD(maxs.z);
	WRITE_COORD(flHeight);  // height
	WRITE_SHORT(g_sModelIndexBubbles);
	WRITE_BYTE(count);  // count
	WRITE_COORD(8);     // speed
	MESSAGE_END();
}

void UTIL_BubbleTrail(Vector from, Vector to, int count)
{
	float flHeight = UTIL_WaterLevel(from, from.z, from.z + 256);
	flHeight = flHeight - from.z;

	if (flHeight < 8)
	{
		flHeight = UTIL_WaterLevel(to, to.z, to.z + 256);
		flHeight = flHeight - to.z;
		if (flHeight < 8)
			return;

		// UNDONE: do a ploink sound
		flHeight = flHeight + to.z - from.z;
	}

	if (count > 255)
		count = 255;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BUBBLETRAIL);
	WRITE_COORD(from.x);  // mins
	WRITE_COORD(from.y);
	WRITE_COORD(from.z);
	WRITE_COORD(to.x);  // maxz
	WRITE_COORD(to.y);
	WRITE_COORD(to.z);
	WRITE_COORD(flHeight);  // height
	WRITE_SHORT(g_sModelIndexBubbles);
	WRITE_BYTE(count);  // count
	WRITE_COORD(8);     // speed
	MESSAGE_END();
}


void UTIL_Remove(CBaseEntity* pEntity)
{
	if (!pEntity)
		return;

	pEntity->UpdateOnRemove();
	pEntity->pev->flags |= FL_KILLME;
	pEntity->pev->targetname = 0;
}


BOOL UTIL_IsValidEntity(edict_t* pent)
{
	if (!pent || pent->free || (pent->v.flags & FL_KILLME))
		return FALSE;
	return TRUE;
}


void UTIL_PrecacheOther(const char* szClassname)
{
	edict_t* pent;

	pent = CREATE_NAMED_ENTITY(MAKE_STRING(szClassname));
	if (FNullEnt(pent))
	{
		ALERT(at_console, "nullptr Ent in UTIL_PrecacheOther\n");
		return;
	}

	CBaseEntity* pEntity = CBaseEntity::SafeInstance<CBaseEntity>(pent);
	if (pEntity)
		pEntity->Precache();
	REMOVE_ENTITY(pent);
}

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf(char* fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	vsprintf_s(string, fmt, argptr);
	va_end(argptr);

	// Print to server console
	ALERT(at_logged, "%s", string);
}

//=========================================================
// UTIL_DotPoints - returns the dot product of a line from
// src to check and vecdir.
//=========================================================
float UTIL_DotPoints(const Vector& vecSrc, const Vector& vecCheck, const Vector& vecDir)
{
	Vector2D vec2LOS;

	vec2LOS = (vecCheck - vecSrc).Make2D();
	vec2LOS = vec2LOS.Normalize();

	return DotProduct(vec2LOS, (vecDir.Make2D()));
}


//=========================================================
// UTIL_StripToken - for redundant keynames
//=========================================================
void UTIL_StripToken(const char* pKey, char* pDest)
{
	int i = 0;

	while (pKey[i] && pKey[i] != '#')
	{
		pDest[i] = pKey[i];
		i++;
	}
	pDest[i] = 0;
}


// Convenience function to get the constants of a parabola function (in 2d space) from 3 points - 2017-12-25, Max Vollmer
void UTIL_ParabolaFromPoints(const Vector2D& p1, const Vector2D& p2, const Vector2D& p3, Vector& parabola)
{
	// General function is: y = A*x*x + B*x + C

	// Resolve for C with first point
	// A*p1.x*p1.x + B*p1.x + C = p1.y
	// ==> C = p1.y - A*p1.x*p1.x - B*p1.x

	// Resolve for B with C and second point
	// A*p2.x*p2.x + B*p2.x + p1.y - A*p1.x*p1.x - B*p1.x = p2.y
	// ===> B = (p2.y - p1.y - A*(p2.x*p2.x - p1.x*p1.x)) / (p2.x - p1.x)

	// Resolve for A with B and C and third point
	// A*p3.x*p3.x + ((p2.y - p1.y - A*(p2.x*p2.x - p1.x*p1.x)) / (p2.x - p1.x))*p3.x + p1.y - A*p1.x*p1.x - ((p2.y - p1.y - A*(p2.x*p2.x - p1.x*p1.x)) / (p2.x - p1.x))*p1.x
	// ===> A = ((p2.y - p1.y)*(p1.x - p3.x) + (p3.y - p1.y)*(p2.x - p1.x)) / ((p1.x - p3.x)*(p2.x*p2.x - p1.x*p1.x) + (p2.x - p1.x)*(p3.x*p3.x - p1.x*p1.x))

	// Do the actual calculations:
	float A = ((p2.y - p1.y) * (p1.x - p3.x) + (p3.y - p1.y) * (p2.x - p1.x)) / ((p1.x - p3.x) * (p2.x * p2.x - p1.x * p1.x) + (p2.x - p1.x) * (p3.x * p3.x - p1.x * p1.x));
	float B = ((p2.y - p1.y) - A * (p2.x * p2.x - p1.x * p1.x)) / (p2.x - p1.x);
	float C = p1.y - A * p1.x * p1.x - B * p1.x;

	parabola.x = A;
	parabola.y = B;
	parabola.z = C;
}


// Moved from barney.cpp and made 3d (why tf was this 2d?) - Max Vollmer, 2018-01-02
bool UTIL_IsFacing(const Vector& viewPos, const Vector& viewAngles, const Vector& reference)
{
	Vector referenceDir = (reference - viewPos);
	Vector viewDir;
	UTIL_MakeVectorsPrivate(viewAngles, viewDir, nullptr, nullptr);
	if (DotProduct(viewDir, referenceDir) > 0.96f)  // +/- 15 degrees or so
	{
		return true;
	}
	return false;
}

bool UTIL_StartsWith(const char* s1, const char* s2)
{
	return strncmp(s1, s2, strlen(s2)) == 0;
}

// Checks if a trace would hit this entity (does not check if stuff is in the way, use UTIL_CheckClearSight for that), entity must be solid - Max Vollmer, 2018-01-06
bool UTIL_CheckTraceIntersectsEntity(const Vector& pos1, const Vector& pos2, CBaseEntity* pCheck)
{
	// Check if line starts inside box
	if (UTIL_PointInsideBBox(pos1, pCheck->pev->absmin, pCheck->pev->absmax))
	{
		return true;
	}

	TraceResult tr;
	UTIL_TraceLine(pos1, pos2, dont_ignore_monsters, dont_ignore_glass, nullptr, &tr);
	if (tr.pHit == pCheck->edict() || UTIL_PointInsideBBox(tr.vecEndPos, pCheck->pev->absmin, pCheck->pev->absmax))
	{
		return true;
	}
	else if (tr.flFraction > 0.f && tr.flFraction < 1.f && (pos2 - tr.vecEndPos).Length() > 2.f)
	{
		return UTIL_CheckTraceIntersectsEntity(tr.vecEndPos + (pos2 - pos1).Normalize(), pos2, pCheck);
	}
	else
	{
		return false;
	}
}

#include "skill.h"

float UTIL_CalculateMeleeDamage(int iId, float speed)
{
	if (speed >= GetMeleeSwingSpeed())
	{
		float baseDamage = gSkillData.plrDmgCrowbar * speed / GetMeleeSwingSpeed();
		switch (iId)
		{
		case WEAPON_CROWBAR:
			return baseDamage;
		case WEAPON_HORNETGUN:
			return baseDamage * 0.05f;
		case WEAPON_NONE:
		case WEAPON_BAREHAND:
		case WEAPON_SNARK:
			return baseDamage * 0.1f;
		case WEAPON_HANDGRENADE:
		case WEAPON_TRIPMINE:
		case WEAPON_SATCHEL:
			return baseDamage * 0.2f;
		case WEAPON_GLOCK:
		case WEAPON_PYTHON:
			return baseDamage * 0.3f;
		case WEAPON_MP5:
		case WEAPON_SHOTGUN:
		case WEAPON_CROSSBOW:
			return baseDamage * 0.4f;
		case WEAPON_RPG:
		case WEAPON_GAUSS:
		case WEAPON_EGON:
			return baseDamage * 0.5f;
		default:
			return baseDamage * 0.1f;
		}
	}
	else
	{
		return 0;
	}
}

int UTIL_DamageTypeFromWeapon(int iId)
{
	switch (iId)
	{
	case WEAPON_CROWBAR:
		return DMG_CLUB;
	default:
		return DMG_CLUB | DMG_NEVERGIB;
	}
}


/*
bool UTIL_ShouldCollideWithPlayer(CBaseEntity *pOther)
{
	/*
	if (FClassnameIs(pOther->pev, "func_pushable") || FClassnameIs(pOther->pev, "monster_scientist") || FClassnameIs(pOther->pev, "monster_barney"))
	{
		return false;
	}
	* /
	/*
	if (pOther->pev->solid == SOLID_BSP || pOther->pev->movetype == MOVETYPE_PUSHSTEP)
	{
		return false;
	}
	* /
	return true;
}
*/

bool UTIL_ShouldCollide(CBaseEntity* pTouched, CBaseEntity* pOther)
{
	/*
	if (FBitSet(pTouched->pev->flags, FL_CLIENT))
	{
		return UTIL_ShouldCollideWithPlayer(pOther);
	}
	else if (FBitSet(pOther->pev->flags, FL_CLIENT))
	{
		return UTIL_ShouldCollideWithPlayer(pTouched);
	}
	if (FClassnameIs(pTouched->pev, "func_illusionary") || FClassnameIs(pOther->pev, "func_illusionary"))
	{
		return false;
	}
	*/
	return true;
}


// --------------------------------------------------------------
//
// CSave
//
// --------------------------------------------------------------
static int gSizes[FIELD_TYPECOUNT] =
{
	sizeof(float),      // FIELD_FLOAT
	sizeof(int),        // FIELD_STRING
	sizeof(int),        // FIELD_ENTITY
	sizeof(int),        // FIELD_CLASSPTR
	sizeof(int),        // FIELD_EHANDLE
	sizeof(int),        // FIELD_entvars_t
	sizeof(int),        // FIELD_EDICT
	sizeof(float) * 3,  // FIELD_VECTOR
	sizeof(float) * 3,  // FIELD_POSITION_VECTOR
	sizeof(int*),       // FIELD_POINTER
	sizeof(int),        // FIELD_INTEGER
	sizeof(int*),       // FIELD_FUNCTION
	sizeof(int),        // FIELD_BOOLEAN
	sizeof(short),      // FIELD_SHORT
	sizeof(char),       // FIELD_CHARACTER
	sizeof(float),      // FIELD_TIME
	sizeof(int),        // FIELD_MODELNAME
	sizeof(int),        // FIELD_SOUNDNAME
};


// Base class includes common SAVERESTOREDATA pointer, and manages the entity table
CSaveRestoreBuffer::CSaveRestoreBuffer(void)
{
	m_pdata = nullptr;
}


CSaveRestoreBuffer::CSaveRestoreBuffer(SAVERESTOREDATA* pdata)
{
	m_pdata = pdata;
}


CSaveRestoreBuffer ::~CSaveRestoreBuffer(void)
{
}

int CSaveRestoreBuffer::EntityIndex(CBaseEntity* pEntity)
{
	if (pEntity == nullptr)
		return -1;
	return EntityIndex(pEntity->pev);
}

int CSaveRestoreBuffer::EntityIndex(entvars_t* pevLookup)
{
	if (pevLookup == nullptr)
		return -1;
	return EntityIndex(ENT(pevLookup));
}

int CSaveRestoreBuffer::EntityIndex(EOFFSET eoLookup)
{
	return EntityIndex(ENT(eoLookup));
}

int CSaveRestoreBuffer::EntityIndex(edict_t* pentLookup)
{
	if (!m_pdata || pentLookup == nullptr)
		return -1;

	int i = 0;
	ENTITYTABLE* pTable;

	for (i = 0; i < m_pdata->tableCount; i++)
	{
		pTable = m_pdata->pTable + i;
		if (pTable->pent == pentLookup)
			return i;
	}
	return -1;
}


edict_t* CSaveRestoreBuffer::EntityFromIndex(int entityIndex)
{
	if (!m_pdata || entityIndex < 0)
		return nullptr;

	int i = 0;
	ENTITYTABLE* pTable;

	for (i = 0; i < m_pdata->tableCount; i++)
	{
		pTable = m_pdata->pTable + i;
		if (pTable->id == entityIndex)
			return pTable->pent;
	}
	return nullptr;
}


int CSaveRestoreBuffer::EntityFlagsSet(int entityIndex, int flags)
{
	if (!m_pdata || entityIndex < 0)
		return 0;
	if (entityIndex > m_pdata->tableCount)
		return 0;

	m_pdata->pTable[entityIndex].flags |= flags;

	return m_pdata->pTable[entityIndex].flags;
}


void CSaveRestoreBuffer::BufferRewind(int size)
{
	if (!m_pdata)
		return;

	if (m_pdata->size < size)
		size = m_pdata->size;

	m_pdata->pCurrentData -= size;
	m_pdata->size -= size;
}

#ifndef _WIN32
extern "C"
{
	unsigned _rotr(unsigned val, int shift)
	{
		register unsigned lobit;     /* non-zero means lo bit set */
		register unsigned num = val; /* number to rotate */

		shift &= 0x1f; /* modulo 32 -- this will also make
										   negative shifts work */

		while (shift--)
		{
			lobit = num & 1; /* get high bit */
			num >>= 1;       /* shift right one bit */
			if (lobit)
				num |= 0x80000000; /* set hi bit if lo bit was set */
		}

		return num;
	}
}
#endif

unsigned int CSaveRestoreBuffer::HashString(const char* pszToken)
{
	unsigned int hash = 0;

	while (*pszToken)
		hash = _rotr(hash, 4) ^ *pszToken++;

	return hash;
}

unsigned short CSaveRestoreBuffer::TokenHash(const char* pszToken)
{
	unsigned short hash = (unsigned short)(HashString(pszToken) % (unsigned)m_pdata->tokenCount);

#if _DEBUG
	static int tokensparsed = 0;
	tokensparsed++;
	if (!m_pdata->tokenCount || !m_pdata->pTokens)
		ALERT(at_error, "No token table array in TokenHash()!");
#endif

	for (int i = 0; i < m_pdata->tokenCount; i++)
	{
#if _DEBUG
		static qboolean beentheredonethat = FALSE;
		if (i > 50 && !beentheredonethat)
		{
			beentheredonethat = TRUE;
			ALERT(at_error, "CSaveRestoreBuffer :: TokenHash() is getting too full!");
		}
#endif

		int index = hash + i;
		if (index >= m_pdata->tokenCount)
			index -= m_pdata->tokenCount;

		if (!m_pdata->pTokens[index] || strcmp(pszToken, m_pdata->pTokens[index]) == 0)
		{
			m_pdata->pTokens[index] = pszToken;
			return index;
		}
	}

	// Token hash table full!!!
	// [Consider doing overflow table(s) after the main table & limiting linear hash table search]
	ALERT(at_error, "CSaveRestoreBuffer :: TokenHash() is COMPLETELY FULL!");
	return 0;
}

void CSave::WriteData(const char* pname, int size, const void* pdata)
{
	BufferField(pname, size, pdata);
}


void CSave::WriteShort(const char* pname, const short* data, int count)
{
	BufferField(pname, sizeof(short) * count, data);
}


void CSave::WriteInt(const char* pname, const int* data, int count)
{
	BufferField(pname, sizeof(int) * count, data);
}


void CSave::WriteFloat(const char* pname, const float* data, int count)
{
	BufferField(pname, sizeof(float) * count, data);
}


void CSave::WriteTime(const char* pname, const float* data, int count)
{
	int i = 0;
	Vector tmp, input;

	BufferHeader(pname, sizeof(float) * count);
	for (i = 0; i < count; i++)
	{
		float tmp = data[0];

		// Always encode time as a delta from the current time so it can be re-based if loaded in a new level
		// Times of 0 are never written to the file, so they will be restored as 0, not a relative time
		if (m_pdata)
			tmp -= m_pdata->time;

		BufferData(&tmp, sizeof(float));
		data++;
	}
}


void CSave::WriteString(const char* pname, const char* pdata)
{
#ifdef TOKENIZE
	short token = (short)TokenHash(pdata);
	WriteShort(pname, &token, 1);
#else
	BufferField(pname, strlen(pdata) + 1, pdata);
#endif
}


void CSave::WriteString(const char* pname, const int* stringId, int count)
{
	int i, size;

#ifdef TOKENIZE
	short token = (short)TokenHash(STRING(*stringId));
	WriteShort(pname, &token, 1);
#else
#if 0
	if (count != 1)
		ALERT(at_error, "No string arrays!\n");
	WriteString(pname, STRING(*stringId));
#endif

	size = 0;
	for (i = 0; i < count; i++)
		size += strlen(STRING(stringId[i])) + 1;

	BufferHeader(pname, size);
	for (i = 0; i < count; i++)
	{
		const char* pString = STRING(stringId[i]);
		BufferData(pString, strlen(pString) + 1);
	}
#endif
}


void CSave::WriteVector(const char* pname, const Vector& value)
{
	WriteVector(pname, &value.x, 1);
}


void CSave::WriteVector(const char* pname, const float* value, int count)
{
	BufferHeader(pname, sizeof(float) * 3 * count);
	BufferData(value, sizeof(float) * 3 * count);
}



void CSave::WritePositionVector(const char* pname, const Vector& value)
{
	if (m_pdata && m_pdata->fUseLandmark)
	{
		Vector tmp = value - m_pdata->vecLandmarkOffset;
		WriteVector(pname, tmp);
	}

	WriteVector(pname, value);
}


void CSave::WritePositionVector(const char* pname, const float* value, int count)
{
	int i = 0;
	Vector tmp, input;

	BufferHeader(pname, sizeof(float) * 3 * count);
	for (i = 0; i < count; i++)
	{
		Vector tmp(value[0], value[1], value[2]);

		if (m_pdata && m_pdata->fUseLandmark)
			tmp = tmp - m_pdata->vecLandmarkOffset;

		BufferData(&tmp.x, sizeof(float) * 3);
		value += 3;
	}
}


void CSave::WriteFunction(const char* pname, const int* data, int count)
{
	const char* functionName;

	functionName = NAME_FOR_FUNCTION(*data);
	if (functionName)
		BufferField(pname, strlen(functionName) + 1, functionName);
	else
		ALERT(at_error, "Invalid function pointer in entity!");
}


void EntvarsKeyvalue(entvars_t* pev, KeyValueData* pkvd)
{
	if (!pev || !pkvd)
		return;

	int i = 0;
	TYPEDESCRIPTION* pField;

	for (i = 0; i < ENTVARS_COUNT; i++)
	{
		pField = &gEntvarsDescription[i];

		if (!_stricmp(pField->fieldName, pkvd->szKeyName))
		{
			switch (pField->fieldType)
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				(*reinterpret_cast<int*>(reinterpret_cast<char*>(pev) + pField->fieldOffset)) = ALLOC_STRING(pkvd->szValue);
				break;

			case FIELD_TIME:
			case FIELD_FLOAT:
				(*reinterpret_cast<float*>(reinterpret_cast<char*>(pev) + pField->fieldOffset)) = atof(pkvd->szValue);
				break;

			case FIELD_INTEGER:
				(*reinterpret_cast<int*>(reinterpret_cast<char*>(pev) + pField->fieldOffset)) = atoi(pkvd->szValue);
				break;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				UTIL_StringToVector(reinterpret_cast<float*>(reinterpret_cast<char*>(pev) + pField->fieldOffset), pkvd->szValue);
				break;

			default:
			case FIELD_EVARS:
			case FIELD_CLASSPTR_NOT_SUPPORTED_ANYMORE:
			case FIELD_EDICT:
			case FIELD_ENTITY:
			case FIELD_POINTER:
				ALERT(at_error, "Bad field in entity!!\n");
				break;
			}
			pkvd->fHandled = TRUE;
			return;
		}
	}
}



int CSave::WriteEntVars(const char* pname, entvars_t* pev)
{
	return WriteFields(pname, pev, gEntvarsDescription, ENTVARS_COUNT);
}


int CSave::WriteFields(const char* pname, const void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	// Precalculate the number of empty fields
	int emptyCount = 0;
	for (int i = 0; i < fieldCount; i++)
	{
		TYPEDESCRIPTION* pTest = &pFields[i];
		const void* pOutputData = static_cast<const char*>(pBaseData) + pTest->fieldOffset;
		if (DataEmpty(pOutputData, pTest->fieldSize * gSizes[pTest->fieldType]))
		{
			emptyCount++;
		}
	}

	// Empty fields will not be written, write out the actual number of fields to be written
	int actualCount = fieldCount - emptyCount;
	WriteInt(pname, &actualCount, 1);

	int entityArray[MAX_ENTITYARRAY];

	for (int i = 0; i < fieldCount; i++)
	{
		TYPEDESCRIPTION* pTest = &pFields[i];
		const void* pOutputData = static_cast<const char*>(pBaseData) + pTest->fieldOffset;

		// UNDONE: Must we do this twice?
		if (DataEmpty(pOutputData, pTest->fieldSize * gSizes[pTest->fieldType]))
			continue;

		switch (pTest->fieldType)
		{
		case FIELD_FLOAT:
			WriteFloat(pTest->fieldName, static_cast<const float*>(pOutputData), pTest->fieldSize);
			break;
		case FIELD_TIME:
			WriteTime(pTest->fieldName, static_cast<const float*>(pOutputData), pTest->fieldSize);
			break;
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			WriteString(pTest->fieldName, static_cast<const int*>(pOutputData), pTest->fieldSize);
			break;
		case FIELD_CLASSPTR_NOT_SUPPORTED_ANYMORE:
		case FIELD_EVARS:
		case FIELD_EDICT:
		case FIELD_ENTITY:
		case FIELD_EHANDLE:
			if (pTest->fieldSize > MAX_ENTITYARRAY)
			{
				ALERT(at_error, "Can't save more than %d entities in an array!!!\n", MAX_ENTITYARRAY);
			}

			for (int j = 0; j < pTest->fieldSize; j++)
			{
				switch (pTest->fieldType)
				{
				case FIELD_EVARS:
					entityArray[j] = EntityIndex(static_cast<entvars_t* const*>(pOutputData)[j]);
					break;
				case FIELD_CLASSPTR_NOT_SUPPORTED_ANYMORE:
					entityArray[j] = 0;
					break;
				case FIELD_EDICT:
					entityArray[j] = EntityIndex(static_cast<edict_t* const*>(pOutputData)[j]);
					break;
				case FIELD_ENTITY:
					entityArray[j] = EntityIndex(static_cast<const EOFFSET*>(pOutputData)[j]);
					break;
				case FIELD_EHANDLE:
					entityArray[j] = EntityIndex(static_cast<const EHANDLE<CBaseEntity>*>(pOutputData)[j].Get());
					break;
				}
			}
			WriteInt(pTest->fieldName, entityArray, pTest->fieldSize);
			break;
		case FIELD_POSITION_VECTOR:
			WritePositionVector(pTest->fieldName, static_cast<const float*>(pOutputData), pTest->fieldSize);
			break;
		case FIELD_VECTOR:
			WriteVector(pTest->fieldName, static_cast<const float*>(pOutputData), pTest->fieldSize);
			break;

		case FIELD_BOOLEAN:
		case FIELD_INTEGER:
			WriteInt(pTest->fieldName, static_cast<const int*>(pOutputData), pTest->fieldSize);
			break;

		case FIELD_SHORT:
			WriteData(pTest->fieldName, 2 * pTest->fieldSize, (pOutputData));
			break;

		case FIELD_CHARACTER:
			WriteData(pTest->fieldName, pTest->fieldSize, (pOutputData));
			break;

			// For now, just write the address out, we're not going to change memory while doing this yet!
		case FIELD_POINTER:
			WriteInt(pTest->fieldName, static_cast<const int*>(pOutputData), pTest->fieldSize);
			break;

		case FIELD_FUNCTION:
			WriteFunction(pTest->fieldName, static_cast<const int*>(pOutputData), pTest->fieldSize);
			break;
		default:
			ALERT(at_error, "Bad field type\n");
		}
	}

	return 1;
}


void CSave::BufferString(const char* pdata, int len)
{
	char c = 0;

	BufferData(pdata, len);  // Write the string
	BufferData(&c, 1);       // Write a null terminator
}


int CSave::DataEmpty(const void* pdata, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (static_cast<const byte*>(pdata)[i] != 0)
			return 0;
	}
	return 1;
}


void CSave::BufferField(const char* pname, int size, const void* pdata)
{
	BufferHeader(pname, size);
	BufferData(pdata, size);
}


void CSave::BufferHeader(const char* pname, int size)
{
	short hashvalue = TokenHash(pname);
	if (size > 1 << (sizeof(short) * 8))
		ALERT(at_error, "CSave :: BufferHeader() size parameter exceeds 'short'!");
	BufferData(&size, sizeof(short));
	BufferData(&hashvalue, sizeof(short));
}


void CSave::BufferData(const void* pdata, int size)
{
	if (!m_pdata)
		return;

	if (m_pdata->size + size > m_pdata->bufferSize)
	{
		ALERT(at_error, "Save/Restore overflow!");
		m_pdata->size = m_pdata->bufferSize;
		return;
	}

	memcpy(m_pdata->pCurrentData, pdata, size);
	m_pdata->pCurrentData += size;
	m_pdata->size += size;
}



// --------------------------------------------------------------
//
// CRestore
//
// --------------------------------------------------------------

int CRestore::ReadField(void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount, int startField, int size, const char* pName, const void* pData)
{
	int i, j, stringCount, fieldNumber, entityIndex;
	TYPEDESCRIPTION* pTest;
	float time, timeData;
	Vector position;
	edict_t* pent;
	const char* pString;

	time = 0;
	position = Vector(0, 0, 0);

	if (m_pdata)
	{
		time = m_pdata->time;
		if (m_pdata->fUseLandmark)
			position = m_pdata->vecLandmarkOffset;
	}

	for (i = 0; i < fieldCount; i++)
	{
		fieldNumber = (i + startField) % fieldCount;
		pTest = &pFields[fieldNumber];
		if (!_stricmp(pTest->fieldName, pName))
		{
			if (!m_global || !(pTest->flags & FTYPEDESC_GLOBAL))
			{
				for (j = 0; j < pTest->fieldSize; j++)
				{
					void* pOutputData = (static_cast<char*>(pBaseData) + pTest->fieldOffset + (j * gSizes[pTest->fieldType]));
					const void* pInputData = static_cast<const char*>(pData) + j * gSizes[pTest->fieldType];

					switch (pTest->fieldType)
					{
					case FIELD_TIME:
						timeData = *static_cast<const float*>(pInputData);
						// Re-base time variables
						timeData += time;
						*static_cast<float*>(pOutputData) = timeData;
						break;
					case FIELD_FLOAT:
						*static_cast<float*>(pOutputData) = *static_cast<const float*>(pInputData);
						break;
					case FIELD_MODELNAME:
					case FIELD_SOUNDNAME:
					case FIELD_STRING:
						// Skip over j strings
						pString = static_cast<const char*>(pData);
						for (stringCount = 0; stringCount < j; stringCount++)
						{
							while (*pString)
								pString++;
							pString++;
						}
						pInputData = pString;
						if (strlen(static_cast<const char*>(pInputData)) == 0)
						{
							*static_cast<int*>(pOutputData) = 0;
						}
						else
						{
							int string = ALLOC_STRING(static_cast<const char*>(pInputData));
							*static_cast<int*>(pOutputData) = string;

							if (!FStringNull(string) && m_precache)
							{
								if (pTest->fieldType == FIELD_MODELNAME)
									PRECACHE_MODEL(STRING(string));
								else if (pTest->fieldType == FIELD_SOUNDNAME)
									PRECACHE_SOUND(STRING(string));
							}
						}
						break;
					case FIELD_EVARS:
						entityIndex = *static_cast<const int*>(pInputData);
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*static_cast<entvars_t**>(pOutputData) = VARS(pent);
						else
							*static_cast<entvars_t**>(pOutputData) = nullptr;
						break;
					case FIELD_CLASSPTR_NOT_SUPPORTED_ANYMORE:
						*static_cast<CBaseEntity**>(pOutputData) = nullptr;
						break;
					case FIELD_EDICT:
						entityIndex = *static_cast<const int*>(pInputData);
						pent = EntityFromIndex(entityIndex);
						*static_cast<edict_t**>(pOutputData) = pent;
						break;
					case FIELD_EHANDLE:
						// Input and Output sizes are different!
						pOutputData = static_cast<char*>(pOutputData) + j * (sizeof(EHANDLE<CBaseEntity>) - gSizes[pTest->fieldType]);
						entityIndex = *static_cast<const int*>(pInputData);
						pent = EntityFromIndex(entityIndex);
						*static_cast<EHANDLE<CBaseEntity>*>(pOutputData) = CBaseEntity::SafeInstance<CBaseEntity>(pent);
						break;
					case FIELD_ENTITY:
						entityIndex = *static_cast<const int*>(pInputData);
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*static_cast<EOFFSET*>(pOutputData) = OFFSET(pent);
						else
							*static_cast<EOFFSET*>(pOutputData) = 0;
						break;
					case FIELD_VECTOR:
						static_cast<float*>(pOutputData)[0] = static_cast<const float*>(pInputData)[0];
						static_cast<float*>(pOutputData)[1] = static_cast<const float*>(pInputData)[1];
						static_cast<float*>(pOutputData)[2] = static_cast<const float*>(pInputData)[2];
						break;
					case FIELD_POSITION_VECTOR:
						static_cast<float*>(pOutputData)[0] = static_cast<const float*>(pInputData)[0] + position.x;
						static_cast<float*>(pOutputData)[1] = static_cast<const float*>(pInputData)[1] + position.y;
						static_cast<float*>(pOutputData)[2] = static_cast<const float*>(pInputData)[2] + position.z;
						break;

					case FIELD_BOOLEAN:
					case FIELD_INTEGER:
						*static_cast<int*>(pOutputData) = *static_cast<const int*>(pInputData);
						break;

					case FIELD_SHORT:
						*static_cast<short*>(pOutputData) = *static_cast<const short*>(pInputData);
						break;

					case FIELD_CHARACTER:
						*static_cast<char*>(pOutputData) = *static_cast<const char*>(pInputData);
						break;

					case FIELD_POINTER:
						*static_cast<int*>(pOutputData) = *static_cast<const int*>(pInputData);
						break;
					case FIELD_FUNCTION:
						if (strlen(static_cast<const char*>(pInputData)) == 0)
							*static_cast<int*>(pOutputData) = 0;
						else
							*static_cast<int*>(pOutputData) = FUNCTION_FROM_NAME(static_cast<const char*>(pInputData));
						break;

					default:
						ALERT(at_error, "Bad field type\n");
					}
				}
			}
#if 0
			else
			{
				ALERT(at_console, "Skipping global field %s\n", pName);
			}
#endif
			return fieldNumber;
		}
	}

	return -1;
}


int CRestore::ReadEntVars(const char* pname, entvars_t* pev)
{
	return ReadFields(pname, pev, gEntvarsDescription, ENTVARS_COUNT);
}


int CRestore::ReadFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	unsigned short i, token;
	int lastField, fileCount;
	HEADER header;

	i = ReadShort();
	ASSERT(i == sizeof(int));  // First entry should be an int

	token = ReadShort();

	// Check the struct name
	if (token != TokenHash(pname))  // Field Set marker
	{
		//		ALERT( at_error, "Expected %s found %s!\n", pname, BufferPointer() );
		BufferRewind(2 * sizeof(short));
		return 0;
	}

	// Skip over the struct name
	fileCount = ReadInt();  // Read field count

	lastField = 0;  // Make searches faster, most data is read/written in the same order

	// Clear out base data
	for (i = 0; i < fieldCount; i++)
	{
		// Don't clear global fields
		if (!m_global || !(pFields[i].flags & FTYPEDESC_GLOBAL))
		{
			memset(static_cast<char*>(pBaseData) + pFields[i].fieldOffset, 0, pFields[i].fieldSize * gSizes[pFields[i].fieldType]);
		}
	}

	for (i = 0; i < fileCount; i++)
	{
		BufferReadHeader(&header);
		lastField = ReadField(pBaseData, pFields, fieldCount, lastField, header.size, m_pdata->pTokens[header.token], header.pData);
		lastField++;
	}

	return 1;
}


void CRestore::BufferReadHeader(HEADER* pheader)
{
	ASSERT(pheader != nullptr);
	pheader->size = ReadShort();      // Read field size
	pheader->token = ReadShort();      // Read field name token
	pheader->pData = BufferPointer();  // Field Data is next
	BufferSkipBytes(pheader->size);    // Advance to next field
}


short CRestore::ReadShort(void)
{
	short tmp = 0;

	BufferReadBytes(&tmp, sizeof(short));

	return tmp;
}

int CRestore::ReadInt(void)
{
	int tmp = 0;

	BufferReadBytes(&tmp, sizeof(int));

	return tmp;
}

int CRestore::ReadNamedInt(const char* pName)
{
	HEADER header;

	BufferReadHeader(&header);
	return reinterpret_cast<int*>(header.pData)[0];
}

const char* CRestore::ReadNamedString(const char* pName)
{
	HEADER header;

	BufferReadHeader(&header);
#ifdef TOKENIZE
	return (m_pdata->pTokens[*reinterpret_cast<short*>(header.pData)]);
#else
	return header.pData;
#endif
}


char* CRestore::BufferPointer(void)
{
	if (!m_pdata)
		return nullptr;

	return m_pdata->pCurrentData;
}

void CRestore::BufferReadBytes(void* pOutput, int size)
{
	ASSERT(m_pdata != nullptr);

	if (!m_pdata || Empty())
		return;

	if ((m_pdata->size + size) > m_pdata->bufferSize)
	{
		ALERT(at_error, "Restore overflow!");
		m_pdata->size = m_pdata->bufferSize;
		return;
	}

	if (pOutput)
		memcpy(pOutput, m_pdata->pCurrentData, size);
	m_pdata->pCurrentData += size;
	m_pdata->size += size;
}


void CRestore::BufferSkipBytes(int bytes)
{
	BufferReadBytes(nullptr, bytes);
}

int CRestore::BufferSkipZString(void)
{
	char* pszSearch;
	int len = 0;

	if (!m_pdata)
		return 0;

	int maxLen = m_pdata->bufferSize - m_pdata->size;

	len = 0;
	pszSearch = m_pdata->pCurrentData;
	while (*pszSearch++ && len < maxLen)
		len++;

	len++;

	BufferSkipBytes(len);

	return len;
}

int CRestore::BufferCheckZString(const char* string)
{
	if (!m_pdata)
		return 0;

	int maxLen = m_pdata->bufferSize - m_pdata->size;
	int len = strlen(string);
	if (len <= maxLen)
	{
		if (!strncmp(string, m_pdata->pCurrentData, len))
			return 1;
	}
	return 0;
}



// The CVAR_GET_* functions are incredibly slow. (O(n))
// They get called repeatedly during a single frame for the same cvars, in a worst case scenario this can lead to O(n²).
// This cache dramatically improves performance.
// VRClearCvarCache() needs to be called once per frame, ideally at the beginning.
// - Max Vollmer, 2020-03-16
namespace
{
	static std::unordered_map<std::string, float> cvarfloatcache;
	static std::unordered_map<std::string, std::unique_ptr<std::string>> cvarstringcache;
	static std::unordered_map<std::string, std::vector<unsigned char>> modelpointercache;
}
void VRClearCvarCache()
{
	cvarfloatcache.clear();
	cvarstringcache.clear();
	modelpointercache.clear();
}

float CVAR_GET_FLOAT(const char* x)
{
	auto& it = cvarfloatcache.find(x);
	if (it != cvarfloatcache.end())
	{
		return it->second;
	}
	else
	{
		float result = g_engfuncs.pfnCVarGetFloat(x);
		cvarfloatcache[x] = result;
		return result;
	}
}

const char* CVAR_GET_STRING(const char* x)
{
	auto& it = cvarstringcache.find(x);
	if (it != cvarstringcache.end() && it->second)
	{
		return it->second->data();
	}
	else
	{
		const char* result = g_engfuncs.pfnCVarGetString(x);
		cvarstringcache[x] = std::make_unique<std::string>(result);
		return result;
	}
}

void* GET_MODEL_PTR(edict_t* pent)
{
	constexpr const int STUDIO_HEADER_LENGTH_OFFSET = 72; // see studiohdr_t in studio.h

	std::string modelName = STRING(pent->v.model);

	auto& it = modelpointercache.find(modelName);
	if (it != modelpointercache.end())
	{
		if (it->second.empty())
			return nullptr;
		else
			return it->second.data();
	}
	else
	{
		void* model = nullptr;
		if (modelName.size() > 4
			&& modelName[0] != '*'
			&& modelName.find(".mdl") == modelName.size() - 4)
		{
			model = (*g_engfuncs.pfnGetModelPtr)(pent);
		}
#ifdef _DEBUG
		else
		{
			ALERT(at_console, "Warning: Tried to get model pointer to non-studio model (%s)\n", modelName.c_str());
		}
#endif
		if (model != nullptr)
		{
			// models live only temporarily (not even surviving during a single frame), so we create a copy here
			auto& modelcopy = modelpointercache[modelName];
			unsigned char* modeldata = static_cast<unsigned char*>(model);
			int length = *reinterpret_cast<int*>(modeldata + STUDIO_HEADER_LENGTH_OFFSET);
			modelcopy.assign(modeldata, modeldata + length);
		}
		return model;
	}
}


// Moved all the entity functions here for proper debugging (no more inline) - Max Vollmer, 2020-03-04

edict_t* ENT(const entvars_t* pev)
{
	if (!pev)
		return nullptr;

	if ((FNullEnt(pev->pContainingEntity) && !FWorldEnt(pev->pContainingEntity)) || &pev->pContainingEntity->v != pev)
	{
		const_cast<entvars_t*>(pev)->pContainingEntity = (*g_engfuncs.pfnFindEntityByVars)(pev);
	}

	if ((FNullEnt(pev->pContainingEntity) && !FWorldEnt(pev->pContainingEntity)) || &pev->pContainingEntity->v != pev)
	{
		return nullptr;
	}

	return pev->pContainingEntity;
}

edict_t* ENT(edict_t* pent)
{
	return pent;
}

edict_t* ENT(EOFFSET eoffset)
{
	return (*g_engfuncs.pfnPEntityOfEntOffset)(eoffset);
}

EOFFSET OFFSET(EOFFSET eoffset)
{
	return eoffset;
}

EOFFSET OFFSET(const edict_t* pent)
{
	if (FNullEnt(pent))
	{
#if _DEBUG
		if (!FWorldEnt(pent))
		{
			ALERT(at_error, "Bad ent in OFFSET()\n");
		}
#endif
		return 0;
	}

	return (*g_engfuncs.pfnEntOffsetOfPEntity)(pent);
}

EOFFSET OFFSET(entvars_t* pev)
{
	if (!pev)
	{
#if _DEBUG
		ALERT(at_error, "Bad pev in OFFSET()\n");
#endif
		return 0;
	}

	return OFFSET(ENT(pev));
}

entvars_t* VARS(entvars_t* pev)
{
	return pev;
}

entvars_t* VARS(edict_t* pent)
{
	if (FNullEnt(pent) && !FWorldEnt(pent))
		return nullptr;

	return &pent->v;
}

entvars_t* VARS(EOFFSET eoffset)
{
	return VARS(ENT(eoffset));
}

int ENTINDEX(edict_t* pEdict)
{
	if (FNullEnt(pEdict))
		return 0;

	return (*g_engfuncs.pfnIndexOfEdict)(pEdict);
}

edict_t* INDEXENT(int iEdictNum)
{
	return (*g_engfuncs.pfnPEntityOfEntIndex)(iEdictNum);
}

BOOL FNullEnt(EOFFSET eoffset)
{
	return eoffset == eoNullEntity;
}

BOOL FNullEnt(const edict_t* pent)
{
	if (pent == nullptr)
		return TRUE;

#ifdef _DEBUG
	// entities are stored in an array of length 3072
	// check that pointer is pointing into that array, and isn't misaligned
	edict_t* pentstart = INDEXENT(0);
	unsigned long long pentstartaddress = reinterpret_cast<unsigned long long>(pentstart);
	unsigned long long pentaddress = reinterpret_cast<unsigned long long>(pent);
	if (pentaddress < pentstartaddress)
	{
		ASSERT(!"edict pointer before edict array");
		return TRUE;
	}
	unsigned long long offset = pentaddress - pentstartaddress;
	if (offset > (MAX_EDICTS - 1) * sizeof(edict_t))
	{
		ASSERT(!"edict pointer after edict array");
		return TRUE;
	}
	if ((offset % sizeof(edict_t)) != 0)
	{
		ASSERT(!"edict pointer is misaligned");
		return TRUE;
	}
#endif

	if (pent->free)
		return TRUE;

	if (pent->v.pContainingEntity != pent)
		return TRUE;

	if ((*g_engfuncs.pfnEntOffsetOfPEntity)(pent) == eoNullEntity)
		return TRUE;

	return FALSE;
}

BOOL FNullEnt(entvars_t* pev)
{
	return pev == nullptr || FNullEnt(ENT(pev));
}

BOOL FWorldEnt(const edict_t* pent)
{
	return pent != nullptr && !pent->free && pent->v.pContainingEntity == pent && (*g_engfuncs.pfnEntOffsetOfPEntity)(pent) == eoNullEntity;
}

BOOL FWorldEnt(entvars_t* pev)
{
	return pev != nullptr && FWorldEnt(ENT(pev));
}
