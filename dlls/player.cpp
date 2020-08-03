/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include <vector>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <algorithm>

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "hltv.h"
#include "func_break.h"
#include "explode.h"
#include "talkmonster.h"
#include "trains.h"

#include "VRPhysicsHelper.h"
#include "VRGroundEntityHandler.h"
#include "VRModelHelper.h"
#include "VRMovementHandler.h"
#include "VRControllerModel.h"


constexpr const float VR_RETINASCANNER_ACTIVATE_LOOK_TIME = 1.f;
std::unordered_map<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> g_vrRetinaScanners;
std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> g_vrRetinaScannerButtons;
bool g_vrNeedRecheckForSpecialEntities = true;


// #define DUCKFIX

// Savedata descriptor for vr level change data
TYPEDESCRIPTION g_vrLevelChangeDataSaveData[] =
{
	DEFINE_FIELD(VRLevelChangeData, lastHMDOffset, FIELD_VECTOR),
	DEFINE_FIELD(VRLevelChangeData, clientOriginOffset, FIELD_VECTOR),
	DEFINE_FIELD(VRLevelChangeData, hasData, FIELD_BOOLEAN),
	DEFINE_FIELD(VRLevelChangeData, prevYaw, FIELD_FLOAT),
	DEFINE_FIELD(VRLevelChangeData, currentYaw, FIELD_FLOAT),
};

#include "pm_defs.h"
#include "pm_movevars.h"
extern playermove_t* pmove;


// If anyone ever wants to multiplayer this mod,
// you somehow need to know which player is CalculateWeaponTimeOffset being called for:
namespace
{
	EHANDLE<CBasePlayer> m_hAnalogFirePlayer;
}

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL BOOL g_fDrawLines;
int gEvilImpulse101 = 0;
extern DLL_GLOBAL int g_iSkillLevel, gDisplayTitle;


BOOL gInitHUD = TRUE;

extern void CopyToBodyQue(entvars_t* pev);
extern void respawn(entvars_t* pev, BOOL fCopyCorpse);
extern Vector VecBModelOrigin(entvars_t* pevBModel);
extern edict_t* EntSelectSpawnPoint(CBaseEntity* pPlayer);

// the world node graph
extern CGraph WorldGraph;

#define TRAIN_ACTIVE  0x80
#define TRAIN_NEW     0xc0
#define TRAIN_OFF     0x00
#define TRAIN_NEUTRAL 0x01
#define TRAIN_SLOW    0x02
#define TRAIN_MEDIUM  0x03
#define TRAIN_FAST    0x04
#define TRAIN_BACK    0x05

#define FLASH_DRAIN_TIME  1.2  //100 units/3 minutes
#define FLASH_CHARGE_TIME 0.2  // 100 units/20 seconds  (seconds per unit)

// Global Savedata for player
TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] =
{
	DEFINE_FIELD(CBasePlayer, m_flFlashLightTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_iFlashBattery, FIELD_INTEGER),

	DEFINE_FIELD(CBasePlayer, m_afButtonLast, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonPressed, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonReleased, FIELD_INTEGER),

	DEFINE_ARRAY(CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS),
	DEFINE_FIELD(CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER),

	DEFINE_FIELD(CBasePlayer, m_flTimeStepSound, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flSwimTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flDuckTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flWallJumpTime, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_flSuitUpdate, FIELD_TIME),
	DEFINE_ARRAY(CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST),
	DEFINE_FIELD(CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER),
	DEFINE_ARRAY(CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT),
	DEFINE_ARRAY(CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT),
	DEFINE_FIELD(CBasePlayer, m_lastDamageAmount, FIELD_INTEGER),

	DEFINE_ARRAY(CBasePlayer, m_rgpPlayerItems, FIELD_EHANDLE, MAX_ITEM_TYPES),
	DEFINE_FIELD(CBasePlayer, m_pActiveItem, FIELD_EHANDLE),
	DEFINE_FIELD(CBasePlayer, m_pLastItem, FIELD_EHANDLE),

	DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
	DEFINE_FIELD(CBasePlayer, m_idrowndmg, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_idrownrestored, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_tSneaking, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_flFallVelocity, FIELD_FLOAT),
	DEFINE_FIELD(CBasePlayer, m_iTargetVolume, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iWeaponVolume, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iWeaponFlash, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_fLongJump, FIELD_BOOLEAN),
	DEFINE_FIELD(CBasePlayer, m_fInitHUD, FIELD_BOOLEAN),
	DEFINE_FIELD(CBasePlayer, m_tbdPrev, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),
	DEFINE_FIELD(CBasePlayer, m_iHideHUD, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iFOV, FIELD_INTEGER),

	//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
	//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_INTEGER ), // reset to zero on load
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	//DEFINE_FIELD( CBasePlayer, m_pentSndLast, FIELD_EDICT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRoomtype, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRange, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore

};


int giPrecacheGrunt = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgSelAmmo = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgWeaponList = 0;
int gmsgAmmoX = 0;
int gmsgHudText = 0;
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgServerName = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
int gmsgTeamNames = 0;

int gmsgStatusText = 0;
int gmsgStatusValue = 0;

// VR messages
int gmsgVRRestoreYaw = 0;
int gmsgVRGroundEntity = 0;
int gmsgVRSetSpawnYaw = 0;
int gmsgVRControllerEnt = 0;
int gmsgVRTrainControls = 0;
int gmsgVRGrabbedLadder = 0;
int gmsgVRPullingLedge = 0;
int gmsgVRUpdateEgon = 0;
int gmsgVRScreenShake = 0;
int gmsgVRTouch = 0;


void LinkUserMessages(void)
{
	// Already taken care of?
	if (gmsgSelAmmo)
	{
		return;
	}

	gmsgSelAmmo = REG_USER_MSG("SelAmmo", sizeof(SelAmmo));
	gmsgCurWeapon = REG_USER_MSG("CurWeapon", 3);
	gmsgGeigerRange = REG_USER_MSG("Geiger", 1);
	gmsgFlashlight = REG_USER_MSG("Flashlight", 2);
	gmsgFlashBattery = REG_USER_MSG("FlashBat", 1);
	gmsgHealth = REG_USER_MSG("Health", 1);
	gmsgDamage = REG_USER_MSG("Damage", 12);
	gmsgBattery = REG_USER_MSG("Battery", 2);
	gmsgTrain = REG_USER_MSG("Train", 1);
	gmsgHudText = REG_USER_MSG("HudText", -1);
	gmsgSayText = REG_USER_MSG("SayText", -1);
	gmsgTextMsg = REG_USER_MSG("TextMsg", -1);
	gmsgWeaponList = REG_USER_MSG("WeaponList", -1);
	gmsgResetHUD = REG_USER_MSG("ResetHUD", 1);  // called every respawn
	gmsgInitHUD = REG_USER_MSG("InitHUD", 0);   // called every time a new player joins the server
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG("DeathMsg", -1);
	gmsgScoreInfo = REG_USER_MSG("ScoreInfo", 9);
	gmsgTeamInfo = REG_USER_MSG("TeamInfo", -1);   // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG("TeamScore", -1);  // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG("GameMode", 1);
	gmsgMOTD = REG_USER_MSG("MOTD", -1);
	gmsgServerName = REG_USER_MSG("ServerName", -1);
	gmsgAmmoPickup = REG_USER_MSG("AmmoPickup", 2);
	gmsgWeapPickup = REG_USER_MSG("WeapPickup", 1);
	gmsgItemPickup = REG_USER_MSG("ItemPickup", -1);
	gmsgHideWeapon = REG_USER_MSG("HideWeapon", 1);
	gmsgSetFOV = REG_USER_MSG("SetFOV", 1);
	gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
	gmsgAmmoX = REG_USER_MSG("AmmoX", 2);
	gmsgTeamNames = REG_USER_MSG("TeamNames", -1);

	gmsgStatusText = REG_USER_MSG("StatusText", -1);
	gmsgStatusValue = REG_USER_MSG("StatusValue", 3);

	gmsgVRRestoreYaw = REG_USER_MSG("VRRstrYaw", 2);
	gmsgVRGroundEntity = REG_USER_MSG("GroundEnt", 2);
	gmsgVRSetSpawnYaw = REG_USER_MSG("VRSpawnYaw", 1);
	gmsgVRControllerEnt = REG_USER_MSG("VRCtrlEnt", -1);
	gmsgVRTrainControls = REG_USER_MSG("TrainCtrl", 7);
	gmsgVRGrabbedLadder = REG_USER_MSG("GrbdLddr", 2);
	gmsgVRPullingLedge = REG_USER_MSG("PullLdg", 1);
	gmsgVRUpdateEgon = REG_USER_MSG("VRUpdEgon", -1);
	gmsgVRScreenShake = REG_USER_MSG("VRScrnShke", 3 * sizeof(float));
	gmsgVRTouch = REG_USER_MSG("VRTouch", 5);
}

LINK_ENTITY_TO_CLASS(player, CBasePlayer);



void CBasePlayer::Pain(void)
{
	float flRndSound = 0.f;  //sound randomizer

	flRndSound = RANDOM_FLOAT(0, 1);

	if (flRndSound <= 0.33)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
	else if (flRndSound <= 0.66)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
}

/*
 *
 */
Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound(void)
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch (RANDOM_LONG(1, 5))
	{
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT(ENT(pev), "HEV_DEAD");
}

// override takehealth
// bitsDamageType indicates type of damage healed.

int CBasePlayer::TakeHealth(float flHealth, int bitsDamageType)
{
	return CBaseMonster::TakeHealth(flHealth, bitsDamageType);
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (pev->takedamage)
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch (ptr->iHitgroup)
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= gSkillData.plrHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.plrChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.plrStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.plrArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.plrLeg;
			break;
		default:
			break;
		}

		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);  // a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
	}
}

