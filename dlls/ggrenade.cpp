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

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "player.h"


//===================grenade


LINK_ENTITY_TO_CLASS(grenade, CGrenade);

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE 0x0001

//
// Grenade Explode
//
void CGrenade::Explode(Vector vecSrc, Vector vecAim)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	float flRndSound = 0.f;  // sound randomizer

	pev->model = iStringNull;  //invisible
	pev->solid = SOLID_NOT;    // intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iContents = UTIL_PointContents(pev->origin);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);    // This makes a dynamic light and the explosion sprites/sound
	WRITE_COORD(pev->origin.x);  // Send to PAS because of the sound
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE((pev->dmg - 50) * .60);  // scale * 10
	WRITE_BYTE(15);                     // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
	entvars_t* pevOwner;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = nullptr;

	pev->owner = nullptr;  // can't traceline attack owner if this is set

	RadiusDamage(pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType);

	if (RANDOM_FLOAT(0, 1) < 0.5)
	{
		UTIL_DecalTrace(pTrace, DECAL_SCORCH1);
	}
	else
	{
		UTIL_DecalTrace(pTrace, DECAL_SCORCH2);
	}

	flRndSound = RANDOM_FLOAT(0, 1);

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM); break;
	}

	pev->effects |= EF_NODRAW;
	SetThink(&CGrenade::Smoke);
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0, 3);
		for (int i = 0; i < sparkCount; i++)
		{
			CBaseEntity::Create<CBaseEntity>("spark_shower", pev->origin, pTrace->vecPlaneNormal, nullptr);
		}
	}

	SetTouch(nullptr);
}


void CGrenade::Smoke(void)
{
	if (UTIL_PointContents(pev->origin) == CONTENTS_WATER)
	{
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);
	}
	else
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE((pev->dmg - 50) * 0.80);  // scale * 10
		WRITE_BYTE(12);                      // framerate
		MESSAGE_END();
	}
	UTIL_Remove(this);
}

void CGrenade::Killed(entvars_t* pevAttacker, int bitsDamageType, int iGib)
{
	Detonate();
}


// Timed grenade, this think is called when time runs out.
void CGrenade::DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CGrenade::Detonate);
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate(void)
{
	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 400, 0.3);

	SetThink(&CGrenade::Detonate);
	pev->nextthink = gpGlobals->time + 1;
}


void CGrenade::Detonate(void)
{
	TraceResult tr;
	Vector vecSpot;  // trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}


//
// Contact grenade, explode when it touches something
//
void CGrenade::ExplodeTouch(CBaseEntity* pOther)
{
	TraceResult tr;
	Vector vecSpot;  // trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}


void CGrenade::DangerSoundThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length(), 0.2);
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}
}


void CGrenade::BounceTouch(CBaseEntity* pOther)
{
	// don't hit the entity that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		entvars_t* pevOwner = VARS(pev->owner);
		if (pevOwner)
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB);
			ApplyMultiDamage(pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0;  // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity.
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity;
	vecTestVelocity.z *= 0.45;

	if (!m_fRegisteredSound && vecTestVelocity.Length() <= 60)
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving.
		// go ahead and emit the danger sound.

		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, pev->dmg / 0.4, 0.3);
		m_fRegisteredSound = TRUE;
	}

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG(1, 1);
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;
}



void CGrenade::SlideTouch(CBaseEntity* pOther)
{
	// don't hit the entity that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;

		if (pev->velocity.x != 0 || pev->velocity.y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade::BounceSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM); break;
	}
}

void CGrenade::TumbleThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CGrenade::Detonate);
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


void CGrenade::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING("grenade");

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;

	m_bloodColor = DONT_BLEED;
}


CGrenade* CGrenade::ShootContact(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = GetClassPtr<CGrenade>(nullptr);
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5;  // lower gravity since grenade is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	// make monsters afaid of it while in the air
	pGrenade->SetThink(&CGrenade::DangerSoundThink);
	pGrenade->pev->nextthink = gpGlobals->time;

	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT(-100, -500);

	// Explode on contact
	pGrenade->SetTouch(&CGrenade::ExplodeTouch);

	pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;

	return pGrenade;
}


