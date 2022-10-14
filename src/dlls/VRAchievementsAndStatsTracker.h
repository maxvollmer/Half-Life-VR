#pragma once

class CBaseEntity;

class VRAchievementsAndStatsTracker
{
public:
	static void PlayerTakeNegativeCrushDamage(CBaseEntity* pPlayer, float dmg);

	static void SmthKilledSmth(struct entvars_s* pKiller, struct entvars_s* pKilled, int bitsDamageType);

	static void PlayerKilledSmth(CBaseEntity* pPlayer, bool friendlyFire);

	static void PlayerTrippedMineOrLaser(CBaseEntity* pPlayer);

	static void PlayerUsedTrain(CBaseEntity* pPlayer);

	static void PlayerUsedNoclip(CBaseEntity* pPlayer);

	static void PlayerSavedDoomedScientist(CBaseEntity* pPlayer);

	static void PlayerSurfacedInThatMapWithTheTank(CBaseEntity* pPlayer);

	static void PlayerLaunchedTentacleRocketFire(CBaseEntity* pPlayer);

	static void PlayerGotHeardByTentacles();

	static void PlayerDestroyedBlastPitBridge();

	static void PlayerSolvedBlastPitFan();


	static void GiveAchievementBasedOnMap(CBaseEntity* pPlayer);
};