/*
	Take some damage.
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO 0.2  // Armor Takes 80% of the damage
#define ARMOR_BONUS 0.5  // Each Point of Armor is work 1/x points of health

int CBasePlayer::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fmajor = 0;
	int fcritical = 0;
	int fTookDamage = 0;
	int ftrivial = 0;
	float flRatio = 0.f;
	float flBonus = 0.f;
	float flHealthPrev = pev->health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ((bitsDamageType & DMG_BLAST) && g_pGameRules->IsMultiplayer())
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if (!IsAlive())
		return 0;
	// go take the damage first


	CBaseEntity* pAttacker = CBaseEntity::InstanceOrWorld(pevAttacker);

	if (!g_pGameRules->FPlayerCanTakeDamage(this, pAttacker))
	{
		// Refuse the damage
		return 0;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	// Armor.
	if (pev->armorvalue && !(bitsDamageType & (DMG_FALL | DMG_DROWN)))  // armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;

		float flArmor = 0.f;

		flArmor = (flDamage - flNew) * flBonus;

		// Does this use more armor than we have?
		if (flArmor > pev->armorvalue)
		{
			flArmor = pev->armorvalue;
			flArmor *= (1 / flBonus);
			flNew = flDamage - flArmor;
			pev->armorvalue = 0;
		}
		else
			pev->armorvalue -= flArmor;

		flDamage = flNew;
	}

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if (bitsDamageType & (DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);                             // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);                 // take damage event
	WRITE_SHORT(ENTINDEX(this->edict()));      // index number of primary entity
	WRITE_SHORT(ENTINDEX(ENT(pevInflictor)));  // index number of secondary entity
	WRITE_LONG(5);                             // eventflags (priority and flags)
	MESSAGE_END();


	// how bad is it, doc?

	ftrivial = (pev->health > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (pev->health < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

	// DMG_BURN
	// DMG_FREEZE
	// DMG_BLAST
	// DMG_SHOCK

	m_bitsDamageType |= bitsDamage;  // Save this so we can report it to the client
	m_bitsHUDDamage = -1;            // make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED)) && ffound && bitsDamage)
	{
		ffound = FALSE;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);  // minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = TRUE;
		}
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", FALSE, SUIT_NEXT_IN_30SEC);  // major fracture
			else
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);  // minor fracture

			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = TRUE;
		}

		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC);  // blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_BULLET;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", FALSE, SUIT_NEXT_IN_30SEC);  // major laceration
			else
				SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);  // minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN);  // internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = TRUE;
		}

		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			SetSuitUpdate("!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN);  // blood toxins detected
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = TRUE;
		}

		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN);  // hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN);  // biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN);  // radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = TRUE;
		}
		if (bitsDamage & DMG_SHOCK)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = TRUE;
		}
	}

	pev->punchangle.x = -2;

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75)
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN);  // automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN);  // morphine shot
	}

	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{
		// already took major damage, now it's critical...
		if (pev->health < 6)
			SetSuitUpdate("!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN);  // near death
		else if (pev->health < 20)
			SetSuitUpdate("!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN);  // health critical

		// give critical health warnings
		if (!RANDOM_LONG(0, 3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN);  //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (bitsDamageType & DMG_TIMEBASED) && flHealthPrev < 75)
	{
		if (flHealthPrev < 50)
		{
			if (!RANDOM_LONG(0, 3))
				SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN);  //seek medical attention
		}
		else
			SetSuitUpdate("!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN);  // health dropping
	}

	return fTookDamage;
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems(void)
{
	int iWeaponRules = 0;
	int iAmmoRules = 0;
	int i = 0;
	EHANDLE<CBasePlayerWeapon> rgpPackWeapons[20];  // 20 hardcoded for now. How to determine exactly how many weapons we have?
	int iPackAmmo[MAX_AMMO_SLOTS + 1];
	int iPW = 0;  // index into packweapons array
	int iPA = 0;  // index into packammo array

	memset(iPackAmmo, -1, sizeof(iPackAmmo));

	// get the game rules
	iWeaponRules = g_pGameRules->DeadPlayerWeapons(this);
	iAmmoRules = g_pGameRules->DeadPlayerAmmo(this);

	if (iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO)
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems(TRUE);
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			// there's a weapon here. Should I pack it?
			EHANDLE<CBasePlayerItem> pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				switch (iWeaponRules)
				{
				case GR_PLR_DROP_GUN_ACTIVE:
					if (m_pActiveItem && pPlayerItem == m_pActiveItem)
					{
						// this is the active item. Pack it.
						rgpPackWeapons[iPW++] = pPlayerItem;
					}
					break;

				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[iPW++] = pPlayerItem;
					break;

				default:
					break;
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	// now go through ammo and make a list of which types to pack.
	if (iAmmoRules != GR_PLR_DROP_AMMO_NO)
	{
		for (i = 0; i < MAX_AMMO_SLOTS; i++)
		{
			if (m_rgAmmo[i] > 0)
			{
				// player has some ammo of this type.
				switch (iAmmoRules)
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[iPA++] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					if (m_pActiveItem && i == m_pActiveItem->PrimaryAmmoIndex())
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					else if (m_pActiveItem && i == m_pActiveItem->SecondaryAmmoIndex())
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					break;

				default:
					break;
				}
			}
		}
	}

	// create a box to pack the stuff into.
	CWeaponBox* pWeaponBox = CBaseEntity::Create<CWeaponBox>("weaponbox", pev->origin, pev->angles, edict());

	pWeaponBox->pev->angles.x = 0;  // don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink(&CWeaponBox::Kill);
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

	// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

	// pack the ammo
	while (iPackAmmo[iPA] != -1)
	{
		pWeaponBox->PackAmmo(MAKE_STRING(CBasePlayerItem::AmmoInfoArray[iPackAmmo[iPA]].pszName), m_rgAmmo[iPackAmmo[iPA]]);
		iPA++;
	}

	// now pack all of the items in the lists
	while (rgpPackWeapons[iPW])
	{
		// weapon unhooked from the player. Pack it into der box.
		pWeaponBox->PackWeapon(rgpPackWeapons[iPW]);

		iPW++;
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2;  // weaponbox has player's velocity, then some.

	RemoveAllItems(TRUE);  // now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems(BOOL removeSuit)
{
	if (m_pActiveItem)
	{
		ResetAutoaim();
		m_pActiveItem->Holster();
		m_pActiveItem = nullptr;
	}

	m_pLastItem = nullptr;

	int i = 0;
	CBasePlayerItem* pPendingItem;
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while (m_pActiveItem)
		{
			pPendingItem = m_pActiveItem->m_pNext;
			m_pActiveItem->Drop();
			m_pActiveItem = pPendingItem;
		}
		m_rgpPlayerItems[i] = nullptr;
	}
	m_pActiveItem = nullptr;

	pev->viewmodel = 0;
	pev->weaponmodel = 0;

	if (removeSuit)
		pev->weapons = 0;
	else
		pev->weapons &= ~WEAPON_ALLWEAPONS;

	for (i = 0; i < MAX_AMMO_SLOTS; i++)
		m_rgAmmo[i] = 0;

	UpdateClientData();
	// send Selected Weapon Message to our client
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, nullptr, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	MESSAGE_END();
}

const char* GibToString(int iGib)
{
	switch (iGib)
	{
	case GIB_ALWAYS:
		return "GIB_ALWAYS";
	case GIB_NEVER:
		return "GIB_NEVER";
	case GIB_NORMAL:
		return "GIB_NORMAL";
	case GIB_HEALTH_VALUE:
		return "GIB_HEALTH_VALUE";
	default:
		return "GIB_UNKNOWN";
	}
}

std::string DamageBitsToString(int bitsDamageType)
{
	if (bitsDamageType == 0)
		return "DMG_GENERIC";
	std::string damageString;
	if (bitsDamageType & DMG_CRUSH)
		damageString += "DMG_CRUSH";
	if (bitsDamageType & DMG_BULLET)
		damageString += "DMG_BULLET";
	if (bitsDamageType & DMG_SLASH)
		damageString += "DMG_SLASH";
	if (bitsDamageType & DMG_BURN)
		damageString += "DMG_BURN";
	if (bitsDamageType & DMG_FREEZE)
		damageString += "DMG_FREEZE";
	if (bitsDamageType & DMG_FALL)
		damageString += "DMG_FALL";
	if (bitsDamageType & DMG_BLAST)
		damageString += "DMG_BLAST";
	if (bitsDamageType & DMG_CLUB)
		damageString += "DMG_CLUB";
	if (bitsDamageType & DMG_SHOCK)
		damageString += "DMG_SHOCK";
	if (bitsDamageType & DMG_SONIC)
		damageString += "DMG_SONIC";
	if (bitsDamageType & DMG_ENERGYBEAM)
		damageString += "DMG_ENERGYBEAM";
	if (bitsDamageType & DMG_NEVERGIB)
		damageString += "DMG_NEVERGIB";
	if (bitsDamageType & DMG_ALWAYSGIB)
		damageString += "DMG_ALWAYSGIB";
	if (bitsDamageType & DMG_DROWN)
		damageString += "DMG_DROWN";
	if (bitsDamageType & DMG_TIMEBASED)
		damageString += "DMG_TIMEBASED";
	if (bitsDamageType & DMG_PARALYZE)
		damageString += "DMG_PARALYZE";
	if (bitsDamageType & DMG_NERVEGAS)
		damageString += "DMG_NERVEGAS";
	if (bitsDamageType & DMG_POISON)
		damageString += "DMG_POISON";
	if (bitsDamageType & DMG_RADIATION)
		damageString += "DMG_RADIATION";
	if (bitsDamageType & DMG_DROWNRECOVER)
		damageString += "DMG_DROWNRECOVER";
	if (bitsDamageType & DMG_ACID)
		damageString += "DMG_ACID";
	if (bitsDamageType & DMG_SLOWBURN)
		damageString += "DMG_SLOWBURN";
	if (bitsDamageType & DMG_SLOWFREEZE)
		damageString += "DMG_SLOWFREEZE";
	if (bitsDamageType & DMG_MORTAR)
		damageString += "DMG_MORTAR";
	if (damageString.empty())
		return "DMG_UNKNOWN";
	else
		return damageString;
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t* g_pevLastInflictor;  // Set in combat.cpp.  Used to pass the damage inflictor for death messages.
								// Better solution:  Add as parameter to all Killed() functions.

void CBasePlayer::Killed(entvars_t* pevAttacker, int bitsDamageType, int iGib)
{
	ClearLadderGrabbingControllers();
	StopPullingLedge();

	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
	pev->flags &= ~FL_BARNACLED;

	// Debug death console message - Max Vollmer, 2019-04-13
	if (pevAttacker)
	{
		ALERT(at_console, "Killed by %s %s, damage type: %s, iGib: %s\n", STRING(pevAttacker->classname), STRING(pevAttacker->targetname), DamageBitsToString(bitsDamageType).data(), GibToString(iGib));
	}
	else
	{
		ALERT(at_console, "Killed by world, damage type: %s, iGib: %s\n", DamageBitsToString(bitsDamageType).data(), GibToString(iGib));
	}

	CSound* pSound;

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	g_pGameRules->PlayerKilled(this, pevAttacker, g_pevLastInflictor);

	if (m_pTank != nullptr)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = nullptr;
	}

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
	{
		if (pSound)
		{
			pSound->Reset();
		}
	}

	SetAnimation(PLAYER_DIE);

	m_iRespawnFrames = 0;

	pev->modelindex = g_ulModelIndexPlayer;  // don't use eyes

	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_NONE;  // MOVETYPE_TOSS;
	ClearBits(pev->flags, FL_ONGROUND);
	if (pev->velocity.z < 10)
		pev->velocity.z += RANDOM_FLOAT(0, 300);

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(nullptr, FALSE, 0);

	// send "health" update message to zero
	m_iClientHealth = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgHealth, nullptr, pev);
	WRITE_BYTE(m_iClientHealth);
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, nullptr, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0XFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	pev->fov = m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, nullptr, pev);
	WRITE_BYTE(0);
	MESSAGE_END();


	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector(128,0,0), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

	if ((pev->health < -40 && iGib != GIB_NEVER) || iGib == GIB_ALWAYS)
	{
		pev->solid = SOLID_NOT;
		GibMonster();  // This clears pev->model
		pev->effects |= EF_NODRAW;
		return;
	}

	DeathSound();

	pev->angles.x = 0;
	pev->angles.z = 0;

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


// Set the activity based on an event or current state
void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	int animDesired = 0;
	float speed = 0.f;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	if (pev->flags & FL_FROZEN)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch (playerAnim)
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;

	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;

	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity();
		break;

	case PLAYER_ATTACK1:
		switch (m_Activity)
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if (!FBitSet(pev->flags, FL_ONGROUND) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP))  // Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if (pev->waterlevel > 1)
		{
			if (speed == 0)
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	}

	switch (m_IdealActivity)
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if (m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity(m_Activity);
		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence = animDesired;
		pev->frame = 0;
		ResetSequenceInfo();
		return;

	case ACT_RANGE_ATTACK1:
		if (FBitSet(pev->flags, FL_DUCKING))  // crouching
			strcpy_s(szAnim, "crouch_shoot_");
		else
			strcpy_s(szAnim, "ref_shoot_");
		strcat_s(szAnim, m_szAnimExtention);
		animDesired = LookupSequence(szAnim);
		if (animDesired == -1)
			animDesired = 0;

		if (pev->sequence != animDesired || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence = animDesired;
		ResetSequenceInfo();
		break;

	case ACT_WALK:
		if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
		{
			if (FBitSet(pev->flags, FL_DUCKING))  // crouching
				strcpy_s(szAnim, "crouch_aim_");
			else
				strcpy_s(szAnim, "ref_aim_");
			strcat_s(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);
			if (animDesired == -1)
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		if (speed == 0)
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCHIDLE);
			// pev->gaitsequence	= LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCH);
		}
	}
	else if (speed > 220)
	{
		pev->gaitsequence = LookupActivity(ACT_RUN);
	}
	else if (speed > 0)
	{
		pev->gaitsequence = LookupActivity(ACT_WALK);
	}
	else
	{
		// pev->gaitsequence	= LookupActivity( ACT_WALK );
		pev->gaitsequence = LookupSequence("deep_idle");
	}


	// Already using the desired animation?
	if (pev->sequence == animDesired)
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence = animDesired;
	pev->frame = 0;
	ResetSequenceInfo();
}

/*
===========
TabulateAmmo
This function is used to find and store
all the ammo we have into the ammo vars.
============
*/
void CBasePlayer::TabulateAmmo()
{
	ammo_9mm = AmmoInventory(GetAmmoIndex("9mm"));
	ammo_357 = AmmoInventory(GetAmmoIndex("357"));
	ammo_argrens = AmmoInventory(GetAmmoIndex("ARgrenades"));
	ammo_bolts = AmmoInventory(GetAmmoIndex("bolts"));
	ammo_buckshot = AmmoInventory(GetAmmoIndex("buckshot"));
	ammo_rockets = AmmoInventory(GetAmmoIndex("rockets"));
	ammo_uranium = AmmoInventory(GetAmmoIndex("uranium"));
	ammo_hornets = AmmoInventory(GetAmmoIndex("Hornets"));
}


/*
===========
WaterMove
============
*/
#define AIRTIME 12  // lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air = 0;

	/*
	if (pev->movetype == MOVETYPE_NOCLIP)
		return;
	*/

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3)
	{
		// not underwater

		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{  // fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time)  // drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += pev->dmg;
			}
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (!pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
		{
			ClearBits(pev->flags, FL_INWATER);
		}
		return;
	}

	// make bubbles

	air = (int)(pev->air_finished - gpGlobals->time);
	if (!RANDOM_LONG(0, 0x1f) && RANDOM_LONG(0, AIRTIME - 1) >= air)
	{
		switch (RANDOM_LONG(0, 3))
		{
		case 0: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM); break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM); break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM); break;
		case 3: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM); break;
		}
	}

	if (pev->watertype == CONTENT_LAVA)  // do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME)  // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 4 * pev->waterlevel, DMG_ACID);
	}

	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
}


// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder(void)
{
	return pev->movetype == MOVETYPE_FLY;
}

void CBasePlayer::PlayerDeathThink(void)
{
	ClearLadderGrabbingControllers();
	StopPullingLedge();

	float flForward = 0.f;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	if (HasWeapons())
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}


	if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
	{
		StudioFrameAdvance();

		m_iRespawnFrames++;          // Note, these aren't necessarily real "frames", so behavior is dependent on # of client movement commands
		if (m_iRespawnFrames < 120)  // Animations should be no longer than this
			return;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if they walked over it while the dead player was clicking their button to respawn
	if (pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND))
		pev->movetype = MOVETYPE_NONE;

	if (pev->deadflag == DEAD_DYING)
		pev->deadflag = DEAD_DEAD;

	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	// wait for all buttons released
	if (pev->deadflag == DEAD_DEAD)
	{
		// clear attack/use commands
		m_afButtonPressed = 0;
		pev->button = 0;
		m_afButtonReleased = 0;
		m_fDeadTime = gpGlobals->time;
		pev->deadflag = DEAD_RESPAWNABLE;
		pmove->oldbuttons = 0;
		pmove->cmd.buttons = 0;
		pmove->cmd.buttons_ex = 0;
		return;
	}

	// auto-respawn after 1 second in VR (respawn immediately if any button down)
	BOOL fAnyButtonDown = (pev->button & ~IN_SCORE);
	if (fAnyButtonDown || gpGlobals->time > (m_fDeadTime + 1.f))
	{
		// clear attack/use commands
		m_afButtonPressed = 0;
		pev->button = 0;
		m_afButtonReleased = 0;
		m_iRespawnFrames = 0;
		pmove->oldbuttons = 0;
		pmove->cmd.buttons = 0;
		pmove->cmd.buttons_ex = 0;

		//ALERT(at_console, "Respawn\n");

		respawn(pev, !(m_afPhysicsFlags & PFLAG_OBSERVER));  // don't copy a corpse if we're in deathcam.
		pev->nextthink = -1;
	}
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam(void)
{
	edict_t* pSpot, * pNewSpot;
	int iRand = 0;

	if (pev->view_ofs == g_vecZero)
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME(nullptr, "info_intermission");

	if (!FNullEnt(pSpot))
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG(0, 3);

		while (iRand > 0)
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME(pSpot, "info_intermission");

			if (pNewSpot)
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CopyToBodyQue(pev);
		StartObserver(pSpot->v.origin, pSpot->v.v_angle);
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		CopyToBodyQue(pev);
		UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, 128), ignore_monsters, edict(), &tr);
		StartObserver(tr.vecEndPos, UTIL_VecToAngles(tr.vecEndPos - pev->origin));
		return;
	}
}

void CBasePlayer::StartObserver(Vector vecPosition, Vector vecViewAngle)
{
	m_afPhysicsFlags |= PFLAG_OBSERVER;

	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
	UTIL_SetOrigin(pev, vecPosition);
}

//
// PlayerUse - handles USE keypress
//
#define PLAYER_SEARCH_RADIUS (float)64

void CBasePlayer::PlayerUse(void)
{
	// Was use pressed or released?
	if (!((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE))
		return;

	// Hit Use on a train?
	if (m_afButtonPressed & IN_USE)
	{
		if (m_pTank != nullptr)
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = nullptr;
			SelectLastItem();
			return;
		}
		else
		{
			if (CVAR_GET_FLOAT("vr_train_controls") != 0.f)
			{
				if (m_afPhysicsFlags & PFLAG_ONTRAIN)
				{
					m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
					m_iTrain = TRAIN_NEW | TRAIN_OFF;
					return;
				}
				else
				{
					// Check if we are on a train
					CBaseEntity* pMaybeTrain = CBaseEntity::InstanceOrWorld(pev->groundentity);
					if (IsUsableTrackTrain(pMaybeTrain))
					{
						// Start controlling the train!
						m_afPhysicsFlags |= PFLAG_ONTRAIN;
						m_iTrain = TrainSpeed(pMaybeTrain->pev->speed, pMaybeTrain->pev->impulse);
						m_iTrain |= TRAIN_NEW;
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
						return;
					}
				}
			}
		}
	}

	CBaseEntity* pObject = nullptr;
	CBaseEntity* pClosest = nullptr;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot = 0.f;

	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != nullptr)
	{
		if (pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE))
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin(pObject->pev) - (pev->origin + pev->view_ofs));

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);

			flDot = DotProduct(vecLOS, vr_hmdForward);
			if (flDot > flMaxDot)
			{  // only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
			//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject)
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls
		int caps = pObject->ObjectCaps();

		if (m_afButtonPressed & IN_USE)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

		if (((pev->button & IN_USE) && (caps & FCAP_CONTINUOUS_USE)) ||
			((m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE | FCAP_ONOFF_USE))))
		{
			if (caps & FCAP_CONTINUOUS_USE)
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use(this, this, USE_SET, 1);
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ((m_afButtonReleased & IN_USE) && (pObject->ObjectCaps() & FCAP_ONOFF_USE))  // BUGBUG This is an "off" use
		{
			pObject->Use(this, this, USE_SET, 0);
		}
	}
	else
	{
		if (m_afButtonPressed & IN_USE)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}
}



void CBasePlayer::Jump()
{
	Vector vecWallCheckDir;  // direction we're tracing a line to find a wall when walljumping
	Vector vecAdjustedVelocity;
	Vector vecSpot;
	TraceResult tr;

	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;

	if (pev->waterlevel >= 2)
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if (!FBitSet(m_afButtonPressed, IN_JUMP))
		return;  // don't pogo stick

	if (!(pev->flags & FL_ONGROUND) || !pev->groundentity)
	{
		return;
	}

	// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors(pev->angles);

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk

	SetAnimation(PLAYER_JUMP);

	if (m_fLongJump &&
		FBitSet(pev->flags, FL_DUCKING) &&  //		(pev->button & IN_DUCK) &&
		(pev->flDuckTime > 0) &&
		pev->velocity.Length() > 50)
	{
		SetAnimation(PLAYER_SUPERJUMP);
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t* pevGround = VARS(pev->groundentity);
	if (pevGround && (pevGround->flags & FL_CONVEYOR))
	{
		pev->velocity = pev->velocity + pev->basevelocity;
	}
}


void CBasePlayer::Duck()
{
	// Disable ducking in VR (ducking is realized by real "ducking")
}

//
// ID's player as such.
//
int CBasePlayer::Classify(void)
{
	return CLASS_PLAYER;
}


void CBasePlayer::AddPoints(int score, BOOL bAllowNegativeScore)
{
	// Positive score always adds
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (pev->frags < 0)  // Can't go more negative
				return;

			if (-score > pev->frags)  // Will this go negative?
			{
				score = -pev->frags;  // Sum will be 0
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_SHORT(pev->frags);
	WRITE_SHORT(m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(g_pGameRules->GetTeamIndex(m_szTeamName) + 1);
	MESSAGE_END();
}


void CBasePlayer::AddPointsToTeam(int score, BOOL bAllowNegativeScore)
{
	int index = entindex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer && i != index)
		{
			if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			{
				pPlayer->AddPoints(score, bAllowNegativeScore);
			}
		}
	}
}

//Player ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0;
}

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[SBAR_END];
	char sbuf0[SBAR_STRING_SIZE];
	char sbuf1[SBAR_STRING_SIZE];

	memset(newSBarState, 0, sizeof(newSBarState));
	strcpy_s(sbuf0, m_SbarString0);
	strcpy_s(sbuf1, m_SbarString1);

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + (gpGlobals->v_forward * MAX_ID_RANGE);
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction != 1.0)
	{
		if (!FNullEnt(tr.pHit))
		{
			CBasePlayer* pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(tr.pHit);

			if (pPlayer)
			{
				newSBarState[SBAR_ID_TARGETNAME] = ENTINDEX(pPlayer->edict());
				strcpy_s(sbuf1, "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%");

				// allies and medics get to see the targets health
				if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
				{
					newSBarState[SBAR_ID_TARGETHEALTH] = 100 * (pPlayer->pev->health / pPlayer->pev->max_health);
					newSBarState[SBAR_ID_TARGETARMOR] = pPlayer->pev->armorvalue;  //No need to get it % based since 100 it's the max.
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
			}
		}
		else if (m_flStatusBarDisappearDelay > gpGlobals->time)
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
			newSBarState[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
			newSBarState[SBAR_ID_TARGETARMOR] = m_izSBarState[SBAR_ID_TARGETARMOR];
		}
	}

	BOOL bForceResend = FALSE;

	if (strcmp(sbuf0, m_SbarString0))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, nullptr, pev);
		WRITE_BYTE(0);
		WRITE_STRING(sbuf0);
		MESSAGE_END();

		strcpy_s(m_SbarString0, sbuf0);

		// make sure everything's resent
		bForceResend = TRUE;
	}

	if (strcmp(sbuf1, m_SbarString1))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, nullptr, pev);
		WRITE_BYTE(1);
		WRITE_STRING(sbuf1);
		MESSAGE_END();

		strcpy_s(m_SbarString1, sbuf1);

		// make sure everything's resent
		bForceResend = TRUE;
	}

	// Check values and send if they don't match
	for (int i = 1; i < SBAR_END; i++)
	{
		if (newSBarState[i] != m_izSBarState[i] || bForceResend)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusValue, nullptr, pev);
			WRITE_BYTE(i);
			WRITE_SHORT(newSBarState[i]);
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}

