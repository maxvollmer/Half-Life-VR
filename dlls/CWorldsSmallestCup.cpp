
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


EHANDLE<CWorldsSmallestCup> CWorldsSmallestCup::m_instance{};
std::string CWorldsSmallestCup::m_lastMap;
bool CWorldsSmallestCup::m_wasDraggedLastMap{ false };

TYPEDESCRIPTION CWorldsSmallestCup::m_SaveData[] =
{
	DEFINE_FIELD(CWorldsSmallestCup, m_spawnOrigin, FIELD_POSITION_VECTOR)
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

	SetThink(&CWorldsSmallestCup::CupThink);
	pev->nextthink = gpGlobals->time;
	pev->flags |= FL_ALWAYSTHINK;

	if (m_instance && m_instance != this)
	{
		m_hAlreadySpokenKleiners.insert(m_instance->m_hAlreadySpokenKleiners.begin(), m_instance->m_hAlreadySpokenKleiners.end());
		m_instance->SetThink(&CBaseEntity::SUB_Remove);
	}
	m_instance = this;
}

void CWorldsSmallestCup::HandleDragStart()
{
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

	if (m_instance && m_instance != this)
	{
		m_instance->m_hAlreadySpokenKleiners.insert(m_hAlreadySpokenKleiners.begin(), m_hAlreadySpokenKleiners.end());
		SetThink(&CBaseEntity::SUB_Remove);
		return;
	}

	m_instance = this;

	m_lastMap = STRING(INDEXENT(0)->v.model);

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
					CTalkMonster::g_talkWaitTime = gpGlobals->time + 20;
					m_hKleiner->Talk(20);
					m_hKleiner->m_hTalkTarget = UTIL_PlayerByIndex(0);
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

void CWorldsSmallestCup::EnsureInstance(CBasePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	if (!m_instance)
	{
		bool spawnnew = false;
		CBaseEntity* pEasterEgg = UTIL_FindEntityByClassname(nullptr, "vr_easteregg");
		if (pEasterEgg)
		{
			CWorldsSmallestCup* pWorldsSmallestCup = dynamic_cast<CWorldsSmallestCup*>(pEasterEgg);
			if (pWorldsSmallestCup)
			{
				m_instance = pWorldsSmallestCup;
			}
			else
			{
				UTIL_Remove(pEasterEgg);
				spawnnew = true;
			}
		}
		else
		{
			// If player carried cup last map, spawn a new one.
			spawnnew = CWorldsSmallestCup::m_wasDraggedLastMap;
			CWorldsSmallestCup::m_wasDraggedLastMap = false;
		}
		if (spawnnew)
		{
			edict_t* pentinstance = CREATE_NAMED_ENTITY(MAKE_STRING("vr_easteregg"));
			if (!FNullEnt(pentinstance))
			{
				pentinstance->v.origin = pPlayer->pev->origin;
				DispatchSpawn(pentinstance);
			}
		}
		return;
	}

	if (m_instance->m_spawnOrigin.LengthSquared() == 0.f)
	{
		m_instance->m_spawnOrigin = pPlayer->pev->origin;
	}

	// if easteregg is being dragged, all is well
	if (m_instance->IsBeingDragged())
	{
		EHANDLE<CBasePlayer> hPlayer = m_instance->m_vrDragger;
		if (hPlayer)
		{
			auto& controller = hPlayer->GetController(m_instance->m_vrDragController);
			if (controller.IsValid() && controller.IsDragging() && controller.IsDraggedEntity(m_instance))
			{
				return;
			}
		}
	}

	// ensure easteregg thinks
	m_instance->m_vrDragger = nullptr;
	m_instance->m_vrDragController = VRControllerID::INVALID;
	m_instance->SetThink(&CWorldsSmallestCup::CupThink);
	m_instance->pev->nextthink = gpGlobals->time;
	m_instance->pev->flags |= FL_ALWAYSTHINK;
}

void EnsureEasteregg(CBasePlayer* pPlayer)
{
	CWorldsSmallestCup::EnsureInstance(pPlayer);
}
