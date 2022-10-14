
#include "VRAchievementsAndStatsTracker.h"

#include <string.h>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

// keep track of up to 20 past maps. used for shadow achievements (skipping sections etc)
constexpr const int NUM_PAST_MAPS_TO_TRACK = 20;

class CVRAchievementsAndStatsTracker : public CBaseEntity
{
public:
	virtual int Save(CSave& save) override;
	virtual int Restore(CRestore& restore) override;
	virtual int ObjectCaps(void) override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];

	int m_friendlyKillCount = 0;
	int m_totalKillCount = 0;
	int m_trippedMineOrLaser = 0;
	int m_usedTrainInOnARail = 0;
	int m_SavedDoomedScientist = 0;
	int m_usedNoclip = 0;
	int m_wentIntoWGHMaps = 0;
	int m_wentIntoPUMaps = 0;
	int m_hasSurfacedInThatMapWithTheTank = 0;
	int m_wentIntoOAR = 0;
	int m_wentIntoBlastPit = 0;
	int m_didTentaclesHearPlayer = 0;
	int m_bpBridgesDestroyed = 0;

	float m_negativeCrushDamage = 0.f;

	char m_prevMaps[NUM_PAST_MAPS_TO_TRACK][128] = {0};

	// placeholders to stay save compatible. hopefully these are enough
	int iplaceHolder0 = 0;
	int iplaceHolder1 = 0;
	int iplaceHolder2 = 0;
	int iplaceHolder3 = 0;
	int iplaceHolder4 = 0;
	int iplaceHolder5 = 0;
	int iplaceHolder6 = 0;
	int iplaceHolder7 = 0;
	int iplaceHolder8 = 0;
	int iplaceHolder9 = 0;

	float fplaceHolder0 = 0;
	float fplaceHolder1 = 0;
	float fplaceHolder2 = 0;
	float fplaceHolder3 = 0;
	float fplaceHolder4 = 0;
	float fplaceHolder5 = 0;
	float fplaceHolder6 = 0;
	float fplaceHolder7 = 0;
	float fplaceHolder8 = 0;
	float fplaceHolder9 = 0;

	char splaceHolder0[128] = { 0 };
	char splaceHolder1[128] = { 0 };
	char splaceHolder2[128] = { 0 };
	char splaceHolder3[128] = { 0 };
	char splaceHolder4[128] = { 0 };
	char splaceHolder5[128] = { 0 };
	char splaceHolder6[128] = { 0 };
	char splaceHolder7[128] = { 0 };
	char splaceHolder8[128] = { 0 };
	char splaceHolder9[128] = { 0 };

	void UpdateMap(const char* mapname)
	{
		for (int i = NUM_PAST_MAPS_TO_TRACK - 1; i > 0; i--)
		{
			strncpy_s(m_prevMaps[i], m_prevMaps[i - 1], 128);
		}
		strncpy_s(m_prevMaps[0], mapname, 128);
	}
};

LINK_ENTITY_TO_CLASS(vr_statstracker, CVRAchievementsAndStatsTracker);

TYPEDESCRIPTION CVRAchievementsAndStatsTracker::m_SaveData[] =
{
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_friendlyKillCount, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_totalKillCount, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_trippedMineOrLaser, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_usedTrainInOnARail, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_SavedDoomedScientist, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_usedNoclip, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_wentIntoWGHMaps, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_wentIntoPUMaps, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_hasSurfacedInThatMapWithTheTank, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_wentIntoOAR, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_wentIntoBlastPit, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_didTentaclesHearPlayer, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_bpBridgesDestroyed, FIELD_INTEGER),

	DEFINE_FIELD(CVRAchievementsAndStatsTracker, m_negativeCrushDamage, FIELD_FLOAT),

	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, m_prevMaps, FIELD_CHARACTER, 128 * NUM_PAST_MAPS_TO_TRACK),

	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder0, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder1, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder2, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder3, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder4, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder5, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder6, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder7, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder8, FIELD_INTEGER),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, iplaceHolder9, FIELD_INTEGER),

	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder0, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder1, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder2, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder3, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder4, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder5, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder6, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder7, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder8, FIELD_FLOAT),
	DEFINE_FIELD(CVRAchievementsAndStatsTracker, fplaceHolder9, FIELD_FLOAT),

	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder0, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder1, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder2, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder3, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder4, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder5, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder6, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder7, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder8, FIELD_CHARACTER, 128),
	DEFINE_ARRAY(CVRAchievementsAndStatsTracker, splaceHolder9, FIELD_CHARACTER, 128)
};

