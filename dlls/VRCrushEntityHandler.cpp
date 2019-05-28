
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
		WHEN_MOVING_DOWN
	};

	// TODO: Fill these with actual data
	const std::unordered_map<std::string, std::unordered_map<std::string, CrushMode>> m_entityCrushModes{
		{
			{
				"mapname",
				{{
					{ "modelname", CrushMode::NEVER },
					{ "modelname", CrushMode::NEVER },
					{ "modelname", CrushMode::NEVER },
				}}
			},
			{
				"mapname",
				{{
					{ "modelname", CrushMode::NEVER },
					{ "modelname", CrushMode::NEVER },
					{ "modelname", CrushMode::NEVER },
				}}
			}
		}
	};

	CrushMode GetCrushModeFromEntity(CBaseEntity* pEntity);

public:
	void CheckEntAndMaybeCrushPlayer(CBaseEntity* pEntity, CBaseEntity* pPlayer);
};

VRCrushEntityHandler gVRCrushEntityHandler;

// called by pm_shared.cpp when we are stuck on an entity (pm_shared will unstuck us, but we use this here to determine if we should get killed)
void VRNotifyStuckEnt(int player, int ent)
{
	if (ent <= 0)
		return;

	CBaseEntity* pPlayer = UTIL_PlayerByIndex(player);
	if (pPlayer == nullptr)
		return;

	CBaseEntity* pEntity = nullptr;
	INDEXENT(ent);
	edict_t *pent = INDEXENT(ent);
	if (pent && !pent->free && !FNullEnt(pent))
	{
		pEntity = CBaseEntity::Instance(pent);
	}
	if (pEntity == nullptr)
		return;

	gVRCrushEntityHandler.CheckEntAndMaybeCrushPlayer(pEntity, pPlayer);
}

VRCrushEntityHandler::CrushMode VRCrushEntityHandler::GetCrushModeFromEntity(CBaseEntity* pEntity)
{
	if (!pEntity->pev->model)
		return CrushMode::NEVER;

	std::string modelname = STRING(pEntity->pev->model);

	if (modelname.empty() || modelname[0] != '*')
		return CrushMode::NEVER;

	extern const model_t* GetWorldBSPModel();
	const model_t* mapmodel = GetWorldBSPModel();

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

void VRCrushEntityHandler::CheckEntAndMaybeCrushPlayer(CBaseEntity* pEntity, CBaseEntity* pPlayer)
{
	// already dead
	if (pPlayer->pev->deadflag != DEAD_NO)
		return;

	CrushMode crushMode = GetCrushModeFromEntity(pEntity);

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

	case CrushMode::NEVER:
	default:
		crush = false;
		break;
	}

	if (crush)
	{
		pPlayer->TakeDamage(pEntity->pev, pEntity->pev, pPlayer->pev->health + pPlayer->pev->armorvalue + pEntity->pev->dmg + 1000.f, DMG_CRUSH | DMG_ALWAYSGIB);
	}
}