bool CBasePlayer::CheckVRTRainButtonTouched(const Vector& buttonLeftPos, const Vector& buttonRightPos)
{
	for (auto& [id, controller] : m_vrControllers)
	{
		if (controller.IsValid())
		{
			if (VRPhysicsHelper::Instance().ModelIntersectsLine(controller.GetModel(), buttonLeftPos, buttonRightPos))
			{
				controller.AddTouch(VRController::TouchType::LIGHT_TOUCH, 0.1f);
				return true;
			}
		}
	}
	return false;
}

float GetTrainSpeed(const char* cvarname)
{
	float speed = CVAR_GET_FLOAT(cvarname);

	if (speed <= 0.f)
		return 0.25f;

	if (speed > 1.f)
		return 1.f;

	return speed;
}

bool IsTrashCompactor(CBaseEntity* pEntity)
{
	std::string modelname = STRING(pEntity->pev->model);

	if (modelname.empty() || modelname[0] != '*')
		return false;

	std::string mapname = STRING(INDEXENT(0)->v.model);

	if (mapname != std::string{ "maps/c2a3e.bsp" })
		return false;

	return modelname == "*6" || modelname == "*13";
}

bool CBasePlayer::IsUsableTrackTrain(CBaseEntity* pTrain)
{
	return pTrain
		&& !FBitSet(pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL)
		&& FBitSet(pTrain->ObjectCaps(), FCAP_DIRECTIONAL_USE)
		&& pTrain->OnControls(pev)
		&& !IsTrashCompactor(pTrain);
}

void CBasePlayer::PreThink(void)
{
	// gets reset by ClientPrecache everytime new map is loaded (new game, changelevel, load save)
	if (g_vrNeedRecheckForSpecialEntities)
	{
		g_vrNeedRecheckForSpecialEntities = false;
		for (int index = 1; index < gpGlobals->maxEntities; index++)
		{
			edict_t* pent = INDEXENT(index);
			if (FNullEnt(pent))
				continue;

			EHANDLE<CBaseEntity> hEntity = CBaseEntity::SafeInstance<CBaseEntity>(pent);
			if (hEntity)
			{
				if (!hEntity->CheckIsSpecialVREntity())
				{
					g_vrNeedRecheckForSpecialEntities = true;
				}
			}
		}
	}

	// Make sure we always have the right hand model
	if (HasSuit() && FStrEq(STRING(pev->viewmodel), "models/v_hand_labcoat.mdl"))
	{
		pev->viewmodel = MAKE_STRING("models/v_hand_hevsuit.mdl");
	}
	else if (!HasSuit() && FStrEq(STRING(pev->viewmodel), "models/v_hand_hevsuit.mdl"))
	{
		pev->viewmodel = MAKE_STRING("models/v_hand_labcoat.mdl");
	}

	int buttonsChanged = (m_afButtonLast ^ pev->button);  // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed = buttonsChanged & pev->button;     // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button);  // The ones not down are "released"

	if (g_pGameRules)
		g_pGameRules->PlayerThink(this);

	if (g_fGameOver)
		return;  // intermission or finale

	// VR: We are MOVETYPE_NOCLIP, so the engine doesn't handle collisions with solid entities for various reasons (getting stuck, pushables not working nicely in VR etc.)
	// This also means that trains and elevators just move through us. To avoid this, we do appropriate movement handling here.
	// P.S. In pm_shared.cpp we do some modified normal movement as if we were MOVETYPE_WALK
	GetGroundEntityHandler().HandleMovingWithSolidGroundEntities();

	UTIL_MakeVectors(pev->v_angle);  // is this still used?

	ItemPreFrame();
	WaterMove();

	if (g_pGameRules && g_pGameRules->FAllowFlashlight())
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	if (m_pTank)
		m_iHideHUD |= HIDEHUD_WEAPONBLOCKED;
	else
		m_iHideHUD &= ~HIDEHUD_WEAPONBLOCKED;

	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	CheckSuitUpdate();

	if (pev->deadflag >= DEAD_DYING)
	{
		PlayerDeathThink();
		return;
	}

	// VR stuff: Calculate controller interactions with world
	for (auto& controller : m_vrControllers)
	{
		m_vrControllerInteractionManager.CheckAndPressButtons(this, controller.second);
	}

	// Special interaction with 2 controllers at once (e.g. pull up on ledges)
	if (m_vrControllers[VRControllerID::HAND].IsValid() && m_vrControllers[VRControllerID::WEAPON].IsValid())
	{
		m_vrControllerInteractionManager.DoMultiControllerActions(this, m_vrControllers[VRControllerID::HAND], m_vrControllers[VRControllerID::WEAPON]);
	}

	// Handle retina scanners
	VRHandleRetinaScanners();

	// Check if we are on a train
	if (CVAR_GET_FLOAT("vr_train_controls") == 0.f)
	{
		if (pev->groundentity && !FBitSet(m_afPhysicsFlags, PFLAG_ONTRAIN))
		{
			CBaseEntity* pMaybeTrain = CBaseEntity::InstanceOrWorld(pev->groundentity);
			if (IsUsableTrackTrain(pMaybeTrain))
			{
				// Start controlling the train!
				m_afPhysicsFlags |= PFLAG_ONTRAIN;
				m_iTrain = TrainSpeed(pMaybeTrain->pev->speed, pMaybeTrain->pev->impulse);
				m_iTrain |= TRAIN_NEW;
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
			}
		}
	}

	// Set FL_STUCK_ONTRAIN only if legacy train controls are enabled (this flag prevents movement in pm_shared)
	if (CVAR_GET_FLOAT("vr_train_controls") != 0.f && (m_afPhysicsFlags & PFLAG_ONTRAIN))
		pev->flags |= FL_STUCK_ONTRAIN;
	else
		pev->flags &= ~FL_STUCK_ONTRAIN;

	// Check if we are still on a train
	if (m_afPhysicsFlags & PFLAG_ONTRAIN)
	{
		CBaseEntity* pTrain = CBaseEntity::InstanceOrWorld(pev->groundentity);

		if (!pTrain)
		{
			// Maybe this is on the other side of a level transition
			TraceResult trainTrace;
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), ignore_monsters, ENT(pev), &trainTrace);

			// HACKHACK - Just look for the func_tracktrain classname
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
			{
				pTrain = CBaseEntity::InstanceOrWorld(trainTrace.pHit);
			}
		}

		if (!IsUsableTrackTrain(pTrain))
		{
			// Turn off the train if the train controls go dead or you leave the control area
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
		}
		else
		{
			// Train speed control
			Vector forward;
			Vector right;
			Vector up;
			UTIL_MakeVectorsPrivate(pTrain->pev->angles, forward, right, up);

			Vector controlsOffset;
			auto pTrackTrain = dynamic_cast<CFuncTrackTrain*>(pTrain);
			if (pTrackTrain)
			{
				controlsOffset = pTrackTrain->GetVRControlsOffset();
			}
			else
			{
				controlsOffset = Vector{ -48.f, 20.f, pev->maxs.z };
			}
			vr_trainControlPosition = pTrain->pev->origin + (forward * controlsOffset.x) + (right * controlsOffset.y) + (up * controlsOffset.z);
			vr_trainControlYaw = pTrain->pev->angles.y;

			if (m_iTrain == TRAIN_OFF)
			{
				m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			}

			if (CVAR_GET_FLOAT("vr_train_controls") != 0.f)
			{
				pev->velocity = g_vecZero;
				float vel = 0;
				if (m_afButtonPressed & IN_FORWARD)
				{
					vel = 1;
				}
				else if (m_afButtonPressed & IN_BACK)
				{
					vel = -1;
				}
				if (vel)
				{
					pTrain->Use(this, this, USE_SET, vel);
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
				}
			}
			else
			{
				Vector trainFastButtonPos = vr_trainControlPosition + (forward * -8.f)	 + (up * 20.f);
				Vector trainMedmButtonPos = vr_trainControlPosition + (forward * -6.25f) + (up * 15.5f);
				Vector trainSlowButtonPos = vr_trainControlPosition + (forward * -4.5f)	 + (up * 11.5f);
				Vector trainNeutButtonPos = vr_trainControlPosition + (forward * -2.75f) + (up * 6.5f);
				Vector trainBackButtonPos = vr_trainControlPosition + (forward * -1.f)	 + (up * 2.f);

				float newspeed = pTrain->pev->speed;
				if (CheckVRTRainButtonTouched(trainNeutButtonPos + (right * 8.f), trainNeutButtonPos - (right * 8.f)))
				{
					m_iTrain = TRAIN_NEUTRAL;
					newspeed = 0.f;
				}
				else if (CheckVRTRainButtonTouched(trainSlowButtonPos + (right * 8.f), trainSlowButtonPos - (right * 8.f)))
				{
					m_iTrain = TRAIN_SLOW;
					newspeed = pTrain->pev->impulse * GetTrainSpeed("vr_train_speed_slow");
				}
				else if (CheckVRTRainButtonTouched(trainMedmButtonPos + (right * 8.f), trainMedmButtonPos - (right * 8.f)))
				{
					m_iTrain = TRAIN_MEDIUM;
					newspeed = pTrain->pev->impulse * GetTrainSpeed("vr_train_speed_medium");
				}
				else if (CheckVRTRainButtonTouched(trainFastButtonPos + (right * 8.f), trainFastButtonPos - (right * 8.f)))
				{
					m_iTrain = TRAIN_FAST;
					newspeed = pTrain->pev->impulse * GetTrainSpeed("vr_train_speed_fast");
				}
				else if (CheckVRTRainButtonTouched(trainBackButtonPos + (right * 8.f), trainBackButtonPos - (right * 8.f)))
				{
					m_iTrain = TRAIN_BACK;
					newspeed = -pTrain->pev->impulse * GetTrainSpeed("vr_train_speed_back");
				}

				if (newspeed != pTrain->pev->speed)
				{
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use2.wav", 0.6, ATTN_NORM);

					pTrain->pev->speed = newspeed;
					auto pTrackTrain = dynamic_cast<CFuncTrackTrain*>(pTrain);
					if (pTrackTrain) pTrackTrain->Next();
				}
			}

			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;

			m_hLastTrain = pTrain;
		}
	}

	// autostop train if we are not standing on it anymore
	if (!FBitSet(m_afPhysicsFlags, PFLAG_ONTRAIN))
	{
		if (m_hLastTrain && (m_hLastTrain->edict() != pev->groundentity) && m_hLastTrain->pev->speed > 0 && CVAR_GET_FLOAT("vr_train_autostop") != 0.f)
		{
			m_hLastTrain->pev->speed = 0;
			m_hLastTrain->pev->velocity = g_vecZero;
			m_hLastTrain->pev->avelocity = g_vecZero;
			m_hLastTrain->SetThink(nullptr);
			EHANDLE<CFuncTrackTrain> hTrackTrain = m_hLastTrain;
			if (hTrackTrain)
			{
				hTrackTrain->StopSound();
			}
			m_hLastTrain = nullptr;
		}
	}

	if (FBitSet(m_iTrain, TRAIN_ACTIVE) && !FBitSet(m_afPhysicsFlags, PFLAG_ONTRAIN))
	{
		m_iTrain = TRAIN_NEW;  // turn off train
	}

	if (pev->button & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = nullptr;

	if (m_afPhysicsFlags & PFLAG_ONBARNACLE)
	{
		pev->velocity = g_vecZero;
	}
}

/* Time based Damage works as follows:
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.


*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */


void CBasePlayer::CheckTimeBasedDamage()
{
	int i = 0;
	BYTE bDuration = 0;

	static float gtbdPrev = 0.0;

	if (!(m_bitsDamageType & DMG_TIMEBASED))
		return;

	// only check for time based damage approx. every 2 seconds
	if (fabs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;

	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if (m_bitsDamageType & (DMG_PARALYZE << i))
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				//				TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				//				TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4;  // get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//				TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//				TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//				TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison) && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", FALSE, SUIT_REPEAT_OK);
					}
				}


				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation.
Some functions are automatic, some require power.
The player gets the suit shortly after getting off the train in C1A0 and it stays
with them for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit
		will come on and the battery will drain while the player stays in the area.
		After the battery is dead, the player starts to take damage.
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge.

Notification (via the HUD)

x	Health
x	Ammo
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged.
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor.

Augmentation

	Reanimation (w/adrenaline)
		Causes the player to come back to life after they have been dead for 3 seconds.
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA
		Used automatically after picked up and after player enters the water.
		Works for N seconds. Single use.

Things powered by the battery

	Armor
		Uses N watts for every M units of damage.
	Heat/Cool
		Uses N watts for every second in hot/cold area.
	Long Jump
		Uses N watts for every jump.
	Alien Cloak
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield
		Augments armor. Reduces Armor drain by one half

*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer::UpdateGeigerCounter(void)
{
	BYTE range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client

	range = (BYTE)(m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, nullptr, pev);
		WRITE_BYTE(range);
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0, 3))
		m_flgeigerRange = 1000;
}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME      3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int i = 0;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if (!(pev->weapons & (1 << WEAPON_SUIT)))
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (g_pGameRules->IsMultiplayer())
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if (gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
		{
			if (isentence = m_rgSuitPlayList[isearch])
				break;

			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[CBSENTENCENAME_MAX + 1];
				strcpy_s(sentence, "!");
				strcat_s(sentence, gszallsentencenames[isentence]);
				EMIT_SOUND_SUIT(ENT(pev), sentence);
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT(ENT(pev), -isentence);
			}
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check
			m_flSuitUpdate = 0;
	}
}

// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(char* name, int fgroup, int iNoRepeatTime)
{
	int i = 0;
	int isentence = 0;
	int iempty = -1;


	// Ignore suit updates if no suit
	if (!(pev->weapons & (1 << WEAPON_SUIT)))
		return;

	if (g_pGameRules->IsMultiplayer())
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == nullptr, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name, nullptr, 0);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			// this sentence or group is already in
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT - 1);  // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot

	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
	}
}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
static void
CheckPowerups(entvars_t* pev)
{
	if (pev->health <= 0)
		return;

	pev->modelindex = g_ulModelIndexPlayer;  // don't use eyes
}


