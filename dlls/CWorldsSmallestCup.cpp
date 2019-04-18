

#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"
#include "activity.h"
#include "VRPhysicsHelper.h"
#include "CWorldsSmallestCup.h"

TYPEDESCRIPTION	CWorldsSmallestCup::m_SaveData[] =
{
	DEFINE_FIELD(CWorldsSmallestCup, m_isBeingDragged, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CWorldsSmallestCup, CBaseEntity);

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

	m_instance = this;

	if (m_isBeingDragged)
	{
		VRPhysicsHelper::Instance().SetWorldsSmallestCupPosition(this);
	}
	else
	{
		VRPhysicsHelper::Instance().GetWorldsSmallestCupPosition(this);
	}
}

EHANDLE CWorldsSmallestCup::m_instance{};