IMPLEMENT_SAVERESTORE(CVRAchievementsAndStatsTracker, CBaseEntity);


namespace
{
	EHANDLE<CVRAchievementsAndStatsTracker> hTracker;
	CVRAchievementsAndStatsTracker* GetTracker()
	{
		if (!hTracker)
		{
			hTracker = dynamic_cast<CVRAchievementsAndStatsTracker*>(UTIL_FindEntityByClassname(nullptr, "vr_statstracker"));
			if (!hTracker)
			{
				hTracker = CBaseEntity::Create<CVRAchievementsAndStatsTracker>("vr_statstracker", g_vecZero, g_vecZero);
			}
		}

		return hTracker;
	}
}


void VRAchievementsAndStatsTracker::PlayerTakeNegativeCrushDamage(CBaseEntity* pPlayer, float dmg)
{
	GetTracker()->m_negativeCrushDamage += dmg;

	if (GetTracker()->m_negativeCrushDamage < -1000.f)
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::HID_NOTCHEATING);
	}
}

void VRAchievementsAndStatsTracker::SmthKilledSmth(struct entvars_s* pKiller, struct entvars_s* pKilled, int bitsDamageType)
{
	if (!pKiller || !pKilled)
		return;

	if (FClassnameIs(pKilled, "monster_gargantua"))
	{
		// Power Up gargantua
		if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a1.bsp")
			&& FClassnameIs(pKiller, "trigger_hurt")
			&& FStrEq(STRING(pKiller->targetname), "electro_hurt"))
		{
			UTIL_VRGiveAchievementAll(VRAchievement::PU_BBQ);
			GetTracker()->m_totalKillCount++;
			return;
		}
		// Forget About Freeman gargantua
		else if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a5g.bsp")
			&& FBitSet(bitsDamageType, DMG_MORTAR))
		{
			UTIL_VRGiveAchievementAll(VRAchievement::FAF_FIREINHOLE);
			GetTracker()->m_totalKillCount++;
			return;
		}
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

void VRAchievementsAndStatsTracker::PlayerKilledSmth(CBaseEntity* pPlayer, bool friendlyFire)
{
	GetTracker()->m_totalKillCount++;
	if (friendlyFire)
	{
		GetTracker()->m_friendlyKillCount++;
	}
}

void VRAchievementsAndStatsTracker::PlayerTrippedMineOrLaser(CBaseEntity* pPlayer)
{
	GetTracker()->m_trippedMineOrLaser++;
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

void VRAchievementsAndStatsTracker::PlayerUsedTrain(CBaseEntity* pPlayer)
{
	if (onARailMaps.find(STRING(INDEXENT(0)->v.model)) != onARailMaps.end())
	{
		GetTracker()->m_usedTrainInOnARail++;
	}

	UTIL_VRGiveAchievement(pPlayer, VRAchievement::OAR_CHOOCHOO);
}

void VRAchievementsAndStatsTracker::PlayerUsedNoclip(CBaseEntity* pPlayer)
{
	GetTracker()->m_usedNoclip = 1;
}

void VRAchievementsAndStatsTracker::PlayerSavedDoomedScientist(CBaseEntity* pPlayer)
{
	UTIL_VRGiveAchievement(pPlayer, VRAchievement::WGH_NOTDOOMED);

	GetTracker()->m_SavedDoomedScientist = 1;
}

void VRAchievementsAndStatsTracker::PlayerSurfacedInThatMapWithTheTank(CBaseEntity* pPlayer)
{
	// ignore if already surfaced
	if (GetTracker()->m_hasSurfacedInThatMapWithTheTank)
		return;

	// double check that player actually surfaced
	if (pPlayer->pev->origin.z <= 0.f)
		return;

	// double check that the map is actually correct
	if (!FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a5b.bsp"))
		return;

	// remember that the player surfaced
	GetTracker()->m_hasSurfacedInThatMapWithTheTank = true;

	// check if the player surfaced behind the tank
	if (pPlayer->pev->origin.y < -976.f)
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::ST_SNEAKY);
	}
}

void VRAchievementsAndStatsTracker::PlayerLaunchedTentacleRocketFire(CBaseEntity* pPlayer)
{
	if (!GetTracker()->m_didTentaclesHearPlayer)
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::BP_SNEAKY);
	}
}

void VRAchievementsAndStatsTracker::PlayerGotHeardByTentacles()
{
	GetTracker()->m_didTentaclesHearPlayer = 1;
}

