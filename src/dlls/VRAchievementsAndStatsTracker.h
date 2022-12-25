#pragma once

class CBaseEntity;
class CBasePlayer;

class CVRAchievementsAndStatsData
{
public:
	int m_bitFlags = 0;
	unsigned char m_bpBridgesDestroyed = 0;
	unsigned char m_exp1HeadcrabsKilled = 0;
	unsigned char m_exp2HeadcrabsKilled = 0;
	float m_totalNegativeCrushDamage = 0.f;
	float m_residueBarneyStartedRunningTime = 0.f;
	char m_prevMap[32] = { 0 };

	void FriendlyKilled();
	void AnyKilled();
};

class VRAchievementsAndStatsTracker
{
public:
	static void PlayerTakeNegativeCrushDamage(CBaseEntity* pPlayer, float dmg);

	static void SmthKilledSmth(struct entvars_s* pKiller, struct entvars_s* pKilled, int bitsDamageType);

	static void PlayerKilledSmth(CBasePlayer* pPlayer, bool friendlyFire);

	static void PlayerTrippedMineOrLaser(CBasePlayer* pPlayer);

	static void PlayerUsedTrain(CBasePlayer* pPlayer);

	static void PlayerUsedNoclip(CBasePlayer* pPlayer);

	static void PlayerSavedDoomedScientist(CBaseEntity* pPlayer);

	static void PlayerSurfacedInThatMapWithTheTank(CBasePlayer* pPlayer);

	static void PlayerLaunchedTentacleRocketFire(CBaseEntity* pPlayer);

	static void PlayerGotHeardByTentacles();

	static void PlayerDestroyedBlastPitBridge();

	static void PlayerSolvedBlastPitFan();

	static void ResidueBarneyStartedRunning();

	static void ResidueBarneyIsAlive();

	static void Update(CBasePlayer* pPlayer);

private:
	static void GiveAchievementBasedOnMap(CBasePlayer* pPlayer, const char* mapname);
};
