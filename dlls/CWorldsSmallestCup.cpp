

#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"
#include "activity.h"
#include "talkmonster.h"
#include "VRPhysicsHelper.h"
#include "VRModelHelper.h"
#include "CWorldsSmallestCup.h"

LINK_ENTITY_TO_CLASS(vr_easteregg, CWorldsSmallestCup);


void CWorldsSmallestCup::Precache()
{
	PRECACHE_MODEL("models/cup.mdl");
	PRECACHE_SOUND("easteregg/smallestcup.wav");
}

void CWorldsSmallestCup::Spawn()
{
	Precache();
	pev->targetname = pev->classname;
	pev->model = MAKE_STRING("models/cup.mdl");
	SET_MODEL(ENT(pev), STRING(pev->model));
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);
	VRPhysicsHelper::Instance().SetWorldsSmallestCupPosition(this);
	SetThink(&CWorldsSmallestCup::CupThink);
	pev->nextthink = gpGlobals->time + 0.1f;
	if (m_instance && m_instance != this)
	{
		m_instance->SetThink(&CBaseEntity::SUB_Remove);
	}
	m_instance = this;
}

void CWorldsSmallestCup::CupThink()
{
	pev->nextthink = gpGlobals->time;
	pev->flags |= FL_ALWAYSTHINK;

	if (m_instance && m_instance != this)
	{
		((CWorldsSmallestCup*)(CBaseEntity*)m_instance)->m_hAlreadySpokenKleiners.insert(m_hAlreadySpokenKleiners.begin(), m_hAlreadySpokenKleiners.end());
		SetThink(&CBaseEntity::SUB_Remove);
		return;
	}

	if (pev->size.LengthSquared() == 0.f)
	{
		const auto& modelInfo = VRModelHelper::GetInstance().GetModelInfo(this);
		pev->mins = modelInfo.m_sequences[0].bboxMins;
		pev->maxs = modelInfo.m_sequences[0].bboxMaxs;
		pev->size = pev->maxs - pev->mins;
	}

	m_instance = this;

	if (m_isBeingDragged.empty())
	{
		VRPhysicsHelper::Instance().GetWorldsSmallestCupPosition(this);

		// if we fall out of the world, respawn where the player is
		if (!IsInWorld() || IsFallingOutOfWorld())
		{
			CBaseEntity* pPlayer = UTIL_PlayerByIndex(0);
			if (pPlayer)
			{
				pev->origin = pPlayer->pev->origin;
				VRPhysicsHelper::Instance().SetWorldsSmallestCupPosition(this);
			}
		}
	}
	else
	{
		VRPhysicsHelper::Instance().SetWorldsSmallestCupPosition(this);
	}

	pev->absmin = pev->origin + pev->mins;
	pev->absmax = pev->origin + pev->maxs;

	// check if we are in front of a kleiner
	if (!m_isBeingDragged.empty())
	{
		if (m_hKleiner && AmIInKleinersFace(m_hKleiner))
		{
			if ((gpGlobals->time - m_flKleinerFaceStart) > 0.5f)
			{
				CTalkMonster* pKleiner = (CTalkMonster*)(CBaseEntity*)m_hKleiner;
				if (pKleiner->FOkToSpeak())
				{
					CTalkMonster::g_talkWaitTime = gpGlobals->time + 20;
					pKleiner->Talk(20);
					pKleiner->m_hTalkTarget = UTIL_PlayerByIndex(0);
					EMIT_SOUND_DYN(pKleiner->edict(), CHAN_VOICE, "easteregg/smallestcup.wav", 1, ATTN_NORM, 0, pKleiner->GetVoicePitch());
					SetBits(pKleiner->m_bitsSaid, bit_saidHelloPlayer);
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
				if (FClassnameIs(pKleiner->pev, "monster_scientist") && !pKleiner->IsFemaleNPC() && pKleiner->pev->body == 0 /*HEAD_GLASSES*/
					&& m_hAlreadySpokenKleiners.count(EHANDLE<CBaseEntity>{ pKleiner }) == 0 && AmIInKleinersFace(pKleiner))
				{
					m_hKleiner = pKleiner;
					m_flKleinerFaceStart = gpGlobals->time;
					break;
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
	if (!m_isBeingDragged.empty())
		return false;

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector{ 0.f, 0.f, -2048.f }, ignore_monsters, nullptr, &tr);
	return tr.flFraction == 1.f;
}

bool CWorldsSmallestCup::AmIInKleinersFace(CBaseEntity* pKleiner)
{
	if (!pKleiner)
		return false;

	Vector forward;
	UTIL_MakeAimVectorsPrivate(pKleiner->pev->angles, forward, nullptr, nullptr);
	Vector pos = pKleiner->EyePosition() + (forward * 16.f);
	return UTIL_PointInsideBBox(pev->origin, pos - Vector{ 8.f, 8.f, 8.f }, pos + Vector{ 8.f, 8.f, 8.f });
}

EHANDLE<CBaseEntity> CWorldsSmallestCup::m_instance{};
