
#include "VRAchievementsAndStatsTracker.h"

#include <string.h>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

namespace
{
	constexpr const int MAX_MAPS_NAME_LENGTH = 16;

	constexpr const int HAS_TRIPPED_MINE_OR_LASER		= 1 << 0;
	constexpr const int HAS_USED_TRAIN_IN_ON_A_RAIL		= 1 << 1;
	constexpr const int HAS_SAVED_DOOMED_SCIENTIST		= 1 << 2;
	constexpr const int HAS_USED_NOCLIP					= 1 << 3;
	constexpr const int HAS_BEEN_IN_WGH_MAPS			= 1 << 4;
	constexpr const int HAS_BEEN_IN_PU_MAPS				= 1 << 5;
	constexpr const int HAS_BEEN_IN_ON_A_RAIL			= 1 << 6;
	constexpr const int HAS_BEEN_IN_BLASTPIT			= 1 << 7;
	constexpr const int HAS_ALERTED_TENTACLES			= 1 << 8;
	constexpr const int HAS_KILLED_ANY					= 1 << 9;
	constexpr const int HAS_KILLED_FRIENDS				= 1 << 10;
	constexpr const int HAS_SURFACED_IN_THAT_MAP_WITH_THAT_TANK		 = 1 << 11;
}

void CVRAchievementsAndStatsData::FriendlyKilled()
{
	m_bitFlags |= HAS_KILLED_FRIENDS;
	AnyKilled();
}

void CVRAchievementsAndStatsData::AnyKilled()
{
	m_bitFlags |= HAS_KILLED_ANY;
}


void VRAchievementsAndStatsTracker::PlayerTakeNegativeCrushDamage(CBaseEntity* pPlayer, float dmg)
{
	CBasePlayer* pActualPlayer = dynamic_cast<CBasePlayer*>(pPlayer);
	if (!pActualPlayer)
		return;

	pActualPlayer->GetAchievementsAndStatsData().m_totalNegativeCrushDamage += dmg;

	if (pActualPlayer->GetAchievementsAndStatsData().m_totalNegativeCrushDamage < -1000.f)
	{
		UTIL_VRGiveAchievement(pActualPlayer, VRAchievement::HID_NOTCHEATING);
	}
}

void CheckRigorousResearchAchievement(CBasePlayer* pPlayer)
{
	if (pPlayer->GetAchievementsAndStatsData().m_exp1HeadcrabsKilled >= 5 && pPlayer->GetAchievementsAndStatsData().m_exp2HeadcrabsKilled >= 7)
	{
		UTIL_VRGiveAchievementAll(VRAchievement::QE_RIGOROUS);
	}
}

bool CheckQuestionableEthicsHumanKilled(CBasePlayer* pPlayer, struct entvars_s* pKilled)
{
	if (FClassnameIs(pKilled, "monster_scientist") || FClassnameIs(pKilled, "monster_barney"))
	{
		pPlayer->GetAchievementsAndStatsData().FriendlyKilled();
		UTIL_VRGiveAchievementAll(VRAchievement::QE_ATALLCOSTS);
		return true;
	}
	else if (FClassnameIs(pKilled, "monster_human_grunt"))
	{
		pPlayer->GetAchievementsAndStatsData().AnyKilled();
		UTIL_VRGiveAchievementAll(VRAchievement::QE_EFFECTIVE);
		return true;
	}

	return false;
}

void CheckQuestionableEthicsKills(CBasePlayer* pPlayer, struct entvars_s* pKiller, struct entvars_s* pKilled)
{
	// First questionable ethics experiment
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a4d.bsp"))
	{
		// first questionable ethics experiment
		if (FStrEq(STRING(pKiller->targetname), "ster1_hurt"))
		{
			if (!CheckQuestionableEthicsHumanKilled(pPlayer, pKilled))
			{
				if (FClassnameIs(pKilled, "monster_headcrab"))
				{
					pPlayer->GetAchievementsAndStatsData().m_exp1HeadcrabsKilled++;
					CheckRigorousResearchAchievement(pPlayer);
				}
			}
			pPlayer->GetAchievementsAndStatsData().AnyKilled();
		}
	}
	// 2nd questionable ethics experiments
	else if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a4e.bsp"))
	{
		if (FStrEq(STRING(pKiller->targetname), "ster1_hurt"))
		{
			if (!CheckQuestionableEthicsHumanKilled(pPlayer, pKilled))
			{
				if (FClassnameIs(pKilled, "monster_headcrab"))
				{
					pPlayer->GetAchievementsAndStatsData().m_exp2HeadcrabsKilled++;
					CheckRigorousResearchAchievement(pPlayer);
				}
			}
			pPlayer->GetAchievementsAndStatsData().AnyKilled();
		}
	}
}