CGrenade* CGrenade::ShootTimed(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float time)
{
	CGrenade* pGrenade = GetClassPtr<CGrenade>(nullptr);
	pGrenade->Spawn();
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->SetTouch(&CGrenade::BounceTouch);  // Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed().

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink(&CGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1.0;

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	SET_MODEL(ENT(pGrenade->pev), "models/w_grenade.mdl");
	pGrenade->pev->dmg = 100;

	return pGrenade;
}




void CGrenade::UseSatchelCharges(entvars_t* pevOwner, SATCHELCODE code)
{
	edict_t* pentFind;
	edict_t* pentOwner;

	if (!pevOwner)
		return;

	CBaseEntity* pOwner = CBaseEntity::SafeInstance<CBaseEntity>(pevOwner);
	if (!pOwner)
		return;

	pentOwner = pOwner->edict();

	pentFind = FIND_ENTITY_BY_CLASSNAME(nullptr, "grenade");
	while (!FNullEnt(pentFind))
	{
		CBaseEntity* pEnt = CBaseEntity::SafeInstance<CBaseEntity>(pentFind);
		if (pEnt)
		{
			if (FBitSet(pEnt->pev->spawnflags, SF_DETONATE) && pEnt->pev->owner == pentOwner)
			{
				if (code == SATCHEL_DETONATE)
					pEnt->Use(pOwner, pOwner, USE_ON, 0);
				else  // SATCHEL_RELEASE
					pEnt->pev->owner = nullptr;
			}
		}
		pentFind = FIND_ENTITY_BY_CLASSNAME(pentFind, "grenade");
	}
}


bool CGrenade::IsDraggable()
{
	if (pev->dmgtime <= gpGlobals->time)
		return false;

	if (FStringNull(pev->model))
		return false;

	if (strcmp(STRING(pev->model), "models/w_satchel.mdl") == 0)
	{
		// satchel, can be picked up and placed somewhere else
		return true;
	}
	else if (strcmp(STRING(pev->model), "models/w_grenade.mdl") == 0)
	{
		// normal grenade, can be picked up and thrown (will explode in hand if timer runs out)
		return true;
	}
	else if (strcmp(STRING(pev->model), "models/w_squeak.mdl") == 0)
	{
		// snark, can be picked up as ammo
		return true;
	}

	// 9mmAR grenade or unknown grenade type or exploded, can't be grabbed
	return false;
}

void CGrenade::HandleDragStart()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);

	if (strcmp(STRING(pev->model), "models/w_squeak.mdl") == 0)
	{
		// snark, can be picked up as ammo

		EHANDLE<CBasePlayer> hDragger = m_vrDragger;
		if (hDragger)
		{
			// kill this snark
			pev->effects = EF_NODRAW;
			pev->flags = 0;
			pev->takedamage = DAMAGE_NO;
			pev->deadflag = DEAD_DEAD;

			UTIL_Remove(this);

			// add snark item to player
			extern int gEvilImpulse101;
			int backup = gEvilImpulse101;
			gEvilImpulse101 = TRUE;
			hDragger->GiveNamedItem("weapon_snark");
			gEvilImpulse101 = backup;

			// switch to snark item
			EHANDLE<CBasePlayerItem> hItem = hDragger->m_rgpPlayerItems[5];
			while (hItem)
			{
				if (FClassnameIs(hItem->pev, "weapon_snark"))
				{
					hDragger->SwitchWeapon(hItem);
					break;
				}
				hItem = hItem->m_pNext;
			}
		}

		m_backupTouch = nullptr;
		m_backupThink = nullptr;

		SetTouch(nullptr);
		SetThink(nullptr);
	}
	else
	{
		m_backupTouch = m_pfnTouch;
		m_backupThink = m_pfnThink;

		SetTouch(nullptr);
		SetThink(nullptr);
	}
}

void CGrenade::HandleDragStop()
{
	if (strcmp(STRING(pev->model), "models/w_squeak.mdl") == 0)
		return;

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_BOUNCE;
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(m_backupTouch);
	SetThink(m_backupThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CGrenade::HandleDragUpdate(const Vector& origin, const Vector& velocity, const Vector& angles)
{
	if (strcmp(STRING(pev->model), "models/w_squeak.mdl") == 0)
		return;

	CBaseEntity::HandleDragUpdate(origin, velocity, angles);
}

void CGrenade::BaseBalled(CBaseEntity* pPlayer, const Vector& velocity)
{
	// only play baseball if we are a draggable grenade
	if (IsDraggable())
		pev->velocity = velocity;
}


//======================end grenade
