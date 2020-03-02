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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"
#include "gamerules.h"

#define TRIPMINE_PRIMARY_VOLUME 450



enum tripmine_e
{
	TRIPMINE_IDLE1 = 0,
	TRIPMINE_IDLE2,
	TRIPMINE_ARM1,
	TRIPMINE_ARM2,
	TRIPMINE_FIDGET,
	TRIPMINE_HOLSTER,
	TRIPMINE_DRAW,
	TRIPMINE_WORLD,
	TRIPMINE_GROUND,
};


#ifndef CLIENT_DLL

class CTripmineGrenade : public CGrenade
{
	void Spawn(void);
	void Precache(void);

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void EXPORT WarningThink(void);
	void EXPORT PowerupThink(void);
	void EXPORT BeamBreakThink(void);
	void EXPORT DelayDeathThink(void);
	void Killed(entvars_t* pevAttacker, int bitsDamageType, int iGib);

	void MakeBeam(void);
	void KillBeam(void);

	float m_flPowerUp = 0.f;
	Vector m_vecDir;
	Vector m_vecEnd;
	float m_flBeamLength = 0.f;

	EHANDLE<CBaseEntity> m_hOwner;
	EHANDLE<CBaseEntity> m_hBeam;
	Vector m_posOwner;
	Vector m_angleOwner;
	edict_t* m_pRealOwner = nullptr;  // tracelines don't hit PEV->OWNER, which means a player couldn't detonate their own trip mine, so we store the owner here.
};

LINK_ENTITY_TO_CLASS(monster_tripmine, CTripmineGrenade);

TYPEDESCRIPTION CTripmineGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CTripmineGrenade, m_flPowerUp, FIELD_TIME),
	DEFINE_FIELD(CTripmineGrenade, m_vecDir, FIELD_VECTOR),
	DEFINE_FIELD(CTripmineGrenade, m_vecEnd, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CTripmineGrenade, m_flBeamLength, FIELD_FLOAT),
	DEFINE_FIELD(CTripmineGrenade, m_hOwner, FIELD_EHANDLE),
	DEFINE_FIELD(CTripmineGrenade, m_hBeam, FIELD_EHANDLE),
	DEFINE_FIELD(CTripmineGrenade, m_posOwner, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CTripmineGrenade, m_angleOwner, FIELD_VECTOR),
	DEFINE_FIELD(CTripmineGrenade, m_pRealOwner, FIELD_EDICT),
};

IMPLEMENT_SAVERESTORE(CTripmineGrenade, CGrenade);


void CTripmineGrenade::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT(pev), "models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_WORLD;
	ResetSequenceInfo();
	pev->framerate = 0;

	UTIL_MakeAimVectors(pev->angles);

	m_vecDir = gpGlobals->v_forward;
	m_vecEnd = pev->origin + m_vecDir * 2048;

	UTIL_SetSize(pev, Vector(-8, -8, 0), Vector(8, 8, 16));

	// Spawned by map, "fall" onto wall
	if (!pev->owner)
	{
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin - m_vecDir * 64.f, dont_ignore_monsters, edict(), &tr);
		if (!tr.fAllSolid && tr.flFraction > 0.f && !tr.fStartSolid && FNullEnt(tr.pHit))
		{
			pev->origin = tr.vecEndPos;
		}
	}

	UTIL_SetOrigin(pev, pev->origin);

	if (pev->spawnflags & 1)
	{
		// power up quickly
		m_flPowerUp = gpGlobals->time + 1.0;
	}
	else
	{
		// power up in 2.5 seconds
		m_flPowerUp = gpGlobals->time + 2.5;
	}

	SetThink(&CTripmineGrenade::PowerupThink);
	pev->nextthink = gpGlobals->time + 0.2;

	pev->takedamage = DAMAGE_YES;
	pev->dmg = gSkillData.plrDmgTripmine;
	pev->health = 1;  // don't let die normally

	if (pev->owner != nullptr)
	{
		// play deploy sound
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav", 1.0, ATTN_NORM);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/mine_charge.wav", 0.2, ATTN_NORM);  // chargeup

		m_pRealOwner = pev->owner;  // see CTripmineGrenade for why.
	}
}