//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer::UpdatePlayerSound(void)
{
	int iBodyVolume = 0;
	int iVolume = 0;
	CSound* pSound;

	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (!pSound)
	{
		ALERT(at_console, "Client lost reserved sound!\n");
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than their body/movement, use the weapon volume, else, use the body volume.

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		iBodyVolume = pev->velocity.Length();

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast.
		if (iBodyVolume > 512)
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	// convert player move speed and actions into sound audible by monsters.
	if (m_iWeaponVolume > iBodyVolume)
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player.
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	if (m_iWeaponVolume < 0)
	{
		iVolume = 0;
	}


	// if target volume is greater than the player sound's current volume, we paste the new volume in
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if (m_iTargetVolume > iVolume)
	{
		iVolume = m_iTargetVolume;
	}
	else if (iVolume > m_iTargetVolume)
	{
		iVolume -= 250 * gpGlobals->frametime;

		if (iVolume < m_iTargetVolume)
		{
			iVolume = 0;
		}
	}

	if (m_fNoPlayerSound)
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if (gpGlobals->time > m_flStopExtraSoundTime)
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if (pSound)
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= (bits_SOUND_PLAYER | m_iExtraSoundTypes);
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	//UTIL_MakeVectors ( pev->angles );
	//gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the
	// player is making.
	// UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT ( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}


void CBasePlayer::PostThink()
{
	if (g_fGameOver)
		goto pt_end;  // intermission or finale

	if (!IsAlive())
		goto pt_end;

	UpdateFlashlight();
	UpdateVRTele();
	UpdateVRLaserSpot();

	VRUseOrUnuseTank();

	VRDoTheBackPackThing();

	// Handle Tank controlling
	if (!m_vrIsUsingTankWithVRControllers && m_pTank != nullptr)
	{
		if (!m_pTank->OnControls(pev))
		{
			// player moved too far from the gun, unuse the gun
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = nullptr;
			SelectLastItem();
		}
		else if (m_pActiveItem != nullptr || pev->weaponmodel || m_vrControllers[VRControllerID::WEAPON].GetWeaponId() != WEAPON_BAREHAND)
		{
			// player selected a weapon, unuse the gun
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = nullptr;
			if (m_pActiveItem == nullptr)
			{
				SelectLastItem();
			}
		}
		else
		{
			// fire the gun
			m_pTank->Use(this, this, USE_SET, 2);
		}
	}

	// do weapon stuff
	ItemPostFrame();


	// controller feedback
	for (auto& [id, controller] : m_vrControllers)
	{
		if (controller.IsValid())
		{
			controller.PostFrame();
		}
	}


	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a
	// fallpain sound, and damage will be inflicted based on how far the player fell

	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did they hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{  // after this point, we start doing damage

			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > pev->health)
			{  //splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if (flFallDamage > 0)
			{
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), flFallDamage, DMG_FALL);
				pev->punchangle.x = 0;
			}
		}

		if (IsAlive())
		{
			SetAnimation(PLAYER_WALK);
		}
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
 		if (m_flFallVelocity > 64 && !(g_pGameRules && g_pGameRules->IsMultiplayer()))
		{
			CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2);
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character
	if (IsAlive())
	{
		if (!pev->velocity.x && !pev->velocity.y)
			SetAnimation(PLAYER_IDLE);
		else if ((pev->velocity.x || pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			SetAnimation(PLAYER_WALK);
		else if (pev->waterlevel > 1)
			SetAnimation(PLAYER_WALK);
	}

	StudioFrameAdvance();
	CheckPowerups(pev);

	UpdatePlayerSound();

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

pt_end:
#if defined(CLIENT_WEAPONS)
	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			EHANDLE<CBasePlayerItem> pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				EHANDLE<CBasePlayerWeapon> gun = pPlayerItem;
				if (gun && gun->UseDecrement())
				{
					gun->m_flNextPrimaryAttack = max(gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0f);
					gun->m_flNextSecondaryAttack = max(gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001f);
					gun->m_flTimeWeaponIdle = max(gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001f);

					if (gun->pev->fuser1 != 1000)
					{
						gun->pev->fuser1 = max(gun->pev->fuser1 - gpGlobals->frametime, -0.001f);
					}
				}
				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if (m_flNextAttack < -0.001)
		m_flNextAttack = -0.001;

	if (m_flNextAmmoBurn != 1000)
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;

		if (m_flNextAmmoBurn < -0.001)
			m_flNextAmmoBurn = -0.001;
	}

	if (m_flAmmoStartCharge != 1000)
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;

		if (m_flAmmoStartCharge < -0.001)
			m_flAmmoStartCharge = -0.001;
	}

#endif
}


// checks if the spot is clear of players
BOOL IsSpawnPointValid(CBaseEntity* pPlayer, CBaseEntity* pSpot)
{
	CBaseEntity* ent = nullptr;

	if (!pSpot->IsTriggered(pPlayer))
	{
		return FALSE;
	}

	while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != nullptr)
	{
		// if ent is a client, don't spawn on 'em
		if (ent->IsPlayer() && ent != pPlayer)
			return FALSE;
	}

	return TRUE;
}