void VRAchievementsAndStatsTracker::SmthKilledSmth(struct entvars_s* pKiller, struct entvars_s* pKilled, int bitsDamageType)
{
	if (!pKiller || !pKilled)
		return;

	CBasePlayer* pHostPlayer = UTIL_PlayerByIndex(1);

	if (FClassnameIs(pKilled, "monster_gargantua"))
	{
		// Power Up gargantua
		if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a1.bsp")
			&& FClassnameIs(pKiller, "env_laser")
			&& FStrEq(STRING(pKiller->targetname), "gargbeams"))
		{
			UTIL_VRGiveAchievementAll(VRAchievement::PU_BBQ);
			pHostPlayer->GetAchievementsAndStatsData().AnyKilled();
			return;
		}
		// Forget About Freeman gargantua
		else if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a5g.bsp")
			&& FBitSet(bitsDamageType, DMG_MORTAR))
		{
			UTIL_VRGiveAchievementAll(VRAchievement::FAF_FIREINHOLE);
			pHostPlayer->GetAchievementsAndStatsData().AnyKilled();
			return;
		}
	}

	if (FClassnameIs(pKiller, "trigger_hurt"))
	{
		CheckQuestionableEthicsKills(pHostPlayer, pKiller, pKilled);
		return;
	}

	// Those weird rotating tools in questionable ethics
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a4e.bsp")
		&& FClassnameIs(pKiller, "func_rotating")
		&& FStrEq(STRING(pKiller->targetname), "psychobot"))
	{
		if (FClassnameIs(pKilled, "monster_scientist"))
		{
			UTIL_VRGiveAchievementAll(VRAchievement::QE_PRECISURGERY);
			pHostPlayer->GetAchievementsAndStatsData().FriendlyKilled();
		}
		else if (!FClassnameIs(pKilled, "monster_scientist_dead"))
		{
			pHostPlayer->GetAchievementsAndStatsData().AnyKilled();
		}
		return;
	}

	CBasePlayer* pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(pKiller->pContainingEntity);
	if (pPlayer && pPlayer->IsNetClient())
	{
		// monster_snark is special case, CSqueakGrenade inherits from CGrenade, not from CBaseMonster
		if (FStrEq(STRING(pKilled->classname), "monster_snark"))
		{
			PlayerKilledSmth(pPlayer, false);
			return;
		}

		// make sure it's a monster
		CBaseMonster* pMonster = CBaseEntity::SafeInstance<CBaseMonster>(pKilled->pContainingEntity);
		if (!pMonster)
		{
			return;
		}

		// don't count enemies that must be killed to progress the story (tentacles, gonarch (bigmomma), nihilanth)
		if (FStrEq(STRING(pKilled->classname), "monster_tentacle")
			|| FStrEq(STRING(pKilled->classname), "monster_bigmomma")
			|| FStrEq(STRING(pKilled->classname), "monster_nihilanth")
			|| FStrEq(STRING(pKilled->classname), "monster_cockroach"))
		{
			return;
		}

		// don't count cockroaches, bc it's impossible not to step on one in some maps
		if (FStrEq(STRING(pKilled->classname), "monster_cockroach"))
		{
			return;
		}

		// friendly fire is true if killed monster is a barney or a scientist
		PlayerKilledSmth(pPlayer, FStrEq(STRING(pKilled->classname), "monster_barney") || FStrEq(STRING(pKilled->classname), "monster_scientist"));
	}
}

void VRAchievementsAndStatsTracker::PlayerKilledSmth(CBasePlayer* pPlayer, bool friendlyFire)
{
	if (friendlyFire)
	{
		pPlayer->GetAchievementsAndStatsData().FriendlyKilled();
	}
	else
	{
		pPlayer->GetAchievementsAndStatsData().AnyKilled();
	}
}

void VRAchievementsAndStatsTracker::PlayerTrippedMineOrLaser(CBasePlayer* pPlayer)
{
	pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_TRIPPED_MINE_OR_LASER;
}

namespace
{
	// TODO: at some point i really need to consolidate all these hardcoded mapnames all over the place into one class that's responsible for mapnames
	std::unordered_set<std::string> onARailMaps = {
		"maps/c2a2.bsp",
		"maps/c2a2a.bsp",
		"maps/c2a2b1.bsp",
		"maps/c2a2b2.bsp",
		"maps/c2a2c.bsp",
		"maps/c2a2d.bsp",
		"maps/c2a2e.bsp",
		"maps/c2a2f.bsp",
		"maps/c2a2g.bsp",
		"maps/c2a2h.bsp"
	};
}