void CTripmineGrenade::Precache(void)
{
	PRECACHE_MODEL("models/v_tripmine.mdl");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("weapons/mine_activate.wav");
	PRECACHE_SOUND("weapons/mine_charge.wav");
}


void CTripmineGrenade::WarningThink(void)
{
	// play warning sound
	// EMIT_SOUND( ENT(pev), CHAN_VOICE, "buttons/Blip2.wav", 1.0, ATTN_NORM );

	// set to power up
	SetThink(&CTripmineGrenade::PowerupThink);
	pev->nextthink = gpGlobals->time + 1.0;
}


void CTripmineGrenade::PowerupThink(void)
{
	TraceResult tr;

	if (m_hOwner == nullptr)
	{
		// find an owner
		edict_t* oldowner = pev->owner;
		pev->owner = nullptr;
		UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 32, dont_ignore_monsters, ENT(pev), &tr);
		if (tr.fStartSolid || (oldowner && tr.pHit == oldowner))
		{
			pev->owner = oldowner;
			m_flPowerUp += 0.1;
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
		if (tr.flFraction < 1.0)
		{
			pev->owner = tr.pHit;
			m_hOwner = CBaseEntity::SafeInstance<CBaseEntity>(pev->owner);
			m_posOwner = m_hOwner->pev->origin;
			m_angleOwner = m_hOwner->pev->angles;
		}
		else
		{
			STOP_SOUND(ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav");
			STOP_SOUND(ENT(pev), CHAN_BODY, "weapons/mine_charge.wav");
			SetThink(&CBaseEntity::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1;
			ALERT(at_console, "WARNING:Tripmine at %.0f, %.0f, %.0f removed\n", pev->origin.x, pev->origin.y, pev->origin.z);
			KillBeam();
			return;
		}
	}
	else if (m_posOwner != m_hOwner->pev->origin || m_angleOwner != m_hOwner->pev->angles)
	{
		// disable
		STOP_SOUND(ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav");
		STOP_SOUND(ENT(pev), CHAN_BODY, "weapons/mine_charge.wav");
		CTripmine* pMine = CBaseEntity::Create<CTripmine>("weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles);
		pMine->pev->spawnflags |= SF_NORESPAWN;

		SetThink(&CBaseEntity::SUB_Remove);
		KillBeam();
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	// ALERT( at_console, "%d %.0f %.0f %0.f\n", pev->owner, m_pOwner->pev->origin.x, m_pOwner->pev->origin.y, m_pOwner->pev->origin.z );

	if (gpGlobals->time > m_flPowerUp)
	{
		// make solid
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin(pev, pev->origin);

		MakeBeam();

		// play enabled sound
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/mine_activate.wav", 0.5, ATTN_NORM, 1.0, 75);
	}
	pev->nextthink = gpGlobals->time + 0.1;
}


void CTripmineGrenade::KillBeam(void)
{
	if (m_hBeam)
	{
		UTIL_Remove(m_hBeam);
		m_hBeam = nullptr;
	}
}


void CTripmineGrenade::MakeBeam(void)
{
	Vector vecBeamStart = pev->origin + m_vecDir * 2.f;

	TraceResult tr;
	UTIL_TraceLine(vecBeamStart, m_vecEnd, dont_ignore_monsters, ENT(pev), &tr);

	m_flBeamLength = tr.flFraction;

	CBeam* pBeam = CBeam::BeamCreate(g_pModelNameLaser, 6);
	pBeam->PointsInit(vecBeamStart, tr.vecEndPos);
	pBeam->SetColor(0, 214, 198);
	pBeam->SetScrollRate(255);
	pBeam->SetBrightness(64);
	m_hBeam = pBeam;

	SetThink(&CTripmineGrenade::BeamBreakThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


void CTripmineGrenade::BeamBreakThink(void)
{
	BOOL bBlowup = 0;

	Vector vecBeamStart = pev->origin + m_vecDir * 2.f;

	TraceResult tr;
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine(vecBeamStart, m_vecEnd, dont_ignore_monsters, ENT(pev), &tr);

	// ALERT( at_console, "%f : %f\n", tr.flFraction, m_flBeamLength );

	// respawn detect.
	if (!m_hBeam)
	{
		MakeBeam();
		if (tr.pHit)
			m_hOwner = CBaseEntity::SafeInstance<CBaseEntity>(tr.pHit);  // reset owner too
	}

	if (fabs(m_flBeamLength - tr.flFraction) > 0.001)
	{
		bBlowup = 1;
	}
	else
	{
		if (m_hOwner == nullptr)
			bBlowup = 1;
		else if (m_posOwner != m_hOwner->pev->origin)
			bBlowup = 1;
		else if (m_angleOwner != m_hOwner->pev->angles)
			bBlowup = 1;
	}

	if (bBlowup)
	{
		// a bit of a hack, but all CGrenade code passes pev->owner along to make sure the proper player gets credit for the kill
		// so we have to restore pev->owner from pRealOwner, because an entity's tracelines don't strike it's pev->owner which meant
		// that a player couldn't trigger their own tripmine. Now that the mine is exploding, it's safe to restore the owner so the
		// CGrenade code knows who the explosive really belongs to.
		pev->owner = m_pRealOwner;
		pev->health = 0;
		Killed(VARS(pev->owner), DMG_GENERIC, GIB_NORMAL);
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

int CTripmineGrenade::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (gpGlobals->time < m_flPowerUp && flDamage < pev->health)
	{
		// disable
		// Create( "weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles );
		SetThink(&CTripmineGrenade::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1;
		KillBeam();
		return FALSE;
	}
	return CGrenade::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CTripmineGrenade::Killed(entvars_t* pevAttacker, int bitsDamageType, int iGib)
{
	pev->takedamage = DAMAGE_NO;

	if (pevAttacker && (pevAttacker->flags & FL_CLIENT))
	{
		// some client has destroyed this mine, they'll get credit for any kills
		pev->owner = ENT(pevAttacker);
	}

	SetThink(&CTripmineGrenade::DelayDeathThink);
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.1, 0.3);

	EMIT_SOUND(ENT(pev), CHAN_BODY, "common/null.wav", 0.5, ATTN_NORM);  // shut off chargeup
}


void CTripmineGrenade::DelayDeathThink(void)
{
	KillBeam();
	TraceResult tr;
	UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 64, dont_ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}
#endif

LINK_ENTITY_TO_CLASS(weapon_tripmine, CTripmine);

void CTripmine::Spawn()
{
	Precache();
	m_iId = WEAPON_TRIPMINE;
	SET_MODEL(ENT(pev), "models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_GROUND;
	// ResetSequenceInfo( );
	pev->framerate = 0;

	// CBasePlayerWeapon::KeyValue sets scale for all weapons to 0.7,
	// but tripmine model was fixed already, so reset to 1 here.
	// TODO: Once all weapon models are fixed, remove this code.
	pev->scale = 1.f;

	pev->dmg = gSkillData.plrDmgTripmine;

	FallInit();  // get ready to fall down

	m_iDefaultAmmo = TRIPMINE_DEFAULT_GIVE;

#ifdef CLIENT_DLL
	if (!bIsMultiplayer())
#else
	if (!g_pGameRules->IsDeathmatch())
#endif
	{
		UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 28));
	}
}

void CTripmine::Precache(void)
{
	PRECACHE_MODEL("models/v_tripmine.mdl");
	PRECACHE_MODEL("models/p_tripmine.mdl");
	UTIL_PrecacheOther("monster_tripmine");

	m_usTripFire = PRECACHE_EVENT(1, "events/tripfire.sc");
}

int CTripmine::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Trip Mine";
	p->iMaxAmmo1 = TRIPMINE_MAX_CARRY;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_TRIPMINE;
	p->iWeight = TRIPMINE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CTripmine::Deploy()
{
	//pev->body = 0;
	return DefaultDeploy("models/v_tripmine.mdl", "models/p_tripmine.mdl", TRIPMINE_DRAW, "trip");
}


void CTripmine::Holster(int skiplocal /* = 0 */)
{
#ifndef CLIENT_DLL
	if (m_hGhost)
	{
		m_hGhost->pev->effects |= EF_NODRAW;
	}
#endif

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		// out of mines
		m_pPlayer->pev->weapons &= ~(1 << WEAPON_TRIPMINE);
		SetThink(&CTripmine::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	SendWeaponAnim(TRIPMINE_HOLSTER);
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

constexpr const float VR_TRIPMINE_PLACEMENT_DISTANCE = 32.f;

void CTripmine::PrimaryAttack(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	int flags = 0;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usTripFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

#ifndef CLIENT_DLL
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector();  //gpGlobals->v_forward;

	TraceResult tr;

	float distance = 0.f;
	if (CVAR_GET_FLOAT("vr_weapon_grenade_mode") != 0.f)
	{
		distance = 128.f;
	}
	else
	{
		distance = VR_TRIPMINE_PLACEMENT_DISTANCE;
	}

	UTIL_TraceLine(vecSrc, vecSrc + (vecAiming * distance), dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	if (tr.flFraction < 1.0)
	{
		CBaseEntity* pEntity = CBaseEntity::SafeInstance<CBaseEntity>(tr.pHit);
		if (pEntity && !(pEntity->pev->flags & FL_CONVEYOR))
		{
			Vector origin = tr.vecEndPos;
			Vector angles = UTIL_VecToAngles(tr.vecPlaneNormal);
			CBaseEntity* pEnt = CBaseEntity::Create<CBaseEntity>("monster_tripmine", origin, angles, m_pPlayer->edict());

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

			if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			{
				// no more mines!
				RetireWeapon();
				return;
			}
		}
	}
#endif

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

#ifdef CLIENT_DLL
	VRRegisterRecoil(0.1f);
#endif
}

#ifndef CLIENT_DLL
void CTripmine::UpdateGhost()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		if (m_hGhost)
		{
			UTIL_Remove(m_hGhost);
		}
		m_hGhost = nullptr;
		return;
	}

	float distance = 0.f;
	if (CVAR_GET_FLOAT("vr_weapon_grenade_mode") != 0.f)
	{
		distance = 128.f;
	}
	else
	{
		distance = VR_TRIPMINE_PLACEMENT_DISTANCE;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector();

	TraceResult tr;

	UTIL_TraceLine(vecSrc, vecSrc + (vecAiming * distance), dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	bool hideGhost = true;
	if (tr.flFraction < 1.0)
	{
		CBaseEntity* pEntity = CBaseEntity::SafeInstance<CBaseEntity>(tr.pHit);
		if (pEntity && !(pEntity->pev->flags & FL_CONVEYOR))
		{
			Vector origin = tr.vecEndPos;
			if (!m_hGhost)
			{
				m_hGhost = CSprite::SpriteCreate("models/v_tripmine.mdl", origin, FALSE);
				m_hGhost->pev->spawnflags |= SF_SPRITE_TEMPORARY | SF_SPRITE_STARTON;
				m_hGhost->pev->rendermode = kRenderTransTexture;
				m_hGhost->pev->renderamt = 50;
				m_hGhost->pev->body = 3;
				m_hGhost->pev->sequence = TRIPMINE_WORLD;
			}
			m_hGhost->pev->angles = UTIL_VecToAngles(tr.vecPlaneNormal);
			m_hGhost->pev->origin = origin;
			m_hGhost->pev->effects &= ~EF_NODRAW;
			hideGhost = false;
		}
	}

	if (m_hGhost && hideGhost)
	{
		m_hGhost->pev->effects |= EF_NODRAW;
	}
}
#endif

void CTripmine::WeaponIdle(void)
{
#ifndef CLIENT_DLL
	UpdateGhost();
#endif

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		SendWeaponAnim(TRIPMINE_DRAW);
	}
	else
	{
		RetireWeapon();
		return;
	}

	int iAnim = 0;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.25)
	{
		iAnim = TRIPMINE_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
	}
	else if (flRand <= 0.75)
	{
		iAnim = TRIPMINE_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 30.0;
	}
	else
	{
		iAnim = TRIPMINE_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 100.0 / 30.0;
	}

	SendWeaponAnim(iAnim);
}