DLL_GLOBAL CBaseEntity* g_pLastSpawn;
inline int FNullEnt(CBaseEntity* ent) { return (ent == nullptr) || FNullEnt(ent->edict()); }

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
edict_t* EntSelectSpawnPoint(CBaseEntity* pPlayer)
{
	CBaseEntity* pSpot;
	edict_t* player;

	player = pPlayer->edict();

	// choose a info_player_deathmatch point
	if (g_pGameRules->IsCoOp())
	{
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_coop");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_start");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else if (g_pGameRules->IsDeathmatch())
	{
		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for (int i = RANDOM_LONG(1, 5); i > 0; i--)
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
		if (FNullEnt(pSpot))  // skip over the null point
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");

		CBaseEntity* pFirstSpot = pSpot;

		do
		{
			if (pSpot)
			{
				// check if pSpot is valid
				if (IsSpawnPointValid(pPlayer, pSpot))
				{
					if (pSpot->pev->origin == Vector(0, 0, 0))
					{
						pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
		} while (pSpot != pFirstSpot);  // loop if we're not back to the start

		// we haven't found a place to spawn yet, so kill any player at the first spawn point and spawn there
		if (pSpot && !FNullEnt(pSpot))
		{
			CBaseEntity* ent = nullptr;
			while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != nullptr)
			{
				// if ent is a client, kill em (unless they are ourselves)
				if (ent->IsPlayer() && !(ent->edict() == player))
					ent->TakeDamage(VARS(INDEXENT(0)), VARS(INDEXENT(0)), 300, DMG_GENERIC);
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if (FStringNull(gpGlobals->startspot) || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = UTIL_FindEntityByClassname(nullptr, "info_player_start");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname(nullptr, STRING(gpGlobals->startspot));
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}

ReturnSpot:
	if (FNullEnt(pSpot))
	{
		ALERT(at_error, "PutClientInServer: no info_player_start on level");
		return INDEXENT(0);
	}

	g_pLastSpawn = pSpot;
	return pSpot->edict();
}

void CBasePlayer::Spawn(void)
{
	ClearLadderGrabbingControllers();
	StopPullingLedge();

	pev->classname = MAKE_STRING("player");
	pev->health = 100;
	pev->armorvalue = 0;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	//pev->solid		= SOLID_NOT;
	//pev->movetype		= MOVETYPE_WALK;
	pev->movetype = MOVETYPE_NOCLIP;
	pev->max_health = pev->health;
	pev->flags &= FL_PROXY;  // keep proxy flag sey by engine
	pev->flags |= FL_CLIENT;
	pev->flags &= ~FL_BARNACLED;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2;  // initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0;
	pev->gravity = 0.0;
	if (HasSuit())
	{
		pev->viewmodel = MAKE_STRING("models/v_hand_hevsuit.mdl");
	}
	else
	{
		pev->viewmodel = MAKE_STRING("models/v_hand_labcoat.mdl");
	}
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	m_fLongJump = FALSE;  // no longjump module.

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	pev->fov = m_iFOV = 0;   // init field of view.
	m_iClientFOV = -1;  // make sure fov reset is sent

	m_flNextDecalTime = 0;  // let this player decal as soon as they spawn.

	m_flgeigerDelay = gpGlobals->time + 2.0;  // wait a few seconds until user-defined message registrations
											  // are recieved by all clients

	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	m_flFieldOfView = 0.5;  // some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor = BLOOD_COLOR_RED;
	m_flNextAttack = UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1;  // force first message

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	if (g_pGameRules)
	{
		g_pGameRules->SetDefaultPlayerTeam(this);
		g_pGameRules->GetPlayerSpawnSpot(this);
	}

	SET_MODEL(ENT(pev), "models/player.mdl");
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence = LookupActivity(ACT_IDLE);

	if (FBitSet(pev->flags, FL_DUCKING))
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos = Vector(0, 32, 0);

	if (m_iPlayerSound == SOUNDLIST_EMPTY)
	{
		ALERT(at_console, "Couldn't alloc player sound slot!\n");
	}

	m_fNoPlayerSound = FALSE;  // normal sound behavior.

	m_pLastItem = nullptr;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;  // force this to be recalculated
	m_fWeapon = FALSE;
	m_pClientActiveItem = nullptr;
	m_iClientBattery = -1;

	// reset all ammo values to 0
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;

	m_flNextChatTime = gpGlobals->time;

	if (g_pGameRules)
		g_pGameRules->PlayerSpawn(this);

	vr_IsJustSpawned = true;
	vr_hasSentRestoreYawMsgToClient = false;
	vr_hasSentSpawnYawToClient = false;
}


void CBasePlayer::Precache(void)
{
	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.

	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if (WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet)
	{
		if (!WorldGraph.FSetGraphPointers())
		{
			ALERT(at_console, "**Graph pointers were not set!\n");
		}
		else
		{
			ALERT(at_console, "**Graph Pointers Set!\n");
		}
	}

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;

	m_iTrain = TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if (gInitHUD)
		m_fInitHUD = TRUE;
}


int CBasePlayer::Save(CSave& save)
{
	if (!CBaseMonster::Save(save))
		return 0;

	StoreVROffsetsForLevelchange();

	return save.WriteFields("PLAYER", this, m_playerSaveData, (int)std::size(m_playerSaveData)) && save.WriteFields("PLAYERVROffsetsForLevelchange", &g_vrLevelChangeData, g_vrLevelChangeDataSaveData, (int)std::size(g_vrLevelChangeDataSaveData));
}


//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems(void)
{
}


void CBasePlayer::StoreVROffsetsForLevelchange()
{
	g_vrLevelChangeData.lastHMDOffset = this->vr_lastHMDOffset;
	g_vrLevelChangeData.clientOriginOffset = this->vr_ClientOriginOffset;
	g_vrLevelChangeData.prevYaw = this->vr_prevYaw;
	g_vrLevelChangeData.currentYaw = this->vr_currentYaw;
	g_vrLevelChangeData.hasData = true;
}

int CBasePlayer::Restore(CRestore& restore)
{
	ClearLadderGrabbingControllers();
	StopPullingLedge();

	if (!CBaseMonster::Restore(restore))
		return 0;

	int status = restore.ReadFields("PLAYER", this, m_playerSaveData, (int)std::size(m_playerSaveData)) && restore.ReadFields("PLAYERVROffsetsForLevelchange", &g_vrLevelChangeData, g_vrLevelChangeDataSaveData, (int)std::size(g_vrLevelChangeDataSaveData));

	SAVERESTOREDATA* pSaveData = static_cast<SAVERESTOREDATA*>(gpGlobals->pSaveData);
	// landmark isn't present.
	if (!pSaveData->fUseLandmark)
	{
		ALERT(at_console, "No Landmark:%s\n", pSaveData->szLandmarkName);

		// default to normal spawn
		edict_t* pentSpawnSpot = EntSelectSpawnPoint(this);
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
	}

	// Restore VR offsets if levelchange has stored them (fixes origin issues in roomscale) - Max Vollmer, 2018-04-02
	if (g_vrLevelChangeData.hasData)
	{
		this->vr_lastHMDOffset = g_vrLevelChangeData.lastHMDOffset;
		this->vr_ClientOriginOffset = g_vrLevelChangeData.clientOriginOffset;
		this->vr_prevYaw = g_vrLevelChangeData.prevYaw;
		this->vr_currentYaw = g_vrLevelChangeData.currentYaw;
		g_vrLevelChangeData.hasData = false;
		vr_needsToSendRestoreYawMsgToClient = true;
	}

	pev->v_angle.z = 0;  // Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;  // turn this way immediately

	// Copied from spawn() for now
	m_bloodColor = BLOOD_COLOR_RED;

	g_ulModelIndexPlayer = pev->modelindex;

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	if (m_fLongJump)
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "1");
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	}

	RenewItems();

#if defined(CLIENT_WEAPONS)
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif

	// Make sure old predisaster savegames start up with hand model
	if (m_pActiveItem == nullptr)
	{
		if (HasSuit())
		{
			pev->viewmodel = MAKE_STRING("models/v_hand_hevsuit.mdl");
		}
		else
		{
			pev->viewmodel = MAKE_STRING("models/v_hand_labcoat.mdl");
		}
		pev->weaponmodel = iStringNull;
	}

	vr_IsJustRestored = true;
	vr_hasSentRestoreYawMsgToClient = false;
	vr_hasSentSpawnYawToClient = false;

	return status;
}



/*
void CBasePlayer::SelectNextItem( int iItem )
{
	CBasePlayerItem *pItem;

	pItem = m_rgpPlayerItems[ iItem ];

	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext;
		if (! pItem)
		{
			return;
		}

		CBasePlayerItem *pLast;
		pLast = pItem;
		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = nullptr;
		m_rgpPlayerItems[ iItem ] = pItem;
	}

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster( );
	}

	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
		m_pActiveItem->UpdateItemInfo( );
	}
}
*/

void CBasePlayer::SelectItem(const char* pstr)
{
	if (pstr == nullptr)
	{
		return;
	}

	CBasePlayerItem* pItem = nullptr;

	if (FStrEq("weapon_barehand", pstr))
	{
		pItem = nullptr;
	}
	else
	{
		for (int i = 0; i < MAX_ITEM_TYPES; i++)
		{
			if (m_rgpPlayerItems[i])
			{
				pItem = m_rgpPlayerItems[i];
				while (pItem != nullptr)
				{
					if (FClassnameIs(pItem->pev, pstr))
					{
						break;
					}
					pItem = pItem->m_pNext;
				}
			}
			if (pItem != nullptr)
			{
				break;
			}
		}
	}

	if (pItem == m_pActiveItem)
	{
		return;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem != nullptr)
	{
		m_pActiveItem->Holster();
	}

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem != nullptr)
	{
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
	else
	{
		if (HasSuit())
		{
			pev->viewmodel = MAKE_STRING("models/v_hand_hevsuit.mdl");
		}
		else
		{
			pev->viewmodel = MAKE_STRING("models/v_hand_labcoat.mdl");
		}
		pev->weaponmodel = iStringNull;
	}
}


void CBasePlayer::SelectLastItem(void)
{
	if (m_pActiveItem != nullptr && !m_pActiveItem->CanHolster())
	{
		return;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem != nullptr)
	{
		m_pActiveItem->Holster();
	}

	if (m_pLastItem == nullptr)
	{
		m_pLastItem = m_pActiveItem;
		m_pActiveItem = nullptr;
		if (HasSuit())
		{
			pev->viewmodel = MAKE_STRING("models/v_hand_hevsuit.mdl");
		}
		else
		{
			pev->viewmodel = MAKE_STRING("models/v_hand_labcoat.mdl");
		}
		pev->weaponmodel = iStringNull;
	}
	else
	{
		CBasePlayerItem* pTemp = m_pActiveItem;
		m_pActiveItem = m_pLastItem;
		m_pLastItem = pTemp;
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
BOOL CBasePlayer::HasWeapons(void)
{
	int i = 0;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			return TRUE;
		}
	}

	return FALSE;
}

/*
void CBasePlayer::SelectPrevItem( int iItem )
{
}
*/


const char* CBasePlayer::TeamID(void)
{
	if (pev == nullptr)  // Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void Spawn(entvars_t* pevOwner);
	void Think(void);

	virtual int ObjectCaps(void) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn(entvars_t* pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
}

void CSprayCan::Think(void)
{
	TraceResult tr;
	int playernum = 0;
	int nFrames = 0;
	CBasePlayer* pPlayer;

	pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);

	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace(&tr, DECAL_LAMBDA6);
		UTIL_Remove(this);
	}
	else
	{
		UTIL_PlayerDecalTrace(&tr, playernum, pev->frame, TRUE);
		// Just painted last custom frame.
		if (pev->frame++ >= (nFrames - 1))
			UTIL_Remove(this);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn(entvars_t* pevOwner);
	void Spray(void);
};

void CBloodSplat::Spawn(entvars_t* pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink(&CBloodSplat::Spray);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray(void)
{
	TraceResult tr;

	if (g_Language != LANGUAGE_GERMAN)
	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
	}
	SetThink(&CBloodSplat::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================



void CBasePlayer::GiveNamedItem(const char* pszName)
{
	edict_t* pent;

	int istr = MAKE_STRING(pszName);

	pent = CREATE_NAMED_ENTITY(istr);
	if (FNullEnt(pent))
	{
		ALERT(at_console, "nullptr Ent in GiveNamedItem!\n");
		return;
	}
	VARS(pent)->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn(pent);
	DispatchTouch(pent, ENT(pev));
}



CBaseEntity* FindEntityForward(CBaseEntity* pMe)
{
	TraceResult tr;

	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs, pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, pMe->edict(), &tr);
	if (tr.flFraction < 1.f && !FNullEnt(tr.pHit))
	{
		return CBaseEntity::SafeInstance<CBaseEntity>(tr.pHit);
	}
	return nullptr;
}


BOOL CBasePlayer::FlashlightIsOn(void)
{
	//return FBitSet(pev->effects, EF_DIMLIGHT);
	return fFlashlightIsOn;
}


void CBasePlayer::FlashlightTurnOn(void)
{
	if (!g_pGameRules->FAllowFlashlight())
	{
		return;
	}

	if ((pev->weapons & (1 << WEAPON_SUIT)))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM);

		//SetBits(pev->effects, EF_DIMLIGHT);
		if (!hFlashLight)
		{
			hFlashLight = CBaseEntity::Create<CBaseEntity>("info_target", pev->origin, Vector());
			SET_MODEL(hFlashLight->edict(), "sprites/black.spr");
			UTIL_SetOrigin(hFlashLight->pev, pev->origin);
			UTIL_SetSize(hFlashLight->pev, Vector(-4, -4, -4), Vector(4, 4, 4));
			hFlashLight->pev->effects = EF_NODRAW;
			hFlashLight->pev->rendermode = kRenderTransAlpha;
		}
		fFlashlightIsOn = true;

		MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, nullptr, pev);
		WRITE_BYTE(1);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
	}
}


void CBasePlayer::FlashlightTurnOff(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM);

	//ClearBits(pev->effects, EF_DIMLIGHT);
	if (hFlashLight)
	{
		hFlashLight->pev->effects = EF_NODRAW;
		UTIL_Remove(hFlashLight);
		hFlashLight = nullptr;
	}
	if (hFlashlightMonster)
	{
		ClearBits(hFlashlightMonster->pev->effects, EF_DIMLIGHT);
		hFlashlightMonster = nullptr;
	}
	fFlashlightIsOn = false;

	MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, nullptr, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(m_iFlashBattery);
	MESSAGE_END();

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer::ForceClientDllUpdate(void)
{
	m_iClientHealth = -1;
	m_iClientBattery = -1;
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = FALSE;   // Force weapon send
	m_fKnownItem = FALSE;   // Force weaponinit messages.
	m_fInitHUD = TRUE;    // Force HUD gmsgResetHUD message

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
extern bool AreCheatsEnabled();

void CBasePlayer::ImpulseCommands()
{
	TraceResult tr;  // UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();

	int iImpulse = (int)pev->impulse;
	switch (iImpulse)
	{
	case 99:
	{
		int iOn = 0;

		if (!gmsgLogo)
		{
			iOn = 1;
			gmsgLogo = REG_USER_MSG("Logo", 1);
		}
		else
		{
			iOn = 0;
		}

		ASSERT(gmsgLogo > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgLogo, nullptr, pev);
		WRITE_BYTE(iOn);
		MESSAGE_END();

		if (!iOn)
			gmsgLogo = 0;
		break;
	}
	case 100:
		// temporary flashlight for level designers
		if (FlashlightIsOn())
		{
			FlashlightTurnOff();
		}
		else
		{
			FlashlightTurnOn();
		}
		break;
	case 201:  // paint decal

		if (gpGlobals->time < m_flNextDecalTime)
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{  // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan* pCan = GetClassPtr<CSprayCan>(nullptr);
			pCan->Spawn(pev);
		}

		break;

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands(iImpulse);
		break;
	}

	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
#if !defined(HLDEMO_BUILD)
	if (!AreCheatsEnabled())
	{
		return;
	}

	CBaseEntity* pEntity{ nullptr };
	TraceResult tr;

	switch (iImpulse)
	{
	case 76:
	{
		if (!giPrecacheGrunt)
		{
			giPrecacheGrunt = 1;
			ALERT(at_console, "You must now restart to use Grunt-o-matic.\n");
		}
		else
		{
			UTIL_MakeVectors(Vector(0, pev->v_angle.y, 0));
			CBaseEntity::Create<CBaseEntity>("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
		}
		break;
	}


	case 101:
		gEvilImpulse101 = TRUE;
		// all the items
		GiveNamedItem("item_suit");
		GiveNamedItem("item_battery");
		GiveNamedItem("item_longjump");

		// all the weapons
		GiveNamedItem("weapon_crowbar");
		GiveNamedItem("weapon_9mmhandgun");
		GiveNamedItem("weapon_shotgun");
		GiveNamedItem("weapon_9mmAR");
		GiveNamedItem("weapon_357");
		GiveNamedItem("weapon_crossbow");
		GiveNamedItem("weapon_egon");
		GiveNamedItem("weapon_gauss");
		GiveNamedItem("weapon_rpg");
		GiveNamedItem("weapon_hornetgun");

		// all the grenades, and i mean /all/ the grenades
		for (int i = 0; i < (SATCHEL_MAX_CARRY / SATCHEL_DEFAULT_GIVE); i++)
			GiveNamedItem("weapon_satchel");
		for (int i = 0; i < (SNARK_MAX_CARRY / SNARK_DEFAULT_GIVE); i++)
			GiveNamedItem("weapon_snark");
		for (int i = 0; i < (HANDGRENADE_MAX_CARRY / HANDGRENADE_DEFAULT_GIVE); i++)
			GiveNamedItem("weapon_handgrenade");
		for (int i = 0; i < (TRIPMINE_MAX_CARRY / TRIPMINE_DEFAULT_GIVE); i++)
			GiveNamedItem("weapon_tripmine");

		// all the ammo, and i mean /all/ the ammo
		for (int i = 0; i < (URANIUM_MAX_CARRY / AMMO_URANIUMBOX_GIVE); i++)
			GiveNamedItem("ammo_gaussclip");
		for (int i = 0; i < (ROCKET_MAX_CARRY / AMMO_RPGCLIP_GIVE); i++)
			GiveNamedItem("ammo_rpgclip");
		for (int i = 0; i < (BOLT_MAX_CARRY / AMMO_CROSSBOWCLIP_GIVE); i++)
			GiveNamedItem("ammo_crossbow");
		for (int i = 0; i < (_357_MAX_CARRY / AMMO_357BOX_GIVE); i++)
			GiveNamedItem("ammo_357");
		for (int i = 0; i < (_9MM_MAX_CARRY / AMMO_MP5CLIP_GIVE); i++)
			GiveNamedItem("ammo_9mmAR");
		for (int i = 0; i < (BUCKSHOT_MAX_CARRY / AMMO_BUCKSHOTBOX_GIVE); i++)
			GiveNamedItem("ammo_buckshot");
		for (int i = 0; i < (M203_GRENADE_MAX_CARRY / AMMO_M203BOX_GIVE); i++)
			GiveNamedItem("ammo_ARgrenades");
		gEvilImpulse101 = FALSE;
		break;

	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs(pev, 1, 1);
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward(this);
		if (pEntity)
		{
			CBaseMonster* pMonster = dynamic_cast<CBaseMonster*>(pEntity);
			if (pMonster)
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case 105:  // player makes no sound for monsters to hear.
	{
		if (m_fNoPlayerSound)
		{
			ALERT(at_console, "Player is audible\n");
			m_fNoPlayerSound = FALSE;
		}
		else
		{
			ALERT(at_console, "Player is silent\n");
			m_fNoPlayerSound = TRUE;
		}
		break;
	}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward(this);
		if (pEntity)
		{
			ALERT(at_console, "Classname: %s", STRING(pEntity->pev->classname));

			if (!FStringNull(pEntity->pev->targetname))
			{
				ALERT(at_console, " - Targetname: %s\n", STRING(pEntity->pev->targetname));
			}
			else
			{
				ALERT(at_console, " - TargetName: No Targetname\n");
			}

			ALERT(at_console, "Model: %s\n", STRING(pEntity->pev->model));
			if (pEntity->pev->globalname)
				ALERT(at_console, "Globalname: %s\n", STRING(pEntity->pev->globalname));
		}
		break;

	case 107:
	{
		TraceResult tr;

		edict_t* pWorld = g_engfuncs.pfnPEntityOfEntIndex(0);

		Vector start = EyePosition();
		Vector end = start + gpGlobals->v_forward * 1024;
		UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
		if (tr.pHit)
			pWorld = tr.pHit;
		const char* pTextureName = TRACE_TEXTURE(pWorld, start, end);
		if (pTextureName)
			ALERT(at_console, "Texture: %s\n", pTextureName);
	}
	break;
	case 195:  // show shortest paths for entire level to nearest node
	{
		CBaseEntity::Create<CBaseEntity>("node_viewer_fly", pev->origin, pev->angles);
	}
	break;
	case 196:  // show shortest paths for entire level to nearest node
	{
		CBaseEntity::Create<CBaseEntity>("node_viewer_large", pev->origin, pev->angles);
	}
	break;
	case 197:  // show shortest paths for entire level to nearest node
	{
		CBaseEntity::Create<CBaseEntity>("node_viewer_human", pev->origin, pev->angles);
	}
	break;
	case 199:  // show nearest node and all connections
	{
		ALERT(at_console, "%d\n", WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
		WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
	}
	break;
	case 202:  // Random blood splatter
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, EyePosition() + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{  // line hit something, so paint a decal
			CBloodSplat* pBlood = GetClassPtr<CBloodSplat>(nullptr);
			pBlood->Spawn(pev);
		}
		break;
	case 203:  // remove creature.
		pEntity = FindEntityForward(this);
		if (pEntity)
		{
			if (pEntity->pev->takedamage)
				pEntity->SetThink(&CBaseEntity::SUB_Remove);
		}
		break;
	}
#endif  // HLDEMO_BUILD
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
int CBasePlayer::AddPlayerItem(CBasePlayerItem* pItem)
{
	CBasePlayerItem* pInsert;

	pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while (pInsert)
	{
		if (FClassnameIs(pInsert->pev, STRING(pItem->pev->classname)))
		{
			if (pItem->AddDuplicate(pInsert))
			{
				g_pGameRules->PlayerGotWeapon(this, pItem);
				pItem->CheckRespawn();

				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo();
				if (m_pActiveItem)
					m_pActiveItem->UpdateItemInfo();

				pItem->Kill();
			}
			else if (gEvilImpulse101)
			{
				// FIXME: remove anyway for deathmatch testing
				pItem->Kill();
			}
			return FALSE;
		}
		pInsert = pInsert->m_pNext;
	}


	if (pItem->AddToPlayer(this))
	{
		g_pGameRules->PlayerGotWeapon(this, pItem);
		pItem->CheckRespawn();

		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		// should we switch to this item?
		if (g_pGameRules->FShouldSwitchWeapon(this, pItem))
		{
			SwitchWeapon(pItem);
		}

		return TRUE;
	}
	else if (gEvilImpulse101)
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill();
	}
	return FALSE;
}



int CBasePlayer::RemovePlayerItem(CBasePlayerItem* pItem)
{
	if (m_pActiveItem == pItem)
	{
		ResetAutoaim();
		pItem->Holster();
		pItem->pev->nextthink = 0;  // crowbar may be trying to swing again, etc.
		pItem->SetThink(nullptr);
		m_pActiveItem = nullptr;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	else if (m_pLastItem == pItem)
		m_pLastItem = nullptr;

	CBasePlayerItem* pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if (pPrev == pItem)
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		return TRUE;
	}
	else
	{
		while (pPrev && pPrev->m_pNext != pItem)
		{
			pPrev = pPrev->m_pNext;
		}
		if (pPrev)
		{
			pPrev->m_pNext = pItem->m_pNext;
			return TRUE;
		}
	}
	return FALSE;
}


//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer::GiveAmmo(int iCount, const char* szName, int iMax, int* pIndex /*= nullptr*/)
{
	if (!szName)
	{
		// no ammo.
		return -1;
	}

	if (!g_pGameRules->CanHaveAmmo(this, szName, iMax))
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = 0;

	i = GetAmmoIndex(szName);

	if (i < 0 || i >= MAX_AMMO_SLOTS)
		return -1;

	int iAdd = min(iCount, iMax - m_rgAmmo[i]);
	if (iAdd < 1)
		return i;

	m_rgAmmo[i] += iAdd;


	if (gmsgAmmoPickup)  // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoPickup, nullptr, pev);
		WRITE_BYTE(GetAmmoIndex(szName));  // ammo ID
		WRITE_BYTE(iAdd);                  // amount
		MESSAGE_END();
	}

	TabulateAmmo();

	return i;
}


/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
#if defined(CLIENT_WEAPONS)
	if (m_flNextAttack > 0)
#else
	if (gpGlobals->time < m_flNextAttack)
#endif
	{
		return;
	}

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPreFrame();
}


/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	static int fInSelect = FALSE;

	// check if the player is using a tank
	if (m_pTank != nullptr)
	{
		return;
	}

	if (m_iHideHUD & HIDEHUD_WEAPONBLOCKED)
	{
		return;
	}

#if defined(CLIENT_WEAPONS)
	if (m_flNextAttack > 0)
#else
	if (gpGlobals->time < m_flNextAttack)
#endif
	{
		return;
	}

	ImpulseCommands();

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPostFrame();
}

int CBasePlayer::AmmoInventory(int iAmmoIndex)
{
	if (iAmmoIndex == -1)
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
}

int CBasePlayer::GetAmmoIndex(const char* psz)
{
	int i = 0;

	if (!psz)
		return -1;

	for (i = 1; i < MAX_AMMO_SLOTS; i++)
	{
		if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if (_stricmp(psz, CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
			return i;
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate(void)
{
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (m_rgAmmo[i] != m_rgAmmoLast[i])
		{
			m_rgAmmoLast[i] = m_rgAmmo[i];

			ASSERT(m_rgAmmo[i] >= 0);
			ASSERT(m_rgAmmo[i] < 255);

			// send "Ammo" update message
			MESSAGE_BEGIN(MSG_ONE, gmsgAmmoX, nullptr, pev);
			WRITE_BYTE(i);
			WRITE_BYTE(max(min(m_rgAmmo[i], 254), 0));  // clamp the value to one byte
			MESSAGE_END();
		}
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData(void)
{
	if (m_fInitHUD)
	{
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;

		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, nullptr, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		if (!m_fGameHUDInitialized && g_pGameRules)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, nullptr, pev);
			MESSAGE_END();

			g_pGameRules->InitHUD(this);
			m_fGameHUDInitialized = TRUE;
			if (g_pGameRules->IsMultiplayer())
			{
				FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);
			}
		}

		FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);

		InitStatusBar();
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, nullptr, pev);
		WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if (m_iFOV != m_iClientFOV)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, nullptr, pev);
		WRITE_BYTE(m_iFOV);
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if (gDisplayTitle)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgShowGameTitle, nullptr, pev);
		WRITE_BYTE(0);
		MESSAGE_END();
		gDisplayTitle = 0;
	}

	if (pev->health != m_iClientHealth)
	{
		int iHealth = max(pev->health, 0);  // make sure that no negative health values are sent

		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgHealth, nullptr, pev);
		WRITE_BYTE(iHealth);
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}


	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		ASSERT(gmsgBattery > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgBattery, nullptr, pev);
		WRITE_SHORT((int)pev->armorvalue);
		MESSAGE_END();
	}

	if (pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t* other = pev->dmg_inflictor;
		if (other)
		{
			CBaseEntity* pEntity = CBaseEntity::SafeInstance<CBaseEntity>(other);
			if (pEntity)
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN(MSG_ONE, gmsgDamage, nullptr, pev);
		WRITE_BYTE(pev->dmg_save);
		WRITE_BYTE(pev->dmg_take);
		WRITE_LONG(visibleDamageBits);
		WRITE_COORD(damageOrigin.x);
		WRITE_COORD(damageOrigin.y);
		WRITE_COORD(damageOrigin.z);
		MESSAGE_END();

		// TODO: VR: gmsgDamage seems broken/don't want to mess with it.
		// TODO: VR: Send proper damage message with all bits for VR damage feedback
		// TODO: VR: - Max Vollmer, 2019-04-06

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	// Update Flashlight
	if ((m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->time))
	{
		if (FlashlightIsOn())
		{
			if (m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				m_iFlashBattery--;

				if (!m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgFlashBattery, nullptr, pev);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();
	}

	if (m_afPhysicsFlags & PFLAG_ONTRAIN)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgVRTrainControls, nullptr, pev);
		WRITE_COORD(vr_trainControlPosition.x);
		WRITE_COORD(vr_trainControlPosition.y);
		WRITE_COORD(vr_trainControlPosition.z);
		WRITE_ANGLE(vr_trainControlYaw);
		MESSAGE_END();
	}

	if (m_iTrain & TRAIN_NEW)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgTrain, nullptr, pev);
		WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();
		m_iTrain &= ~TRAIN_NEW;
	}

	//
	// New Weapon?
	//
	if (!m_fKnownItem)
	{
		m_fKnownItem = TRUE;

		// WeaponInit Message
		// byte  = # of weapons
		//
		// for each weapon:
		// byte		name str length (not including null)
		// bytes... name
		// byte		Ammo Type
		// byte		Ammo2 Type
		// byte		bucket
		// byte		bucket pos
		// byte		flags
		// ????		Icons

		// Send ALL the weapon info now
		int i = 0;

		for (i = 0; i < MAX_WEAPONS; i++)
		{
			ItemInfo& II = CBasePlayerItem::ItemInfoArray[i];

			if (!II.iId)
				continue;

			const char* pszName;
			if (!II.pszName)
				pszName = "Empty";
			else
				pszName = II.pszName;

			MESSAGE_BEGIN(MSG_ONE, gmsgWeaponList, nullptr, pev);
			WRITE_STRING(pszName);                  // string	weapon name
			WRITE_BYTE(GetAmmoIndex(II.pszAmmo1));  // byte		Ammo Type
			WRITE_BYTE(II.iMaxAmmo1);               // byte     Max Ammo 1
			WRITE_BYTE(GetAmmoIndex(II.pszAmmo2));  // byte		Ammo2 Type
			WRITE_BYTE(II.iMaxAmmo2);               // byte     Max Ammo 2
			WRITE_BYTE(II.iSlot);                   // byte		bucket
			WRITE_BYTE(II.iPosition);               // byte		bucket pos
			WRITE_BYTE(II.iId);                     // byte		id (bit index into pev->weapons)
			WRITE_BYTE(II.iFlags);                  // byte		Flags
			MESSAGE_END();
		}
	}


	SendAmmoUpdate();

	// Update all the items
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])  // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData(this);
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	if (m_flNextSBarUpdateTime < gpGlobals->time)
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}

	if (vr_needsToSendRestoreYawMsgToClient)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgVRRestoreYaw, nullptr, pev);
		WRITE_ANGLE(this->vr_prevYaw);
		WRITE_ANGLE(this->vr_currentYaw);
		MESSAGE_END();
		vr_hasSentRestoreYawMsgToClient = true;
		vr_needsToSendRestoreYawMsgToClient = false;
	}
}