void VRAchievementsAndStatsTracker::PlayerUsedTrain(CBasePlayer* pPlayer)
{
	if (onARailMaps.find(STRING(INDEXENT(0)->v.model)) != onARailMaps.end())
	{
		pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_USED_TRAIN_IN_ON_A_RAIL;
	}

	UTIL_VRGiveAchievement(pPlayer, VRAchievement::OAR_CHOOCHOO);
}

void VRAchievementsAndStatsTracker::PlayerUsedNoclip(CBasePlayer* pPlayer)
{
	pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_USED_NOCLIP;
}

void VRAchievementsAndStatsTracker::PlayerSavedDoomedScientist(CBaseEntity* pPlayer)
{
	CBasePlayer* pActualPlayer = dynamic_cast<CBasePlayer*>(pPlayer);
	if (!pActualPlayer)
		return;

	UTIL_VRGiveAchievement(pActualPlayer, VRAchievement::WGH_NOTDOOMED);

	pActualPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_SAVED_DOOMED_SCIENTIST;
}

void VRAchievementsAndStatsTracker::PlayerSurfacedInThatMapWithTheTank(CBasePlayer* pPlayer)
{
	// ignore if already surfaced
	if (FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_SURFACED_IN_THAT_MAP_WITH_THAT_TANK))
		return;

	// double check that player actually surfaced
	if (pPlayer->pev->origin.z <= 0.f)
		return;

	// double check that the map is actually correct
	if (!FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a5b.bsp"))
		return;

	// remember that the player surfaced
	pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_SURFACED_IN_THAT_MAP_WITH_THAT_TANK;

	// check if the player surfaced behind the tank
	if (pPlayer->pev->origin.y < -976.f)
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::ST_SNEAKY);
	}
}

void VRAchievementsAndStatsTracker::PlayerLaunchedTentacleRocketFire(CBaseEntity* pPlayer)
{
	CBasePlayer* pActualPlayer = dynamic_cast<CBasePlayer*>(pPlayer);

	if (pActualPlayer && !FBitSet(pActualPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_ALERTED_TENTACLES))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::BP_SNEAKY);
	}
}

void VRAchievementsAndStatsTracker::PlayerGotHeardByTentacles()
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(1);

	pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_ALERTED_TENTACLES;
}

void VRAchievementsAndStatsTracker::PlayerDestroyedBlastPitBridge()
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(1);

	pPlayer->GetAchievementsAndStatsData().m_bpBridgesDestroyed++;

	if (pPlayer->GetAchievementsAndStatsData().m_bpBridgesDestroyed == 3)
	{
		UTIL_VRGiveAchievementAll(VRAchievement::BP_BLOWUPBRIDGES);
	}
}

void VRAchievementsAndStatsTracker::PlayerSolvedBlastPitFan()
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(1);

	// must not have used noclip
	if (FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_USED_NOCLIP))
		return;

	// must have activated silofan
	CBaseEntity* pEntity = nullptr;
	while ((pEntity = UTIL_FindEntityByTargetname(pEntity, "silofan")) != nullptr)
	{
		if (FClassnameIs(pEntity->pev, "func_rotating"))
		{
			if (pEntity->pev->avelocity != g_vecZero)
			{
				UTIL_VRGiveAchievementAll(VRAchievement::BP_BADDESIGN);
			}
			break;
		}
	}
}

void VRAchievementsAndStatsTracker::ResidueBarneyStartedRunning()
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(1);

	pPlayer->GetAchievementsAndStatsData().m_residueBarneyStartedRunningTime = gpGlobals->time;
}

void VRAchievementsAndStatsTracker::ResidueBarneyIsAlive()
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(1);

	// check that barney started running, AND that that's at least 5 seconds ago
	if (pPlayer->GetAchievementsAndStatsData().m_residueBarneyStartedRunningTime != 0.f
		&& (gpGlobals->time > (pPlayer->GetAchievementsAndStatsData().m_residueBarneyStartedRunningTime + 5.f)))
	{
		UTIL_VRGiveAchievementAll(VRAchievement::RP_PARENTAL);
		pPlayer->GetAchievementsAndStatsData().m_residueBarneyStartedRunningTime = 0.f;	// clear this, so we won't fire the achievement again
	}
}

void VRAchievementsAndStatsTracker::Update(CBasePlayer* pPlayer)
{
	if (FStringNull(INDEXENT(0)->v.model))
		return;

	if (!FStrEq(pPlayer->GetAchievementsAndStatsData().m_prevMap, STRING(INDEXENT(0)->v.model)))
	{
		GiveAchievementBasedOnMap(pPlayer, STRING(INDEXENT(0)->v.model));
		strncpy_s(pPlayer->GetAchievementsAndStatsData().m_prevMap, STRING(INDEXENT(0)->v.model), 32);
	}
}

