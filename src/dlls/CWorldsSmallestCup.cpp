
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"
#include "activity.h"
#include "talkmonster.h"
#include "func_break.h"
#include "com_model.h"

#include "VRPhysicsHelper.h"
#include "VRModelHelper.h"

#include "CWorldsSmallestCup.h"

LINK_ENTITY_TO_CLASS(vr_easteregg, CWorldsSmallestCup);

extern const model_t* VRGetBSPModel(edict_t* pent);

TYPEDESCRIPTION CWorldsSmallestCup::m_SaveData[] =
{
	DEFINE_FIELD(CWorldsSmallestCup, m_spawnOrigin, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CWorldsSmallestCup, m_lastMap, FIELD_STRING),
	DEFINE_FIELD(CWorldsSmallestCup, m_wasDraggedLastMap, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CWorldsSmallestCup, CGib);

void CWorldsSmallestCup::Spawn()
{
	m_spawnOrigin = pev->origin;

	CGib::Spawn("models/cup.mdl");

	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	pev->targetname = pev->classname = MAKE_STRING("vr_easteregg");

	m_material = matWood;
	m_bloodColor = DONT_BLEED;
	m_cBloodDecals = 0;

	m_hAlreadySpokenKleiners.clear();
	m_wasDraggedLastMap = false;

	SetThink(&CWorldsSmallestCup::CupThink);
	pev->nextthink = gpGlobals->time;
	pev->flags |= FL_ALWAYSTHINK;
}

void CWorldsSmallestCup::HandleDragStart()
{
	UTIL_VRGiveAchievement(m_vrDragger, VRAchievement::AM_SMALLCUP);

	pev->flags &= ~FL_ALWAYSTHINK;
}

void CWorldsSmallestCup::HandleDragStop()
{
	SetThink(&CWorldsSmallestCup::CupThink);
	pev->nextthink = gpGlobals->time;
	pev->flags |= FL_ALWAYSTHINK;
}

void CWorldsSmallestCup::HandleDragUpdate(const Vector& origin, const Vector& velocity, const Vector& angles)
{
	CBaseEntity::HandleDragUpdate(origin, velocity, angles);
	CupThink();
}

void CWorldsSmallestCup::CupThink()
{
	pev->nextthink = gpGlobals->time;
	pev->flags |= FL_ALWAYSTHINK;

	if (!FStrEq(STRING(m_lastMap), STRING(INDEXENT(0)->v.model)))
	{
		m_lastMap = INDEXENT(0)->v.model;
		m_hAlreadySpokenKleiners.clear();
		m_wasDraggedLastMap = false;
	}

	if (IsBeingDragged())
	{
		m_wasDraggedLastMap = true;
	}
	else
	{
		// if we fall out of the world, respawn
		if (!IsInWorld() || IsFallingOutOfWorld())
		{
			pev->origin = m_spawnOrigin;
		}
	}

	// check if we are in front of a kleiner
	if (IsBeingDragged())
	{
		if (m_hKleiner && AmIInKleinersFace(m_hKleiner))
		{
			if ((gpGlobals->time - m_flKleinerFaceStart) > 0.5f)
			{
				if (m_hKleiner->FOkToSpeak())
				{
					UTIL_VRGiveAchievement(m_vrDragger, VRAchievement::HID_SMALLESTCUP);

					CTalkMonster::g_talkWaitTime = gpGlobals->time + 20;
					m_hKleiner->Talk(20);
					m_hKleiner->m_hTalkTarget = UTIL_PlayerByIndex(1);
					EMIT_SOUND_DYN(m_hKleiner->edict(), CHAN_VOICE, "easteregg/smallestcup.wav", 1, ATTN_NORM, 0, m_hKleiner->GetVoicePitch());
					SetBits(m_hKleiner->m_bitsSaid, bit_saidHelloPlayer);
					m_hAlreadySpokenKleiners.insert(m_hKleiner);
					m_hKleiner = nullptr;
					m_flKleinerFaceStart = 0.f;
				}
			}
		}
		else
		{
			m_hKleiner = nullptr;
			m_flKleinerFaceStart = 0.f;
			CBaseEntity* pKleiner = nullptr;
			while (pKleiner = UTIL_FindEntityInSphere(pKleiner, pev->origin, 256.f))
			{
				if (FClassnameIs(pKleiner->pev, "monster_scientist") && !pKleiner->IsFemaleNPC() && pKleiner->pev->body == 0 /*HEAD_GLASSES*/)
				{
					EHANDLE<CTalkMonster> hKleiner = dynamic_cast<CTalkMonster*>(pKleiner);
					if (hKleiner && m_hAlreadySpokenKleiners.count(hKleiner) == 0 && AmIInKleinersFace(hKleiner))
					{
						m_hKleiner = hKleiner;
						m_flKleinerFaceStart = gpGlobals->time;
						break;
					}
				}
			}
		}
	}
	else
	{
		m_hKleiner = nullptr;
		m_flKleinerFaceStart = 0.f;
	}
}

bool CWorldsSmallestCup::IsFallingOutOfWorld()
{
	if (IsBeingDragged())
		return false;

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector{ 0.f, 0.f, -1024.f }, ignore_monsters, nullptr, &tr);
	return tr.fAllSolid || tr.flFraction < 0.f || tr.flFraction >= 1.f;
}

bool CWorldsSmallestCup::AmIInKleinersFace(CTalkMonster* pKleiner)
{
	if (!pKleiner)
		return false;

	Vector forward;
	UTIL_MakeAimVectorsPrivate(pKleiner->pev->angles, forward, nullptr, nullptr);
	Vector pos = pKleiner->EyePosition() + (forward * 16.f);
	return UTIL_PointInsideBBox(pev->origin, pos - Vector{ 8.f, 8.f, 8.f }, pos + Vector{ 8.f, 8.f, 8.f });
}