//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer::FBecomeProne(void)
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	pev->flags |= FL_BARNACLED;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer::BarnacleVictimBitten(entvars_t* pevBarnacle)
{
	TakeDamage(pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB);
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns.
//=========================================================
void CBasePlayer::BarnacleVictimReleased(void)
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
	pev->flags &= ~FL_BARNACLED;
}


//=========================================================
// Illumination
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer::Illumination(void)
{
	int iIllum = CBaseEntity::Illumination();

	iIllum += m_iWeaponFlash;
	if (iIllum > 255)
		return 255;
	return iIllum;
}


void CBasePlayer::EnableControl(BOOL fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}


#define DOT_1DEGREE  0.9998476951564
#define DOT_2DEGREE  0.9993908270191
#define DOT_3DEGREE  0.9986295347546
#define DOT_4DEGREE  0.9975640502598
#define DOT_5DEGREE  0.9961946980917
#define DOT_6DEGREE  0.9945218953683
#define DOT_7DEGREE  0.9925461516413
#define DOT_8DEGREE  0.9902680687416
#define DOT_9DEGREE  0.9876883405951
#define DOT_10DEGREE 0.9848077530122
#define DOT_15DEGREE 0.9659258262891
#define DOT_20DEGREE 0.9396926207859
#define DOT_25DEGREE 0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::AutoaimDeflection(Vector& vecSrc, float flDist, float flDelta)
{
	// no auto aim in VR
	return Vector{};
}

void CBasePlayer::ResetAutoaim()
{
	// no auto aim in VR
}


Vector CBasePlayer::GetGunPosition()
{
	// Gun position and angles determined by attachments on weapon model - Max Vollmer, 2019-03-30
	if (m_vrControllers[VRControllerID::WEAPON].IsValid())
	{
		Vector pos;
		bool result = m_vrControllers[VRControllerID::WEAPON].GetAttachment(VR_MUZZLE_ATTACHMENT, pos);
		if (result)
		{
			return pos;
		}
	}

	return GetWeaponPosition();
}

Vector CBasePlayer::GetAimAngles()
{
	return UTIL_VecToAngles(GetAutoaimVector());
}

Vector CBasePlayer::GetAutoaimVector(float flDelta)
{
	// Gun position and angles determined by attachments on weapon model - Max Vollmer, 2019-03-30
	if (m_vrControllers[VRControllerID::WEAPON].IsValid())
	{
		Vector pos1;
		Vector pos2;
		bool result1 = m_vrControllers[VRControllerID::WEAPON].GetAttachment(VR_MUZZLE_ATTACHMENT, pos1);
		bool result2 = m_vrControllers[VRControllerID::WEAPON].GetAttachment(VR_MUZZLE_ATTACHMENT + 1, pos2);
		if (result1 && result2 && pos2 != pos1)
		{
			return (pos2 - pos1).Normalize();
		}
	}
	UTIL_MakeAimVectors(GetWeaponAngles());
	return gpGlobals->v_forward;
}


/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer::GetCustomDecalFrames(void)
{
	return m_nCustomSprayFrames;
}


//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item.
//=========================================================
void CBasePlayer::DropPlayerItem(const char* pszItemName)
{
	if (!g_pGameRules->IsMultiplayer() || (weaponstay.value > 0))
	{
		// no dropping in single player.
		return;
	}

	if (!strlen(pszItemName))
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = nullptr;
	}

	CBasePlayerItem* pWeapon;
	int i = 0;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pWeapon = m_rgpPlayerItems[i];

		while (pWeapon)
		{
			if (pszItemName)
			{
				// try to match by name.
				if (!strcmp(pszItemName, STRING(pWeapon->pev->classname)))
				{
					// match!
					break;
				}
			}
			else
			{
				// trying to drop active item
				if (pWeapon == m_pActiveItem)
				{
					// active item!
					break;
				}
			}

			pWeapon = pWeapon->m_pNext;
		}


		// if we land here with a valid pWeapon pointer, that's because we found the
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if (pWeapon)
		{
			g_pGameRules->GetNextBestWeapon(this, pWeapon);

			UTIL_MakeVectors(pev->angles);

			pev->weapons &= ~(1 << pWeapon->m_iId);  // take item off hud

			CWeaponBox* pWeaponBox = CBaseEntity::Create<CWeaponBox>("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->PackWeapon(pWeapon);
			pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

			// drop half of the ammo for this weapon.
			int iAmmoIndex = 0;

			iAmmoIndex = GetAmmoIndex(pWeapon->pszAmmo1());  // ???

			if (iAmmoIndex != -1)
			{
				// this weapon weapon uses ammo, so pack an appropriate amount.
				if (pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE)
				{
					// pack up all the ammo, this weapon is its own ammo type
					pWeaponBox->PackAmmo(MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[iAmmoIndex]);
					m_rgAmmo[iAmmoIndex] = 0;
				}
				else
				{
					// pack half of the ammo
					pWeaponBox->PackAmmo(MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[iAmmoIndex] / 2);
					m_rgAmmo[iAmmoIndex] /= 2;
				}
			}

			return;  // we're done, so stop searching with the FOR loop.
		}
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasPlayerItem(CBasePlayerItem* pCheckItem)
{
	CBasePlayerItem* pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs(pItem->pev, STRING(pCheckItem->pev->classname)))
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasNamedPlayerItem(const char* pszItemName)
{
	CBasePlayerItem* pItem;
	int i = 0;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pItem = m_rgpPlayerItems[i];

		while (pItem)
		{
			if (!strcmp(pszItemName, STRING(pItem->pev->classname)))
			{
				return TRUE;
			}
			pItem = pItem->m_pNext;
		}
	}

	return FALSE;
}

//=========================================================
//
//=========================================================
BOOL CBasePlayer::SwitchWeapon(CBasePlayerItem* pWeapon)
{
	if (!pWeapon->CanDeploy())
	{
		return FALSE;
	}

	ResetAutoaim();

	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pWeapon;
	pWeapon->Deploy();

	return TRUE;
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn(void);
	int Classify(void) { return CLASS_HUMAN_MILITARY; }

	void KeyValue(KeyValueData* pkvd);

	int m_iPose = 0;  // which sequence to display	-- temporary, don't need to save
	static char* m_szPoses[4];
};

char* CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

void CDeadHEV::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_hevsuit_dead, CDeadHEV);

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV::Spawn(void)
{
	PRECACHE_MODEL("models/player.mdl");
	SET_MODEL(ENT(pev), "models/player.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hevsuit with bad pose\n");
		pev->sequence = 0;
		// pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}


class CStripWeapons : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

private:
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

void CStripWeapons::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = nullptr;

	if (pActivator && pActivator->IsPlayer())
	{
		pPlayer = dynamic_cast<CBasePlayer*>(pActivator);
	}
	else if (!g_pGameRules->IsDeathmatch())
	{
		pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(g_engfuncs.pfnPEntityOfEntIndex(1));
	}

	if (pPlayer)
		pPlayer->RemoveAllItems(FALSE);
}


class CRevertSaved : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT MessageThink(void);
	void EXPORT LoadThink(void);
	void KeyValue(KeyValueData* pkvd);

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];

	inline float Duration(void) { return pev->dmg_take; }
	inline float HoldTime(void) { return pev->dmg_save; }
	inline float MessageTime(void) { return m_messageTime; }
	inline float LoadTime(void) { return m_loadTime; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline void SetMessageTime(float time) { m_messageTime = time; }
	inline void SetLoadTime(float time) { m_loadTime = time; }

private:
	float m_messageTime = 0.f;
	float m_loadTime = 0.f;
};

LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

TYPEDESCRIPTION CRevertSaved::m_SaveData[] =
{
	DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT),  // These are not actual times, but durations, so save as floats
	DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRevertSaved, CPointEntity);

void CRevertSaved::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CRevertSaved::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}


void CRevertSaved::MessageThink(void)
{
	UTIL_ShowMessageAll(STRING(pev->message));
	float nextThink = LoadTime() - MessageTime();
	if (nextThink > 0)
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}


void CRevertSaved::LoadThink(void)
{
	if (!gpGlobals->deathmatch)
	{
		SERVER_COMMAND("reload\n");
	}
}


//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
	void Spawn(void);
	void Think(void);
};

void CInfoIntermission::Spawn(void)
{
	UTIL_SetOrigin(pev, pev->origin);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2;  // let targets spawn!
}

