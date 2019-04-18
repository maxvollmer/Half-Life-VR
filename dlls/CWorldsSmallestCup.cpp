

#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"
#include "activity.h"
#include "VRPhysicsHelper.h"
#include "VRModelHelper.h"
#include "CWorldsSmallestCup.h"

LINK_ENTITY_TO_CLASS(vr_easteregg, CWorldsSmallestCup);


void CWorldsSmallestCup::Precache()
{
	PRECACHE_MODEL("models/cup.mdl");
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
	}
	else
	{
		VRPhysicsHelper::Instance().SetWorldsSmallestCupPosition(this);
	}

	pev->absmin = pev->origin + pev->mins;
	pev->absmax = pev->origin + pev->maxs;

	// if we fall out of the world, respawn where the player is
	if (!IsInWorld())
	{
		auto* pPlayer = UTIL_PlayerByIndex(0);
		if (pPlayer != nullptr)
		{
			pev->origin = pPlayer->pev->origin;
		}
	}
}

EHANDLE CWorldsSmallestCup::m_instance{};
