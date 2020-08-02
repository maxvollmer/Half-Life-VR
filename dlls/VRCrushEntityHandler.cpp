
// Methods for handling entities that should crush the player (since getting killed when stuck is disabled due to bugs with VR mode)

#include <unordered_map>
#include <string>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "com_model.h"

class VRCrushEntityHandler
{
private:
	// Determines the crush mode of an entity
	enum class CrushMode
	{
		NEVER,
		ALWAYS,
		WHEN_MOVING,
		WHEN_MOVING_DOWN,
		WHEN_MOVING_UP,
		HEALING_EXPLOIT
	};

	const std::unordered_map<std::string, std::unordered_map<std::string, CrushMode>> m_entityCrushModes{
		{
			{"maps/c1a0c.bsp",
			 {{
				 {"*63", CrushMode::WHEN_MOVING}  // toppling over server rack
			 }}},
			{"maps/c1a1c.bsp",
			 {{
				 {"*147", CrushMode::WHEN_MOVING},  // under water grinder thingies
				 {"*148", CrushMode::WHEN_MOVING},
				 {"*31", CrushMode::WHEN_MOVING},
				 {"*32", CrushMode::WHEN_MOVING},
			 }}},
			{"maps/c1a1f.bsp",
			 {{
				 {"*47", CrushMode::WHEN_MOVING}  // toppling over server rack
			 }}},
			{"maps/c1a2.bsp",
			 {{
				 {"*76", CrushMode::WHEN_MOVING}  // that one evil ventilator
			 }}},
			{"maps/c1a3.bsp",
			 {{
				 {"*18", CrushMode::NEVER}  // ventilator in we got hostiles
			 }}},
			{"maps/c1a3c.bsp",
			 {{
				 {"*1", CrushMode::NEVER}  // ventilator in we got hostiles
			 }}},
			{"maps/c1a4e.bsp",
			 {{
				 {"*1", CrushMode::WHEN_MOVING}  // that huge ventilator near the tentacles
			 }}},
			{"maps/c2a1.bsp",
			 {{
				 {"*97", CrushMode::WHEN_MOVING}  // cave-in near gargantua before on a rail
			 }}},
			{"maps/c2a3.bsp",
			 {{
				 {"*89", CrushMode::WHEN_MOVING}  // cave-in under water after on a rail
			 }}},
			{"maps/c2a3b.bsp",
			 {{
				 {"*4", CrushMode::NEVER},  // stompy thingies where that spit monster is
				 {"*8", CrushMode::NEVER},  // these are the bottom ones and should not kill
				 {"*9", CrushMode::NEVER},
				 {"*10", CrushMode::WHEN_MOVING_DOWN},  // these are the top ones and should kill when moving down
				 {"*11", CrushMode::WHEN_MOVING_DOWN},
				 {"*12", CrushMode::WHEN_MOVING_DOWN},
			 }}},
			{"maps/c2a3e.bsp",
			 {{{"*6", CrushMode::WHEN_MOVING},  // the trash compactor walls
			   {"*13", CrushMode::WHEN_MOVING}}}},
			{"maps/c2a4b.bsp",
			 {{
				 {"*3", CrushMode::NEVER},        // underwater ventilator players need to swim through
				 {"*6", CrushMode::WHEN_MOVING},  // underwater grinder thingy players must avoid
				 {"*9", CrushMode::WHEN_MOVING},  // underwater grinder thingy players must avoid
				 {"*16", CrushMode::NEVER},       // underwater grinder thingy players must swim under

				 {"*19", CrushMode::WHEN_MOVING_DOWN},  // stompy piston thingies on conveyor belts
				 {"*20", CrushMode::WHEN_MOVING_DOWN},
				 {"*21", CrushMode::WHEN_MOVING_DOWN},
				 {"*22", CrushMode::WHEN_MOVING_DOWN},
				 {"*23", CrushMode::WHEN_MOVING_DOWN},
				 {"*24", CrushMode::WHEN_MOVING_DOWN},
			 }}},
			{"maps/c2a4c.bsp",
			 {{
				 {"*4", CrushMode::WHEN_MOVING},  // the sideways murder thingies in residue processing
				 {"*5", CrushMode::WHEN_MOVING},
				 {"*6", CrushMode::WHEN_MOVING},

				 {"*2", CrushMode::WHEN_MOVING_DOWN},  // the downwards stompy murder thingies in residue processing
				 {"*3", CrushMode::WHEN_MOVING_DOWN},
				 {"*46", CrushMode::WHEN_MOVING_DOWN},

				 {"*29", CrushMode::WHEN_MOVING},  // the rolling murder thingies in residue processing
				 {"*30", CrushMode::WHEN_MOVING},
				 {"*31", CrushMode::WHEN_MOVING},

				 {"*38", CrushMode::WHEN_MOVING},  // the crushy-bity murder thingies at the end of residue processing
				 {"*39", CrushMode::WHEN_MOVING},
			 }}},
			{"maps/c2a4e.bsp",
			 {{{"*54", CrushMode::WHEN_MOVING},  // that super weird automated murder equipment in questionable ethics
			   {"*56", CrushMode::WHEN_MOVING}}}},
			{"maps/c2a5.bsp",
			 {{{"*133", CrushMode::WHEN_MOVING},  // underwater ventilators in the canyon dam (need to be shut off from control tower)
			   {"*134", CrushMode::WHEN_MOVING}}}},
			{"maps/c2a5f.bsp",
			 {{{"*139", CrushMode::HEALING_EXPLOIT},  // the healing exploit doors
			   {"*144", CrushMode::HEALING_EXPLOIT}}}},
			{"maps/c2a5g.bsp",
			 {{{"*95", CrushMode::WHEN_MOVING},  // the cars being kicked by gargantua in that underground parking lot
			   {"*93", CrushMode::WHEN_MOVING},
			   {"*96", CrushMode::WHEN_MOVING}}}},
			{"maps/c3a1.bsp",
			 {{
				 {"*8", CrushMode::WHEN_MOVING},  // cave-in ceiling pieces
				 {"*9", CrushMode::WHEN_MOVING},
				 {"*10", CrushMode::WHEN_MOVING},
				 {"*11", CrushMode::WHEN_MOVING},
				 {"*12", CrushMode::WHEN_MOVING},
				 {"*13", CrushMode::WHEN_MOVING},
				 {"*14", CrushMode::WHEN_MOVING},
				 {"*27", CrushMode::WHEN_MOVING},
			 }}},
			{"maps/c3a1b.bsp",
			 {{
				 {"*47", CrushMode::WHEN_MOVING},  // collapsing walls in that alien vs military warzone
				 {"*48", CrushMode::WHEN_MOVING},
				 {"*49", CrushMode::WHEN_MOVING},
			 }}},
		} };