void VRAchievementsAndStatsTracker::GiveAchievementBasedOnMap(CBasePlayer* pPlayer, const char* mapname)
{
	ALERT(at_console, "GiveAchievementBasedOnMap: %s\n", mapname);

	// game start (train intro)
	if (FStrEq(mapname, "maps/c0a0.bsp"))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::BMI_STARTTHEGAME);
	}

	// WGH maps
	if (FStrEq(mapname, "maps/c1a3a.bsp")
		|| FStrEq(mapname, "maps/c1a3b.bsp")
		|| FStrEq(mapname, "maps/c1a3c.bsp")
		|| FStrEq(mapname, "maps/c1a3d.bsp"))
	{
		pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_BEEN_IN_WGH_MAPS;
	}

	// PU maps
	if (FStrEq(mapname, "maps/c2a1a.bsp")
		|| FStrEq(mapname, "maps/c2a1b.bsp"))
	{
		pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_BEEN_IN_PU_MAPS;
	}

	// first blast pit map
	if (FStrEq(mapname, "maps/c1a4.bsp"))
	{
		// check if player skipped We've Got Hostiles
		// must have saved the doomed scientist, never used noclip, never visited a WGH map, and come from c1a3
		// (still possible to cheat, but there is only so much you can do)
		if (!FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_BEEN_IN_BLASTPIT)
			&& FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_SAVED_DOOMED_SCIENTIST)
			&& !FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_USED_NOCLIP)
			&& FStrEq(pPlayer->GetAchievementsAndStatsData().m_prevMap, "maps/c1a3.bsp")
			&& !FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_BEEN_IN_WGH_MAPS))
		{
			UTIL_VRGiveAchievement(pPlayer, VRAchievement::HID_SKIP_WGH);
		}
		else
		{
			// only give WGH achievements, if player played the chapter
			if (FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_BEEN_IN_WGH_MAPS)
				&& !FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_TRIPPED_MINE_OR_LASER))
			{
				UTIL_VRGiveAchievement(pPlayer, VRAchievement::WGH_PERFECT);
			}
		}

		pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_BEEN_IN_BLASTPIT;
	}

	// first On A Rail map
	if (FStrEq(mapname, "maps/c2a2.bsp"))
	{
		// check if player skipped Power Up
		// must never have used noclip, never visited a Power Up map, and come from c2a1
		if (!FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_BEEN_IN_ON_A_RAIL)
			&& !FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_USED_NOCLIP)
			&& FStrEq(pPlayer->GetAchievementsAndStatsData().m_prevMap, "maps/c2a1.bsp")
			&& !FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_BEEN_IN_PU_MAPS))
		{
			UTIL_VRGiveAchievement(pPlayer, VRAchievement::HID_SKIP_PU);
		}

		pPlayer->GetAchievementsAndStatsData().m_bitFlags |= HAS_BEEN_IN_ON_A_RAIL;
	}

	// first xen map
	if (FStrEq(mapname, "maps/c4a1.bsp"))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::XEN_VIEW);
	}

	// nihilanth
	if (FStrEq(mapname, "maps/c4a3.bsp"))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::N_WHAT);
	}

	// game end (gman)
	if (FStrEq(mapname, "maps/c5a1.bsp"))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::END_GMAN);

		// basic sanity check that player has actually played the game, and not just used "map c5a1" to get here.
		if (FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_BEEN_IN_ON_A_RAIL)
			&& FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_BEEN_IN_BLASTPIT)
			&& FStrEq(pPlayer->GetAchievementsAndStatsData().m_prevMap, "maps/c4a3.bsp"))
		{
			if (!FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_KILLED_ANY))
			{
				UTIL_VRGiveAchievement(pPlayer, VRAchievement::GEN_PACIFIST);
			}
			if (!FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_KILLED_FRIENDS))
			{
				UTIL_VRGiveAchievement(pPlayer, VRAchievement::GEN_TEAMPLAYER);
			}
		}
	}

	// transitioning from "on a rail" to "apprehension", and never used a train
	if (FStrEq(mapname, "maps/c2a3.bsp")
		&& FStrEq(pPlayer->GetAchievementsAndStatsData().m_prevMap, "maps/c2a2g.bsp")
		&& FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_BEEN_IN_ON_A_RAIL)
		&& !FBitSet(pPlayer->GetAchievementsAndStatsData().m_bitFlags, HAS_USED_TRAIN_IN_ON_A_RAIL))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::HID_OFFARAIL);
	}
}