void CInfoIntermission::Think(void)
{
	edict_t* pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME(nullptr, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = UTIL_VecToAngles((pTarget->v.origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);



// VR methods:

void CBasePlayer::UpdateVRHeadset(const int timestamp, const Vector2D& hmdOffset, const float hmdOffsetZ, const Vector& hmdForward, const Vector2D& hmdYawOffsetDelta, float prevYaw, float currentYaw, bool hasReceivedRestoreYawMsg, bool hasReceivedSpawnYaw)
{
	// Filter out outdated updates
	if (timestamp <= vr_hmdLastUpdateClienttime && vr_hmdLastUpdateServertime >= gpGlobals->time)
	{
		return;
	}

	// Ignore all updates until client has processed yaw update (fixes timing issues)
	if (vr_needsToSendRestoreYawMsgToClient)
	{
		return;
	}
	if (vr_hasSentRestoreYawMsgToClient && !hasReceivedRestoreYawMsg)
	{
		return;
	}
	vr_hasSentRestoreYawMsgToClient = false;

	Vector TEMPDEBUG_originBefore = pev->origin;

	// Calculate view dir for view position offset
	Vector viewDir2D;
	UTIL_MakeVectorsPrivate(Vector{ 0.f, pev->angles.y, 0.f }, viewDir2D, nullptr, nullptr);
	viewDir2D.z = 0.f;
	viewDir2D = viewDir2D.Normalize();

	// Get view dir with length to hull bounds
	float vrViewOffsetDistToHullBounds = CVAR_GET_FLOAT("vr_view_dist_to_walls");
	Vector viewDirToHullBounds = viewDir2D * (VEC_HULL_MAX.x - vrViewOffsetDistToHullBounds);

	// If we just spawned we need to make sure that the HMD offset gets moved into the spawn position
	// Thus we set vr_lastHMDOffset to the current HMD offset,
	// and use the current HMD offset to set the client origin offset.
	// We also send the initial spawn yaw down to the client so it adjusts and looks in the direction of the spawn spot.
	// - Max Vollmer, 2019-04-13
	if (vr_IsJustSpawned)
	{
		if (!vr_hasSentSpawnYawToClient)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgVRSetSpawnYaw, nullptr, pev);
			WRITE_ANGLE(vr_spawnYaw);
			MESSAGE_END();
			vr_spawnYaw = 0.f;
			vr_hasSentSpawnYawToClient = true;
			return;
		}
		if (vr_hasSentSpawnYawToClient && !hasReceivedSpawnYaw)
		{
			return;
		}
		vr_hasSentSpawnYawToClient = false;

		vr_lastHMDOffset = Vector{ hmdOffset.x, hmdOffset.y, 0.f };
		vr_ClientOriginOffset = Vector{ -hmdOffset.x, -hmdOffset.y, 0.f } +viewDirToHullBounds;
		vr_IsJustSpawned = false;

		return;
	}

	// We probably don't need this?
	if (vr_IsJustRestored)
	{
		//vr_ClientOriginOffset = vr_ClientOriginOffset + viewDir2D * VEC_HULL_MAX.x;
		vr_IsJustRestored = false;
	}

	vr_prevYaw = prevYaw;
	vr_currentYaw = currentYaw;

	vr_hmdLastUpdateClienttime = timestamp;
	vr_hmdLastUpdateServertime = gpGlobals->time;

	// First get origin where the client thinks it is:
	Vector clientOrigin = GetClientOrigin();

	// Add rotation offset delta into client origin (yaw rotates around play area center, so we need to adjust this here):
	clientOrigin = clientOrigin - hmdYawOffsetDelta;

	// Then get headset position:
	Vector newOrigin = clientOrigin + hmdOffset;

	// push origin back so that view position is on border of player's bounding box
	newOrigin = newOrigin - viewDirToHullBounds;

	// Use movement handler to move to new position (instead of simply teleporting)
	// Uses pm_shared code. Allows for climbing up stairs and handling all kinds of collisions with level geometry.
	pev->origin = VRMovementHandler::DoMovement(pev->origin, newOrigin, this);

	// Always add in newOrigin instead of pev->origin, creates much better and smoother results
	vr_ClientOriginOffset.x = clientOrigin.x - newOrigin.x;
	vr_ClientOriginOffset.y = clientOrigin.y - newOrigin.y;

	// Remember offset for wallcheck next call
	vr_lastHMDOffset.x = hmdOffset.x;
	vr_lastHMDOffset.y = hmdOffset.y;

	// HMD height and HMD direction for view_ofs and viewdir for looking at and interaction with stuff
	pev->view_ofs.z = pev->mins.z + hmdOffsetZ;
	vr_hmdForward = hmdForward;
	vr_hmdForward.InlineNormalize();
}

void CBasePlayer::UpdateVRController(const VRControllerID vrControllerID, const int timestamp, const bool isValid, const bool isMirrored, const Vector& offset, const Vector& angles, const Vector& velocity, bool isDragging)
{
	int weaponId = WEAPON_BAREHAND;
	if (vrControllerID == VRControllerID::WEAPON)
	{
		ItemInfo itemInfo = {};
		if (m_pActiveItem != nullptr && m_pActiveItem->GetItemInfo(&itemInfo))
		{
			weaponId = itemInfo.iId;
		}
	}
	m_vrControllers[vrControllerID].Update(this, timestamp, isValid, isMirrored, offset, angles, velocity, isDragging, vrControllerID, weaponId);
}

const Vector CBasePlayer::GetWeaponPosition()
{
	if (m_vrControllers[VRControllerID::WEAPON].IsValid())
	{
		return GetClientOrigin() + m_vrControllers[VRControllerID::WEAPON].GetOffset();
	}
	else
	{
		return EyePosition();
	}
}
const Vector CBasePlayer::GetWeaponAngles()
{
	if (m_vrControllers[VRControllerID::WEAPON].IsValid())
	{
		return m_vrControllers[VRControllerID::WEAPON].GetAngles();
	}
	else
	{
		return UTIL_VecToAngles(vr_hmdForward);
	}
}
const Vector CBasePlayer::GetWeaponViewAngles()
{
	Vector angles = GetWeaponAngles();
	angles.x = -angles.x;
	return angles;
}
const Vector CBasePlayer::GetWeaponVelocity()
{
	if (m_vrControllers[VRControllerID::WEAPON].IsValid())
	{
		return m_vrControllers[VRControllerID::WEAPON].GetVelocity();
	}
	else
	{
		return Vector{};
	}
}
const Vector CBasePlayer::GetClientOrigin()
{
	return Vector(pev->origin.x + vr_ClientOriginOffset.x, pev->origin.y + vr_ClientOriginOffset.y, pev->origin.z);
}
const Vector CBasePlayer::GetClientViewOfs()
{
	return Vector((pev->origin + pev->view_ofs) - GetClientOrigin());
}
bool CBasePlayer::IsWeaponUnderWater()
{
	return UTIL_PointContents(GetWeaponPosition()) == CONTENTS_WATER || UTIL_PointContents(GetGunPosition()) == CONTENTS_WATER;
}
bool CBasePlayer::IsWeaponPositionValid()
{
	int weaponOriginContent = UTIL_PointContents(GetWeaponPosition(), true);
	int weaponMuzzleContent = UTIL_PointContents(GetGunPosition(), true);
	return (weaponOriginContent == CONTENTS_EMPTY || weaponOriginContent == CONTENTS_WATER) && (weaponMuzzleContent == CONTENTS_EMPTY || weaponMuzzleContent == CONTENTS_WATER);
}

#define CROWBAR_BODYHIT_VOLUME 128
#define CROWBAR_WALLHIT_VOLUME 512
void CBasePlayer::PlayMeleeSmackSound(CBaseEntity* pSmackedEntity, const int weaponId, const Vector& pos, const Vector& velocity)
{
	TraceResult fakeTrace = { 0 };
	fakeTrace.pHit = pSmackedEntity->edict();
	fakeTrace.vecEndPos = pos;
	fakeTrace.flFraction = 0.5f;

	// Meat sounds
	if (weaponId == WEAPON_HORNETGUN)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(m_pActiveItem->edict(), CHAN_ITEM, "weapons/bullet_hit1.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			break;
		case 1:
			EMIT_SOUND_DYN(m_pActiveItem->edict(), CHAN_ITEM, "weapons/bullet_hit2.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			break;
		}
	}
	// Squeek and soft meat sounds
	else if (weaponId == WEAPON_SNARK)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(m_pActiveItem->edict(), CHAN_ITEM, "weapons/bullet_hit1.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			break;
		case 1:
			EMIT_SOUND_DYN(m_pActiveItem->edict(), CHAN_ITEM, "weapons/bullet_hit2.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			break;
		}
		EMIT_SOUND_DYN(m_pActiveItem->edict(), CHAN_VOICE, (RANDOM_FLOAT(0, 1) <= 0.5) ? "squeek/sqk_hunt2.wav" : "squeek/sqk_hunt3.wav", 1, ATTN_NORM, 0, 105);
	}
	// TODO: Hand slap sounds
	else if (weaponId == WEAPON_BAREHAND)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(this->edict(), CHAN_ITEM, "weapons/bullet_hit1.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			break;
		case 1:
			EMIT_SOUND_DYN(this->edict(), CHAN_ITEM, "weapons/bullet_hit2.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			break;
		}
	}
	// Any other weapon makes crowbar sounds
	else if (pSmackedEntity->Classify() == CLASS_NONE || pSmackedEntity->Classify() == CLASS_MACHINE)
	{
		this->m_iWeaponVolume = CROWBAR_WALLHIT_VOLUME;

		TEXTURETYPE_PlaySound(&fakeTrace, pos - velocity.Normalize() * 32.f, pos + velocity.Normalize() * 32.f, BULLET_PLAYER_CROWBAR);

		// also play crowbar strike
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(pSmackedEntity->edict(), CHAN_ITEM, "weapons/cbar_hit1.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			break;
		case 1:
			EMIT_SOUND_DYN(pSmackedEntity->edict(), CHAN_ITEM, "weapons/cbar_hit2.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			break;
		}

		DecalGunshot(&fakeTrace, BULLET_PLAYER_CROWBAR);
	}
	else
	{
		this->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;

		// play thwack or smack sound
		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND_DYN(pSmackedEntity->edict(), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 1:
			EMIT_SOUND_DYN(pSmackedEntity->edict(), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 2:
			EMIT_SOUND_DYN(pSmackedEntity->edict(), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM, 0, PITCH_NORM);
			break;
		}
	}
}

void CBasePlayer::PlayVRWeaponAnimation(int iAnim, int body)
{
	//pev->weaponanim = iAnim;
	m_vrControllers[VRControllerID::WEAPON].PlayWeaponAnimation(iAnim, body);
}

void CBasePlayer::PlayVRWeaponMuzzleflash()
{
	m_vrControllers[VRControllerID::WEAPON].PlayWeaponMuzzleflash();
}

void CBasePlayer::UpdateFlashlight()
{
	// always call GetFlashlightPose, as it sets/unsets hand model flashlight body on the appropriate controller(s)
	Vector position;
	Vector dir;
	GetFlashlightPose(position, dir);

	if (FlashlightIsOn())
	{
		if (hFlashlightMonster)
		{
			ClearBits(hFlashlightMonster->pev->effects, EF_DIMLIGHT);
			hFlashlightMonster = nullptr;
		}
		if (hFlashLight)
		{
			TraceResult tr;
			UTIL_TraceLine(position, dir * 8192., dont_ignore_monsters, edict(), &tr);
			CBaseMonster* pFlashlightMonster = CBaseEntity::SafeInstance<CBaseMonster>(tr.pHit);
			if (pFlashlightMonster != nullptr)
			{
				SetBits(pFlashlightMonster->pev->effects, EF_DIMLIGHT);
				hFlashlightMonster = pFlashlightMonster;
				hFlashLight->pev->effects = EF_NODRAW;
			}
			else
			{
				UTIL_SetOrigin(hFlashLight->pev, tr.vecEndPos - dir);
				hFlashLight->pev->effects = EF_DIMLIGHT;
			}
		}
	}
}

void CBasePlayer::StartVRTele()
{
	Vector telePos, dummy;
	GetTeleporterPose(telePos, dummy);
	m_vrControllerTeleporter.StartTele(this, telePos);
}

void CBasePlayer::StopVRTele()
{
	m_vrControllerTeleporter.StopTele(this);
}

void CBasePlayer::UpdateVRTele()
{
	Vector telePos, teleDir;
	GetTeleporterPose(telePos, teleDir);
	m_vrControllerTeleporter.UpdateTele(this, telePos, teleDir);
}


void CBasePlayer::SetCurrentUpwardsTriggerPush(CBaseEntity* pEntity)
{
	m_hCurrentUpwardsTriggerPush = pEntity;
}

CBaseEntity* CBasePlayer::GetCurrentUpwardsTriggerPush()
{
	if (m_hCurrentUpwardsTriggerPush)
	{
		// Make sure it's still valid
		if (
			!UTIL_BBoxIntersectsBBox(m_hCurrentUpwardsTriggerPush->pev->absmin, m_hCurrentUpwardsTriggerPush->pev->absmax, pev->absmin, pev->absmax) || ((m_hCurrentUpwardsTriggerPush->pev->speed * m_hCurrentUpwardsTriggerPush->pev->movedir.z) <= (g_psv_gravity->value * pev->gravity)))
			m_hCurrentUpwardsTriggerPush = nullptr;
	}
	return m_hCurrentUpwardsTriggerPush;
}

void CBasePlayer::SetFlashlightPose(const Vector& offset, const Vector& angles)
{
	m_vrFlashlightOffset = offset;
	m_vrFlashlightAngles = angles;
	m_vrHasFlashlightPose = true;
}

void CBasePlayer::ClearFlashlightPose()
{
	m_vrHasFlashlightPose = false;
}

constexpr const int VR_FLASHLIGHT_ATTACHMENT_HAND = 0;
constexpr const int VR_FLASHLIGHT_ATTACHMENT_WEAPON = 1;
constexpr const int VR_FLASHLIGHT_ATTACHMENT_HEAD = 2;
constexpr const int VR_FLASHLIGHT_ATTACHMENT_POSE = 3;

// TODO: Merge with GetTeleporterPose, super redundant code
void CBasePlayer::GetFlashlightPose(Vector& position, Vector& dir)
{
	int attachment = int(CVAR_GET_FLOAT("vr_flashlight_attachment"));

	for (auto& controller : m_vrControllers)
	{
		controller.second.SetHasFlashlight(false);
	}

	if (attachment == VR_FLASHLIGHT_ATTACHMENT_POSE)
	{
		if (m_vrHasFlashlightPose)
		{
			position = GetClientOrigin() + m_vrFlashlightOffset;
			UTIL_MakeAimVectorsPrivate(m_vrFlashlightAngles, dir, nullptr, nullptr);
			return;
		}
		else
		{
			// fallback to hand
			attachment = VR_FLASHLIGHT_ATTACHMENT_HAND;
		}
	}

	if (attachment == VR_FLASHLIGHT_ATTACHMENT_HAND)
	{
		if (m_vrControllers[VRControllerID::HAND].IsValid())
		{
			Vector pos1;
			Vector pos2;
			bool result1 = m_vrControllers[VRControllerID::HAND].GetAttachment(VR_MUZZLE_ATTACHMENT, pos1);
			bool result2 = m_vrControllers[VRControllerID::HAND].GetAttachment(VR_MUZZLE_ATTACHMENT + 1, pos2);
			if (result1 && result2 && pos2 != pos1)
			{
				position = pos1;
				dir = (pos2 - pos1).Normalize();
			}
			else
			{
				position = m_vrControllers[VRControllerID::HAND].GetPosition();
				UTIL_MakeAimVectorsPrivate(m_vrControllers[VRControllerID::HAND].GetAngles(), dir, nullptr, nullptr);
			}
			m_vrControllers[VRControllerID::HAND].SetHasFlashlight(true);
			return;
		}
		else
		{
			// fallback to weapon
			attachment = VR_FLASHLIGHT_ATTACHMENT_WEAPON;
		}
	}

	if (attachment == VR_FLASHLIGHT_ATTACHMENT_WEAPON)
	{
		if (m_vrControllers[VRControllerID::WEAPON].IsValid())
		{
			Vector pos1;
			Vector pos2;
			bool result1 = m_vrControllers[VRControllerID::WEAPON].GetAttachment(VR_MUZZLE_ATTACHMENT, pos1);
			bool result2 = m_vrControllers[VRControllerID::WEAPON].GetAttachment(VR_MUZZLE_ATTACHMENT + 1, pos2);
			if (result1 && result2 && pos2 != pos1)
			{
				position = pos1;
				dir = (pos2 - pos1).Normalize();
			}
			else
			{
				position = m_vrControllers[VRControllerID::WEAPON].GetPosition();
				UTIL_MakeAimVectorsPrivate(m_vrControllers[VRControllerID::WEAPON].GetAngles(), dir, nullptr, nullptr);
			}
			m_vrControllers[VRControllerID::WEAPON].SetHasFlashlight(true);
			return;
		}
		else
		{
			// fallback to pose
			attachment = VR_FLASHLIGHT_ATTACHMENT_WEAPON;
		}
	}

	// either attachment is pose or we fallback to pose because no other attachment was available
	position = EyePosition();
	UTIL_MakeAimVectorsPrivate(pev->v_angle + pev->punchangle, dir, nullptr, nullptr);
}

void CBasePlayer::SetTeleporterPose(const Vector& offset, const Vector& angles)
{
	m_vrTeleporterOffset = offset;
	m_vrTeleporterAngles = angles;
	m_vrHasTeleporterPose = true;
}

void CBasePlayer::ClearTeleporterPose()
{
	m_vrHasTeleporterPose = false;
}

constexpr const int VR_TELEPORT_ATTACHMENT_HAND = 0;
constexpr const int VR_TELEPORT_ATTACHMENT_WEAPON = 1;
constexpr const int VR_TELEPORT_ATTACHMENT_HEAD = 2;
constexpr const int VR_TELEPORT_ATTACHMENT_POSE = 3;

// TODO: Merge with GetFlashlightPose, super redundant code
void CBasePlayer::GetTeleporterPose(Vector& position, Vector& dir)
{
	int attachment = int(CVAR_GET_FLOAT("vr_teleport_attachment"));

	if (attachment == VR_FLASHLIGHT_ATTACHMENT_POSE)
	{
		if (m_vrHasTeleporterPose)
		{
			position = GetClientOrigin() + m_vrTeleporterOffset;
			UTIL_MakeAimVectorsPrivate(m_vrTeleporterAngles, dir, nullptr, nullptr);
			return;
		}
		else
		{
			// fallback to hand
			attachment = VR_FLASHLIGHT_ATTACHMENT_HAND;
		}
	}

	if (attachment == VR_FLASHLIGHT_ATTACHMENT_HAND)
	{
		if (m_vrControllers[VRControllerID::HAND].IsValid())
		{
			Vector pos1;
			Vector pos2;
			bool result1 = m_vrControllers[VRControllerID::HAND].GetAttachment(VR_MUZZLE_ATTACHMENT, pos1);
			bool result2 = m_vrControllers[VRControllerID::HAND].GetAttachment(VR_MUZZLE_ATTACHMENT + 1, pos2);
			if (result1 && result2 && pos2 != pos1)
			{
				position = pos1;
				dir = (pos2 - pos1).Normalize();
			}
			else
			{
				position = m_vrControllers[VRControllerID::HAND].GetPosition();
				UTIL_MakeAimVectorsPrivate(m_vrControllers[VRControllerID::HAND].GetAngles(), dir, nullptr, nullptr);
			}
			return;
		}
		else
		{
			// fallback to weapon
			attachment = VR_FLASHLIGHT_ATTACHMENT_WEAPON;
		}
	}

	if (attachment == VR_FLASHLIGHT_ATTACHMENT_WEAPON)
	{
		if (m_vrControllers[VRControllerID::WEAPON].IsValid())
		{
			Vector pos1;
			Vector pos2;
			bool result1 = m_vrControllers[VRControllerID::WEAPON].GetAttachment(VR_MUZZLE_ATTACHMENT, pos1);
			bool result2 = m_vrControllers[VRControllerID::WEAPON].GetAttachment(VR_MUZZLE_ATTACHMENT + 1, pos2);
			if (result1 && result2 && pos2 != pos1)
			{
				position = pos1;
				dir = (pos2 - pos1).Normalize();
			}
			else
			{
				position = m_vrControllers[VRControllerID::WEAPON].GetPosition();
				UTIL_MakeAimVectorsPrivate(m_vrControllers[VRControllerID::WEAPON].GetAngles(), dir, nullptr, nullptr);
			}
			return;
		}
		else
		{
			// fallback to pose
			attachment = VR_FLASHLIGHT_ATTACHMENT_WEAPON;
		}
	}

	// either attachment is pose or we fallback to pose because no other attachment was available
	position = EyePosition();
	UTIL_MakeAimVectorsPrivate(pev->v_angle + pev->punchangle, dir, nullptr, nullptr);
}


void CBasePlayer::RestartCurrentMap()
{
	ALERT(at_notice, "RestartCurrentMap not implemented yet, sorry!\n");
}


void CBasePlayer::SetAnalogFire(float analogfire)
{
	if (fabs(analogfire) < EPSILON)
	{
		vr_analogFire = 0.f;
		m_hAnalogFirePlayer = nullptr;
	}
	else
	{
		vr_analogFire = analogfire;
		if (vr_analogFire > 1.f)
			vr_analogFire = 1.f;
		if (vr_analogFire < -1.f)
			vr_analogFire = -1.f;
		m_hAnalogFirePlayer = this;
	}
}

float CBasePlayer::GetAnalogFire()
{
	return vr_analogFire;
}

float CalculateWeaponTimeOffset(float offset)
{
	if (m_hAnalogFirePlayer)
	{
		float analogfire = fabs(m_hAnalogFirePlayer->GetAnalogFire());
		if (analogfire > EPSILON&& analogfire < 1.f)
		{
			return offset / analogfire;
		}
	}
	return offset;
}

float CalculateWeaponTimeOffsetReverse(float offset)
{
	if (m_hAnalogFirePlayer)
	{
		float analogfire = fabs(m_hAnalogFirePlayer->GetAnalogFire());
		if (analogfire > EPSILON&& analogfire < 1.f)
		{
			return offset * analogfire;
		}
	}
	return offset;
}

// In VR it's rather difficult to longjump (run + crouch + jump + correct timing),
// so players can map the long jump on a simple VR input action.
// - Max Vollmer - 2019-04-13
#define BUNNYJUMP_MAX_SPEED_FACTOR 1.7f
#define PLAYER_LONGJUMP_SPEED      350
void CBasePlayer::DoLongJump()
{
	if (pev->waterlevel)
		return;

	if (pev->button & IN_JUMP)
		return;

	if (pev->oldbuttons & IN_JUMP)
		return;

	if (pmove->waterjumptime)
		return;

	if (!(pev->flags & FL_ONGROUND))
		return;

	float maxscaledspeed = BUNNYJUMP_MAX_SPEED_FACTOR * pmove->maxspeed;
	if (maxscaledspeed <= 0.f)
		return;

	float speed = pev->velocity.Length();
	if (speed > maxscaledspeed)
	{
		pev->velocity = pev->velocity * (maxscaledspeed * 0.65f / speed);
	}

	extern void PM_PlayStepSound(int step, float fvol);
	extern int PM_MapTextureTypeStepType(char chTextureType);
	PM_PlayStepSound(PM_MapTextureTypeStepType(pmove->chtexturetype), 1.0f);

	if (m_fLongJump)
	{
		pev->punchangle.x = -5.f;
		pev->velocity = pmove->forward * PLAYER_LONGJUMP_SPEED * 1.6f;
		pev->velocity.z = sqrt(2.f * 800.f * 56.f);
		SetAnimation(PLAYER_SUPERJUMP);
	}
	else
	{
		pev->velocity.z = sqrt(2.f * 800.f * 56.f);
		SetAnimation(PLAYER_JUMP);
	}

	pev->velocity.z -= (pev->gravity * g_psv_gravity->value * gpGlobals->frametime * 0.5f);

	if (pev->velocity.x > pmove->movevars->maxvelocity)
		pev->velocity.x = pmove->movevars->maxvelocity;
	if (pev->velocity.y > pmove->movevars->maxvelocity)
		pev->velocity.y = pmove->movevars->maxvelocity;
	if (pev->velocity.z > pmove->movevars->maxvelocity)
		pev->velocity.z = pmove->movevars->maxvelocity;
	if (pev->velocity.x < -pmove->movevars->maxvelocity)
		pev->velocity.x = -pmove->movevars->maxvelocity;
	if (pev->velocity.y < -pmove->movevars->maxvelocity)
		pev->velocity.y = -pmove->movevars->maxvelocity;
	if (pev->velocity.z < -pmove->movevars->maxvelocity)
		pev->velocity.z = -pmove->movevars->maxvelocity;
}

void CBasePlayer::HolsterWeapon()
{
	SelectItem("weapon_barehand");
}

bool CBasePlayer::HasSuit()
{
	return FBitSet(pev->weapons, (1 << WEAPON_SUIT));
}

void CBasePlayer::SetLadderGrabbingController(VRControllerID controller, CBaseEntity* pLadder)
{
	m_ladderGrabbingControllers.push_back(VRLadderGrabbingController{ controller, pLadder });

	MESSAGE_BEGIN(MSG_ONE, gmsgVRGrabbedLadder, nullptr, pev);
	WRITE_SHORT(ENTINDEX(pLadder->edict()));
	MESSAGE_END();
}

void CBasePlayer::ClearLadderGrabbingControllers()
{
	if (m_ladderGrabbingControllers.empty())
		return;

	m_ladderGrabbingControllers.clear();
	MESSAGE_BEGIN(MSG_ONE, gmsgVRGrabbedLadder, nullptr, pev);
	WRITE_SHORT(0);
	MESSAGE_END();
}

void CBasePlayer::ClearLadderGrabbingController(VRControllerID controller)
{
	if (m_ladderGrabbingControllers.empty())
		return;

	m_ladderGrabbingControllers.erase(
		std::remove_if(
			m_ladderGrabbingControllers.begin(),
			m_ladderGrabbingControllers.end(),
			[controller](const VRLadderGrabbingController& v) { return v.controller == controller; }),
		m_ladderGrabbingControllers.end());

	if (m_ladderGrabbingControllers.empty())
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgVRGrabbedLadder, nullptr, pev);
		WRITE_SHORT(0);
		MESSAGE_END();
	}
}

bool CBasePlayer::IsLadderGrabbingController(VRControllerID controller, CBaseEntity* pLadder)
{
	if (m_ladderGrabbingControllers.empty())
		return false;

	return m_ladderGrabbingControllers.back().controller == controller && m_ladderGrabbingControllers.back().ladder == pLadder;
}

int CBasePlayer::GetGrabbedLadderEntIndex()
{
	if (m_ladderGrabbingControllers.empty() || CVAR_GET_FLOAT("vr_ladder_immersive_movement_enabled") == 0.f)
		return 0;

	return ENTINDEX(m_ladderGrabbingControllers.back().ladder.Get());
}


void CBasePlayer::StartPullingLedge(const Vector& ledgeTargetPosition, float speed)
{
	m_vrLedgeTargetPosition = ledgeTargetPosition;
	m_vrLedgePullSpeed = speed;
	m_vrLedgePullStartTime = gpGlobals->time;
	m_vrLedgePullStartPosition = pev->origin;
	m_vrIsPullingOnLedge = true;

	MESSAGE_BEGIN(MSG_ONE, gmsgVRPullingLedge, nullptr, pev);
	WRITE_BYTE(1);
	MESSAGE_END();
}

void CBasePlayer::StopPullingLedge()
{
	if (!m_vrIsPullingOnLedge)
		return;

	m_vrIsPullingOnLedge = false;

	MESSAGE_BEGIN(MSG_ONE, gmsgVRPullingLedge, nullptr, pev);
	WRITE_BYTE(0);
	MESSAGE_END();
}

constexpr const float VR_MAX_TALK_DISTANCE = 512.f;

void CBasePlayer::HandleSpeechCommand(VRSpeechCommand command)
{
	ALERT(at_console, "HandleSpeechCommand: %i\n", int(command));

	// Create sound
	CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, 1000, 0.5f);

	Vector lookDir;
	UTIL_MakeAimVectorsPrivate(pev->angles, lookDir, nullptr, nullptr);

	CTalkMonster* closestReactingNPC = nullptr;
	std::vector<CTalkMonster*> reactingNPCs;

	// Find all NPCs in PVS
	edict_t* pent = UTIL_EntitiesInPVS(edict());
	while (pent && !FNullEnt(pent))
	{
		CTalkMonster* pMonster = CBaseEntity::SafeInstance<CTalkMonster>(pent);
		if (pMonster)
		{
			if (FClassnameIs(pent, "monster_barney") || FClassnameIs(pent, "monster_scientist"))
			{
				// If command is MUMBLE or HELLO, only add monsters that can speak
				// If command is FOLLOW or WAIT, always add to list
				float biggestdot = 0.f;
				float shortestdist = VR_MAX_TALK_DISTANCE;
				if (command == VRSpeechCommand::FOLLOW || command == VRSpeechCommand::WAIT || pMonster->FOkToSpeak())
				{
					if (FVisible(pMonster) && pMonster->FVisible(this))
					{
						Vector dir = pMonster->EyePosition() - EyePosition();
						float distance = dir.Length();
						if (distance < VR_MAX_TALK_DISTANCE)
						{
							pMonster->MakeIdealYaw(pev->origin);

							float dot = DotProduct(dir.Normalize(), lookDir);
							if (dot > 0.8f)
							{
								reactingNPCs.push_back(pMonster);
								if (dot > biggestdot)
								{
									closestReactingNPC = pMonster;
									biggestdot = dot;
								}
							}
						}
					}
				}
			}
		}
		pent = pent->v.chain;
	}

	if (reactingNPCs.empty() || closestReactingNPC == nullptr)
		return;

	if (command == VRSpeechCommand::MUMBLE || command == VRSpeechCommand::HELLO)
	{
		// List contains closest NPC that should respond
		closestReactingNPC->m_hTalkTarget = this;
		closestReactingNPC->IdleHeadTurn(pev->origin);
		if (command == VRSpeechCommand::HELLO)
		{
			PlaySentence((FClassnameIs(closestReactingNPC->pev, "monster_scientist")) ? "SC_SHELLO" : "BA_SHELLO", RANDOM_FLOAT(3, 3.5), VOL_NORM, ATTN_IDLE);
			SetBits(closestReactingNPC->m_bitsSaid, bit_saidHelloPlayer);
		}
		else
		{
			closestReactingNPC->IdleRespond();
		}
	}
	else if (command == VRSpeechCommand::FOLLOW && FBitSet(closestReactingNPC->pev->spawnflags, SF_MONSTER_PREDISASTER))
	{
		// In predisaster maps only the closest NPC reacts to the follow command ("please, leave me alone until after the experiment")
		closestReactingNPC->FollowerUse(this, this, USE_ON, 1.f);
	}
	else
	{
		// All NPCS react
		for (CTalkMonster* pMonster : reactingNPCs)
		{
			if (command == VRSpeechCommand::FOLLOW && !pMonster->IsFollowing())
			{
				pMonster->FollowerUse(this, this, USE_ON, 1.f);
			}
			else if (command == VRSpeechCommand::WAIT && pMonster->IsFollowing())
			{
				pMonster->StopFollowing(TRUE);
			}
		}
	}
}