	CrushMode GetCrushModeFromEntity(CBaseEntity* pEntity);

public:
	bool CheckEntAndMaybeCrushPlayer(CBaseEntity* pEntity, CBaseEntity* pPlayer);
};

VRCrushEntityHandler gVRCrushEntityHandler;

// called by pm_shared.cpp when we are stuck on an entity (pm_shared will unstuck us, but we use this here to determine if we should get killed)
bool VRNotifyStuckEnt(int player, int ent)
{
	if (ent <= 0)
		return false;

	CBaseEntity* pPlayer = UTIL_PlayerByIndex(player + 1);
	if (pPlayer == nullptr)
		return false;

	CBaseEntity* pEntity = CBaseEntity::SafeInstance<CBaseEntity>(INDEXENT(ent));
	if (pEntity == nullptr)
		return false;

	return gVRCrushEntityHandler.CheckEntAndMaybeCrushPlayer(pEntity, pPlayer);
}

VRCrushEntityHandler::CrushMode VRCrushEntityHandler::GetCrushModeFromEntity(CBaseEntity* pEntity)
{
	if (!pEntity->pev->model)
		return CrushMode::NEVER;

	std::string modelname = STRING(pEntity->pev->model);

	if (modelname.empty() || modelname[0] != '*')
		return CrushMode::NEVER;

	extern const model_t* VRGetBSPModel(CBaseEntity * pEntity);
	const model_t* mapmodel = VRGetBSPModel(CWorld::InstanceOrWorld(ENT(0)));

	if (mapmodel == nullptr || mapmodel->needload)
		return CrushMode::NEVER;

	auto mapEntityCrushModes = m_entityCrushModes.find(mapmodel->name);
	if (mapEntityCrushModes != m_entityCrushModes.end())
	{
		auto entityCrushMode = mapEntityCrushModes->second.find(modelname);
		if (entityCrushMode != mapEntityCrushModes->second.end())
		{
			return entityCrushMode->second;
		}
	}

	return CrushMode::NEVER;
}

bool VRCrushEntityHandler::CheckEntAndMaybeCrushPlayer(CBaseEntity* pEntity, CBaseEntity* pPlayer)
{
	// already dead
	if (pPlayer->pev->deadflag != DEAD_NO)
		return true;  // dont unstuck

	CrushMode crushMode = GetCrushModeFromEntity(pEntity);

	// healing bug entities
	if (crushMode == CrushMode::HEALING_EXPLOIT &&
		CVAR_GET_FLOAT("sv_cheats") != 0.f &&
		CVAR_GET_FLOAT("vr_cheat_enable_healing_exploit") != 0.f &&
		pEntity->pev->dmg < 0.f)
	{
		pPlayer->TakeDamage(pEntity->pev, pEntity->pev, pEntity->pev->dmg, DMG_CRUSH);
		return true;  // dont unstuck
	}

	bool crush = false;

	switch (crushMode)
	{
	case CrushMode::ALWAYS:
		crush = true;
		break;

	case CrushMode::WHEN_MOVING:
		crush = pEntity->pev->velocity.LengthSquared() > 0.f || pEntity->pev->avelocity.LengthSquared() > 0.f;
		break;

	case CrushMode::WHEN_MOVING_DOWN:
		crush = pEntity->pev->velocity.z < 0.f;
		break;

	case CrushMode::WHEN_MOVING_UP:
		crush = pEntity->pev->velocity.z > 0.f;
		break;

	case CrushMode::NEVER:
	default:
		crush = false;
		break;
	}

	if (crush)
	{
		pPlayer->TakeDamage(pEntity->pev, pEntity->pev, pPlayer->pev->health + pPlayer->pev->armorvalue + pEntity->pev->dmg + 1000.f, DMG_CRUSH | DMG_ALWAYSGIB);
		return true;  // dont unstuck
	}

	return false;
}