void VRAchievementsAndStatsTracker::PlayerDestroyedBlastPitBridge()
{
	GetTracker()->m_bpBridgesDestroyed++;

	if (GetTracker()->m_bpBridgesDestroyed == 3)
	{
		UTIL_VRGiveAchievementAll(VRAchievement::BP_BLOWUPBRIDGES);
	}
}

void VRAchievementsAndStatsTracker::PlayerSolvedBlastPitFan()
{
	// must not have used noclip
	if (GetTracker()->m_usedNoclip)
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

void VRAchievementsAndStatsTracker::GiveAchievementBasedOnMap(CBaseEntity* pPlayer)
{
	// game start (train intro)
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c0a0.bsp"))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::BMI_STARTTHEGAME);
	}

	// WGH maps
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c1a3a.bsp")
		|| FStrEq(STRING(INDEXENT(0)->v.model), "maps/c1a3b.bsp")
		|| FStrEq(STRING(INDEXENT(0)->v.model), "maps/c1a3c.bsp")
		|| FStrEq(STRING(INDEXENT(0)->v.model), "maps/c1a3d.bsp"))
	{
		GetTracker()->m_wentIntoWGHMaps = 1;
	}

	// PU maps
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a1a.bsp")
		|| FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a1b.bsp"))
	{
		GetTracker()->m_wentIntoPUMaps = 1;
	}

	// first blast pit map
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c1a4.bsp"))
	{
		// check if player skipped We've Got Hostiles
		// must have saved the doomed scientist, never used noclip, never visited a WGH map, and come from c1a3
		// (still possible to cheat, but there is only so much you can do)
		if (!GetTracker()->m_wentIntoBlastPit
			&& GetTracker()->m_SavedDoomedScientist
			&& !GetTracker()->m_usedNoclip
			&& FStrEq(GetTracker()->m_prevMaps[0], "maps/c1a3.bsp")
			&& !GetTracker()->m_wentIntoWGHMaps)
		{
			UTIL_VRGiveAchievement(pPlayer, VRAchievement::HID_SKIP_WGH);
		}
		else
		{
			// only give WGH achievements, if player played the chapter
			if (GetTracker()->m_trippedMineOrLaser == 0)
			{
				UTIL_VRGiveAchievement(pPlayer, VRAchievement::WGH_PERFECT);
			}
		}

		GetTracker()->m_wentIntoBlastPit = 1;
	}

	// first On A Rail map
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a2.bsp"))
	{
		// check if player skipped Power Up
		// must never have used noclip, never visited a Power Up map, and come from c2a1
		if (!GetTracker()->m_wentIntoOAR
			&& !GetTracker()->m_usedNoclip
			&& FStrEq(GetTracker()->m_prevMaps[0], "maps/c2a1.bsp")
			&& !GetTracker()->m_wentIntoPUMaps)
		{
			UTIL_VRGiveAchievement(pPlayer, VRAchievement::HID_SKIP_PU);
		}

		GetTracker()->m_wentIntoOAR = 1;
	}

	// first xen map
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c4a1.bsp"))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::XEN_VIEW);
	}

	// nihilanth
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c4a3.bsp"))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::N_WHAT);
	}

	// game end (gman)
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c5a1.bsp"))
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::END_GMAN);

		// basic sanity check that player has actually played the game, and not just used "map c5a1" to get here.
		if (GetTracker()->m_wentIntoOAR
			&& GetTracker()->m_wentIntoBlastPit
			&& FStrEq(GetTracker()->m_prevMaps[0], "maps/c4a3.bsp"))
		{
			if (GetTracker()->m_totalKillCount == 0)
			{
				UTIL_VRGiveAchievement(pPlayer, VRAchievement::GEN_PACIFIST);
			}
			if (GetTracker()->m_friendlyKillCount == 0)
			{
				UTIL_VRGiveAchievement(pPlayer, VRAchievement::GEN_TEAMPLAYER);
			}
		}
	}

	// transitioning from "on a rail" to "apprehension", and never used a train
	if (FStrEq(STRING(INDEXENT(0)->v.model), "maps/c2a3.bsp")
		&& FStrEq(GetTracker()->m_prevMaps[0], "maps/c2a2g.bsp")
		&& GetTracker()->m_wentIntoOAR
		&& !GetTracker()->m_usedTrainInOnARail)
	{
		UTIL_VRGiveAchievement(pPlayer, VRAchievement::HID_OFFARAIL);
	}


	GetTracker()->UpdateMap(STRING(INDEXENT(0)->v.model));
}