constexpr const int SF_TANK_CANCONTROL = 0x0020;

bool IsValidTankDraggingController(const VRController& controller, entvars_t* pevTank)
{
	return controller.IsValid() &&
		controller.IsDragging() &&
		((controller.GetPosition() - pevTank->origin).Length() < CVAR_GET_FLOAT("vr_tankcontrols_max_distance"));
}

bool CBasePlayer::IsTankVRControlled(entvars_t* pevTank)
{
	if (CVAR_GET_FLOAT("vr_tankcontrols") == 0.f)
		return false;

	if (!pevTank)
		return false;

	if (FBitSet(pevTank->spawnflags, SF_TANK_CANCONTROL) && UTIL_StartsWith(STRING(pevTank->classname), "func_tank"))
	{
		if (CVAR_GET_FLOAT("vr_tankcontrols") == 1.f)
		{
			return IsValidTankDraggingController(m_vrControllers[VRControllerID::HAND], pevTank) ||
				IsValidTankDraggingController(m_vrControllers[VRControllerID::WEAPON], pevTank);
		}
		else
		{
			return IsValidTankDraggingController(m_vrControllers[VRControllerID::HAND], pevTank) &&
				IsValidTankDraggingController(m_vrControllers[VRControllerID::WEAPON], pevTank);
		}
	}

	return false;
}

void CBasePlayer::VRDoTheBackPackThing()
{
	if (CVAR_GET_FLOAT("vr_virtual_backpack_enabled") == 0.f)
		return;

	auto& controller = m_vrControllers[VRControllerID::WEAPON];

	if (!controller.IsValid())
		return;

	// different logic for grabbing or touching
	bool needsgrab = CVAR_GET_FLOAT("vr_virtual_backpack_needsgrab") == 0.f;

	if (needsgrab)
	{
		bool isgrabbing = (controller.IsDragging() && !controller.HasDraggedEntity());
		if (m_vrHasGrabbedBackPack && isgrabbing)
		{
			if (IsInNonBackPackArea(controller.GetPosition()))
				m_vrHasGrabbedBackPack = false;
			return;
		}

		if (!IsInBackPackArea(controller.GetPosition()))
			return;

		if (isgrabbing && !m_vrHasGrabbedBackPack)
		{
			VRSwitchBackPackItem();
			m_vrHasGrabbedBackPack = true;
		}
		else if (m_vrHasGrabbedBackPack && !isgrabbing)
		{
			m_vrHasGrabbedBackPack = false;
		}
	}
	else
	{
		if (IsInBackPackArea(controller.GetPosition()))
		{
			// already handled!
			if (m_vrHasGrabbedBackPack)
				return;

			VRSwitchBackPackItem();
			m_vrHasGrabbedBackPack = true;
		}
		// there is some space between backpack and non-backpack area, that's why we have two checks
		else if (IsInNonBackPackArea(controller.GetPosition()))
		{
			m_vrHasGrabbedBackPack = false;
		}
	}
}

void CBasePlayer::VRSwitchBackPackItem()
{
	// nothing in backpack and nothing in hand
	if (!m_pActiveItem && !m_vrhBackPackItem)
		return;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

	auto previousItem = m_pActiveItem;
	if (m_vrhBackPackItem && m_vrhBackPackItem != previousItem)
	{
		SelectItem(STRING(m_vrhBackPackItem->pev->classname));
	}
	else
	{
		SelectItem("weapon_barehand");
	}
	m_vrhBackPackItem = previousItem;
}

bool CBasePlayer::IsInBackPackArea(const Vector& pos)
{
	Vector headsetPos = pev->origin + Vector{ vr_lastHMDOffset.x, vr_lastHMDOffset.y, pev->view_ofs.z };

	// check between head and torso
	if (pos.z > headsetPos.z)
		return false;

	if (pos.z < pev->origin.z)
		return false;

	// check far enough from head
	Vector delta = headsetPos - pos;
	delta.z = 0.f;
	float distance = delta.Length();
	if (distance < 8.f)
		return false;

	// check behind player
	delta = delta / distance;

	Vector dir = vr_hmdForward;
	dir.z = 0.f;
	dir.InlineNormalize();

	float dot = DotProduct2D(dir, delta);
	return dot > 0.7f;
}

bool CBasePlayer::IsInNonBackPackArea(const Vector& pos)
{
	Vector headsetPos = pev->origin + Vector{ vr_lastHMDOffset.x, vr_lastHMDOffset.y, pev->view_ofs.z };

	// check in front of player
	Vector delta = pos - headsetPos;
	delta.z = 0.f;
	delta.InlineNormalize();

	Vector dir = vr_hmdForward;
	dir.z = 0.f;
	dir.InlineNormalize();

	float dot = DotProduct2D(dir, delta);
	return dot >= 0.f;
}

CBaseEntity* CBasePlayer::VRFindTank(const char* func_tank_classname)
{
	edict_t* pent = FIND_ENTITY_BY_CLASSNAME(nullptr, func_tank_classname);
	while (!FNullEnt(pent))
	{
		if (IsTankVRControlled(&pent->v) &&
			(UTIL_IsFacing(pev->origin, pev->angles.ToViewAngles(), pent->v.origin)
				|| UTIL_IsFacing(pev->origin, pev->angles.ToViewAngles(), (pent->v.absmax + pent->v.absmin) * 0.5)))
		{
			return CBaseEntity::SafeInstance<CBaseEntity>(pent);
		}
		pent = FIND_ENTITY_BY_CLASSNAME(pent, "func_tank");
	}
	return nullptr;
}

void CBasePlayer::VRUseOrUnuseTank()
{
	if (m_vrIsUsingTankWithVRControllers)
	{
		bool cancelUse = false;
		if (m_pTank && IsTankVRControlled(m_pTank->pev))
		{
			std::vector<Vector> allControllerPositions;
			for (auto& [id, controller] : m_vrControllers)
			{
				if (IsValidTankDraggingController(controller, m_pTank->pev))
				{
					allControllerPositions.push_back(controller.GetPosition());
				}
			}

			if (allControllerPositions.empty())
			{
				cancelUse = true;
			}
			else
			{
				Vector position;
				for (auto& controllerPosition : allControllerPositions)
				{
					position = position + controllerPosition;
				}
				position = position / float(allControllerPositions.size());

				Vector direction = (m_pTank->pev->origin - position).Normalize();
				m_vrTankVRControllerAngles = UTIL_VecToAngles(direction);

				// make sure no weapon is selected (noop if no weapon is selected)
				HolsterWeapon();

				// fire the gun
				m_pTank->Use(this, this, USE_SET, 2);
			}
		}
		else
		{
			cancelUse = true;
		}

		if (cancelUse)
		{
			if (m_pTank)
				m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = nullptr;
			if (m_pActiveItem == nullptr)
			{
				SelectLastItem();
			}
			m_vrIsUsingTankWithVRControllers = false;
			m_vrTankVRControllerAngles = Vector{};
		}
	}

	if (!m_vrIsUsingTankWithVRControllers)
	{
		CBaseEntity* pTank = VRFindTank("func_tank");
		if (!pTank) pTank = VRFindTank("func_tanklaser");
		if (!pTank) pTank = VRFindTank("func_tankrocket");
		if (!pTank) pTank = VRFindTank("func_tankmortar");

		if (pTank)
		{
			if (m_pTank && m_pTank != pTank)
			{
				m_pTank->Use(this, this, USE_OFF, 0);
			}
			m_pTank = pTank;
			m_pTank->Use(this, this, USE_ON, 1337);  // 1337 is hacky way to tell CFuncTank that this is a VR initiated control
			m_vrIsUsingTankWithVRControllers = true;
			m_vrTankVRControllerAngles = m_pTank->pev->angles;
			HolsterWeapon();
		}
	}
}

Vector CBasePlayer::GetTankControlAngles()
{
	if (m_vrIsUsingTankWithVRControllers)
	{
		return m_vrTankVRControllerAngles;
	}
	else
	{
		Vector angles = pev->v_angle;
		angles.x = -angles.x;
		return angles;
	}
}

bool CBasePlayer::VRCanAttack()
{
	TraceResult tr{ 0 };
	UTIL_TraceLine(pev->origin, GetGunPosition(), ignore_monsters, nullptr, &tr);
	return tr.flFraction == 1.f;
}




void EXPORT CBaseEntity::DragThink(void)
{
	m_pfnThink = nullptr;

	EHANDLE<CBasePlayer> hPlayer = m_vrDragger;
	if (hPlayer && m_vrDragController != VRControllerID::INVALID)
	{
		auto& controller = hPlayer->GetController(m_vrDragController);
		if (controller.IsValid())
		{
			Vector origin = controller.GetGunPosition();
			Vector velocity = controller.GetVelocity();
			Vector angles = controller.GetAngles();

			HandleDragUpdate(origin, velocity, UTIL_AnglesMod(angles));
		}
	}
}

void CBasePlayer::VRHandleRetinaScanners()
{
	for (auto [hRetinaScanner, hRetinaScannerButton] : g_vrRetinaScanners)
	{
		Vector retinaScannerPosition = (hRetinaScanner->pev->absmax + hRetinaScanner->pev->absmin) * 0.5;
		bool isLookingAtRetinaScanner =
			UTIL_IsFacing(pev->origin, pev->angles.ToViewAngles(), retinaScannerPosition)
			&& ((EyePosition() - retinaScannerPosition).Length() < 32.f)
			&& (EyePosition().z >= hRetinaScanner->pev->absmin.z)
			&& (EyePosition().z <= hRetinaScanner->pev->absmax.z);

		if (isLookingAtRetinaScanner)
		{
			if (m_vrHRetinaScanner == hRetinaScanner)
			{
				if ((gpGlobals->time - m_vrRetinaScannerLookTime) >= VR_RETINASCANNER_ACTIVATE_LOOK_TIME)
				{
					if (!m_vrRetinaScannerUsed)
					{
						g_vrRetinaScanners[hRetinaScanner]->Use(this, this, USE_SET, 1);
						m_vrRetinaScannerUsed = true;
					}
				}
			}
			else
			{
				m_vrHRetinaScanner = hRetinaScanner;
				m_vrRetinaScannerLookTime = gpGlobals->time;
				m_vrRetinaScannerUsed = false;
			}
		}
		else
		{
			if (m_vrHRetinaScanner == hRetinaScanner)
			{
				m_vrHRetinaScanner = nullptr;
				m_vrRetinaScannerLookTime = 0.f;
				m_vrRetinaScannerUsed = false;
			}
		}
	}
}

void CBasePlayer::UpdateVRLaserSpot()
{
	int weaponId = WEAPON_BAREHAND;
	ItemInfo itemInfo = {};
	if (m_pActiveItem != nullptr && m_pActiveItem->GetItemInfo(&itemInfo))
	{
		weaponId = itemInfo.iId;
	}

	if (IsAlive() && IsWeaponWithVRLaserSpot(weaponId) && CVAR_GET_FLOAT("vr_enable_aim_laser") != 0.f)
	{
		if (!m_hLaserSpot)
		{
			CLaserSpot* pLaserSpot = CLaserSpot::CreateSpot();
			pLaserSpot->Revive();
			pLaserSpot->pev->scale = 0.1f;
			m_hLaserSpot = pLaserSpot;
		}

		TraceResult tr;
		UTIL_TraceLine(GetWeaponPosition(), GetWeaponPosition() + (GetAutoaimVector() * 8192.f), dont_ignore_monsters, edict(), &tr);
		UTIL_SetOrigin(m_hLaserSpot->pev, tr.vecEndPos);
	}
	else
	{
		if (m_hLaserSpot)
		{
			UTIL_Remove(m_hLaserSpot);
			m_hLaserSpot = nullptr;
		}
	}
}
