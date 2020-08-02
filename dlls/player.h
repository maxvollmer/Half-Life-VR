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
#ifndef PLAYER_H
#define PLAYER_H


#include "pm_materials.h"
#include <unordered_set>
#include <unordered_map>

#include "VRCommons.h"
//#include "VRPhysicsHelper.h"
#include "VRController.h"
#include "VRControllerInteractionManager.h"
#include "VRControllerTeleporter.h"
#include "VRGroundEntityHandler.h"
#include "../vr_shared/VRShared.h"


#define PLAYER_FATAL_FALL_SPEED      1024                                                                 // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED   580                                                                  // approx 20 feet
#define DAMAGE_FOR_FALL_SPEED        (float)100 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED)  // damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED      200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350  // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER   (1 << 0)
#define PFLAG_ONSWING    (1 << 0)
#define PFLAG_ONTRAIN    (1 << 1)
#define PFLAG_ONBARNACLE (1 << 2)
//#define		PFLAG_DUCKING		( 1<<3 )		// In the process of ducking, but totally squatted yet
#define PFLAG_USING    (1 << 4)  // Using a continuous entity
#define PFLAG_OBSERVER (1 << 5)  // player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST 4  // max of 4 suit sentences queued up at any time

#define SUIT_GROUP    TRUE
#define SUIT_SENTENCE FALSE

#define SUIT_REPEAT_OK     0
#define SUIT_NEXT_IN_30SEC 30
#define SUIT_NEXT_IN_1MIN  60
#define SUIT_NEXT_IN_5MIN  300
#define SUIT_NEXT_IN_10MIN 600
#define SUIT_NEXT_IN_30MIN 1800
#define SUIT_NEXT_IN_1HOUR 3600

#define CSUITNOREPEAT 32

#define SOUND_FLASHLIGHT_ON  "items/flashlight1.wav"
#define SOUND_FLASHLIGHT_OFF "items/flashlight1.wav"

#define TEAM_NAME_LENGTH 16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
} PLAYER_ANIM;

#define MAX_ID_RANGE     2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END,
};

class CSprite;
class CBeam;
class CFuncTank;
class CTalkMonster;

#include <map>

#define CHAT_INTERVAL 1.0f

class CBasePlayer : public CBaseMonster
{
public:
	int random_seed = 0;  // See that is shared between client & server for shared weapons code

	int m_iPlayerSound = 0;      // the index of the sound list slot reserved for this player
	int m_iTargetVolume = 0;     // ideal sound volume.
	int m_iWeaponVolume = 0;     // how loud the player's weapon is right now.
	int m_iExtraSoundTypes = 0;  // additional classification for this weapon's sound
	int m_iWeaponFlash = 0;      // brightness of the weapon flash
	float m_flStopExtraSoundTime = 0.f;

	float m_flFlashLightTime = 0.f;  // Time until next battery draw/Recharge
	int m_iFlashBattery = 0;       // Flashlight Battery Draw

	int m_afButtonLast = 0;
	int m_afButtonPressed = 0;
	int m_afButtonReleased = 0;

	EHANDLE<CBaseEntity> m_hSndLast;  // last sound entity to modify player room type
	float m_iSndRoomtype = 0;   // last roomtype set by sound entity
	float m_flSndRange = 0.f;      // dist from player to sound entity

	float m_flFallVelocity = 0.f;

	int m_rgItems[MAX_ITEMS];
	int m_fKnownItem = 0;  // True when a new item needs to be added
	int m_fNewAmmo = 0;    // True when a new item has been added

	unsigned int m_afPhysicsFlags = 0;  // physics flags - set when 'normal' physics should be revisited or overriden
	float m_fNextSuicideTime = 0.f;       // the time after which the player can next use the suicide command


	// these are time-sensitive things that we keep track of
	float m_flTimeStepSound = 0.f;   // when the last stepping sound was made
	float m_flTimeWeaponIdle = 0.f;  // when to play another weapon idle animation.
	float m_flSwimTime = 0.f;        // how long player has been underwater
	float m_flDuckTime = 0.f;        // how long we've been ducking
	float m_flWallJumpTime = 0.f;    // how long until next walljump

	float m_flSuitUpdate = 0.f;                         // when to play next suit update
	int m_rgSuitPlayList[CSUITPLAYLIST];          // next sentencenum to play for suit update
	int m_iSuitPlayNext = 0;                          // next sentence slot for queue storage;
	int m_rgiSuitNoRepeat[CSUITNOREPEAT];         // suit sentence no repeat list
	float m_rgflSuitNoRepeatTime[CSUITNOREPEAT];  // how long to wait before allowing repeat
	int m_lastDamageAmount = 0;                       // Last damage taken
	float m_tbdPrev = 0.f;                              // Time-based damage timer

	float m_flgeigerRange = 0.f;  // range to nearest radiation source
	float m_flgeigerDelay = 0.f;  // delay per update of range msg to client
	int m_igeigerRangePrev = 0;
	int m_iStepLeft = 0;                         // alternate left/right foot stepping sound
	char m_szTextureName[CBTEXTURENAMEMAX];  // current texture name we're standing on
	char m_chTextureType;                    // current texture type

	int m_idrowndmg = 0;       // track drowning damage taken
	int m_idrownrestored = 0;  // track drowning damage restored

	int m_bitsHUDDamage = 0;  // Damage bits for the current fame. These get sent to
						  // the hude via the DAMAGE message
	BOOL m_fInitHUD = FALSE;      // True when deferred HUD restart msg needs to be sent
	BOOL m_fGameHUDInitialized = FALSE;
	int m_iTrain = 0;    // Train control position
	EHANDLE<CBaseEntity> m_hLastTrain;
	BOOL m_fWeapon = FALSE;  // Set this to FALSE to force a reset of the current weapon HUD info

	EHANDLE<CBaseEntity> m_pTank;    // the tank which the player is currently controlling,  nullptr if no tank
	float m_fDeadTime = 0.f;  // the time at which the player died  (used in PlayerDeathThink())

	BOOL m_fNoPlayerSound = FALSE;  // a debugging feature. Player makes no sound if this is true.
	BOOL m_fLongJump = FALSE;       // does this player have the longjump module?

	float m_tSneaking = 0.f;
	int m_iUpdateTime = 0;     // stores the number of frame ticks before sending HUD update messages
	int m_iClientHealth = 0;   // the health currently known by the client.  If this changes, send a new
	int m_iClientBattery = 0;  // the Battery currently known by the client.  If this changes, send a new
	int m_iHideHUD = 0;        // the players hud weapon info is to be hidden
	int m_iClientHideHUD = 0;
	int m_iFOV = 0;        // field of view
	int m_iClientFOV = 0;  // client's known FOV
	// usable player items
	EHANDLE<CBasePlayerItem> m_rgpPlayerItems[MAX_ITEM_TYPES];
	EHANDLE<CBasePlayerItem> m_pActiveItem;
	EHANDLE<CBasePlayerItem> m_pClientActiveItem;  // client version of the active item
	EHANDLE<CBasePlayerItem> m_pLastItem;
	// shared ammo slots
	int m_rgAmmo[MAX_AMMO_SLOTS]{ 0 };
	int m_rgAmmoLast[MAX_AMMO_SLOTS]{ 0 };

	int m_iDeaths{ 0 };
	float m_iRespawnFrames{ 0.f };  // used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames = 0;  // Custom clan logo frames for this player
	float m_flNextDecalTime = 0.f;   // next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	virtual void Spawn(void);
	void Pain(void);

	//	virtual void Think( void );
	virtual void Jump(void);
	virtual void Duck(void);
	virtual void PreThink(void);
	virtual void PostThink(void);
	virtual Vector GetGunPosition(void);
	virtual Vector GetAimAngles(void);  // Extra method for VR controller weapons - Max Vollmer, 2019-03-30
	virtual int TakeHealth(float flHealth, int bitsDamageType);
	virtual void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	virtual int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(entvars_t* pevAttacker, int bitsDamageType, int iGib);
	virtual Vector BodyTarget(const Vector& posSrc) { return Center() + pev->view_ofs * RANDOM_FLOAT(0.5, 1.1); };  // position to shoot at
	virtual void StartSneaking(void) { m_tSneaking = gpGlobals->time - 1; }
	virtual void StopSneaking(void) { m_tSneaking = gpGlobals->time + 30; }
	virtual BOOL IsSneaking(void) { return m_tSneaking <= gpGlobals->time; }
	virtual BOOL IsAlive(void) { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	virtual BOOL ShouldFadeOnDeath(void) { return FALSE; }
	virtual BOOL IsPlayer(void) { return TRUE; }  // Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned

	virtual BOOL IsNetClient(void) { return TRUE; }  // Bots should return FALSE for this, they can't receive NET messages
													 // Spectators should return TRUE for this
	virtual const char* TeamID(void);

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);
	void RenewItems(void);
	void PackDeadPlayerItems(void);
	void RemoveAllItems(BOOL removeSuit);
	BOOL SwitchWeapon(CBasePlayerItem* pWeapon);

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData(void);

	static TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	virtual int ObjectCaps(void) { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void Precache(void);
	BOOL IsOnLadder(void);
	BOOL FlashlightIsOn(void);
	void FlashlightTurnOn(void);
	void FlashlightTurnOff(void);

	void UpdatePlayerSound(void);
	void DeathSound(void);

	int Classify(void);
	void SetAnimation(PLAYER_ANIM playerAnim);
	void SetWeaponAnimType(const char* szExtention);
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands(void);
	void CheatImpulseCommands(int iImpulse);

	void StartDeathCam(void);
	void StartObserver(Vector vecPosition, Vector vecViewAngle);

	void AddPoints(int score, BOOL bAllowNegativeScore);
	void AddPointsToTeam(int score, BOOL bAllowNegativeScore);
	BOOL AddPlayerItem(CBasePlayerItem* pItem);
	BOOL RemovePlayerItem(CBasePlayerItem* pItem);
	void DropPlayerItem(const char* pszItemName);
	BOOL HasPlayerItem(CBasePlayerItem* pCheckItem);
	BOOL HasNamedPlayerItem(const char* pszItemName);
	BOOL HasWeapons(void);  // do I have ANY weapons?
	//void SelectPrevItem( int iItem );
	//void SelectNextItem( int iItem );
	void SelectLastItem(void);
	void SelectItem(const char* pstr);
	void ItemPreFrame(void);
	void ItemPostFrame(void);
	void GiveNamedItem(const char* szName);
	void EnableControl(BOOL fControl);

	int GiveAmmo(int iAmount, const char* szName, int iMax, int* pIndex = nullptr) override;
	void SendAmmoUpdate(void);

	void WaterMove(void);
	void EXPORT PlayerDeathThink(void);
	void PlayerUse(void);

	void CheckSuitUpdate();
	void SetSuitUpdate(char* name, int fgroup, int iNoRepeat);
	void UpdateGeigerCounter(void);
	void CheckTimeBasedDamage(void);

	BOOL FBecomeProne(void);
	void BarnacleVictimBitten(entvars_t* pevBarnacle);
	void BarnacleVictimReleased(void);
	static int GetAmmoIndex(const char* psz);
	int AmmoInventory(int iAmmoIndex);
	int Illumination(void);

	void ResetAutoaim(void);
	Vector GetAutoaimVector(float flDelta = 0.f);
	Vector AutoaimDeflection(Vector& vecSrc, float flDist, float flDelta);

	void ForceClientDllUpdate(void);  // Forces all client .dll specific data to be resent to client.

	void DeathMessage(entvars_t* pevKiller);

	void SetCustomDecalFrames(int nFrames);
	int GetCustomDecalFrames(void);

	void CBasePlayer::TabulateAmmo(void);

	bool IsUsableTrackTrain(CBaseEntity* pTrain);

	float m_flStartCharge = 0.f;
	float m_flAmmoStartCharge = 0.f;
	float m_flPlayAftershock = 0.f;
	float m_flNextAmmoBurn = 0.f;  // while charging, when to absorb another unit of player's ammo?

	//Player ID
	void InitStatusBar(void);
	void UpdateStatusBar(void);
	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime = 0.f;
	float m_flStatusBarDisappearDelay = 0.f;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];

	float m_flNextChatTime = 0.f;


/*
*  Methods and members for VR stuff - Max Vollmer, 2017-08-18
*/

// private VR members:
private:
	// for immersive ladder climbing
	struct VRLadderGrabbingController
	{
		VRControllerID controller;
		EHANDLE<CBaseEntity> ladder;
	};

	Vector vr_lastHMDOffset;
	int vr_hmdLastUpdateClienttime = 0;
	float vr_hmdLastUpdateServertime = 0;

	Vector vr_hmdForward{ 1.f, 0.f, 0.f };

	Vector vr_ClientOriginOffset;  // Must be Vector instead of Vector2D for save/restore. z is not used.

	EHANDLE<CBaseEntity> hFlashLight;
	EHANDLE<CBaseEntity> hFlashlightMonster;
	bool fFlashlightIsOn = false;

	EHANDLE<CBaseEntity> m_vrHRetinaScanner;
	float m_vrRetinaScannerLookTime = 0;
	bool m_vrRetinaScannerUsed = false;

	// for save/restore
	// client sends these every frame
	// server sends these when loading/restoring
	float vr_prevYaw = 0.f;
	float vr_currentYaw = 0.f;
	bool vr_needsToSendRestoreYawMsgToClient = false;
	bool vr_hasSentRestoreYawMsgToClient = false;
	bool vr_hasSentSpawnYawToClient = false;

	std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_vrInUseButtons;
	std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_vrLeftMeleeEntities;
	std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_vrRightMeleeEntities;

	std::unordered_map<VRControllerID, VRController> m_vrControllers;

	VRControllerTeleporter m_vrControllerTeleporter;
	VRControllerInteractionManager m_vrControllerInteractionManager;
	std::unique_ptr<VRGroundEntityHandler> m_vrGroundEntityHandler;

	bool m_vrHasTeleporterPose{ false };
	Vector m_vrTeleporterOffset;
	Vector m_vrTeleporterAngles;

	bool m_vrHasFlashlightPose{ false };
	Vector m_vrFlashlightOffset;
	Vector m_vrFlashlightAngles;

	EHANDLE<CBaseEntity> m_hCurrentUpwardsTriggerPush;

	float vr_analogFire{ 0.f };

	// For VR suitable display of train controls
	Vector vr_trainControlPosition;
	float vr_trainControlYaw = 0.f;

	std::vector<VRLadderGrabbingController> m_ladderGrabbingControllers;

	bool m_vrIsUsingTankWithVRControllers{ false };
	Vector m_vrTankVRControllerAngles;

	// filled with current item if virtual backpack is used
	EHANDLE<CBasePlayerItem> m_vrhBackPackItem;

	// set to true when player grabs backpack area
	bool m_vrHasGrabbedBackPack = false;

	mutable EHANDLE<CLaserSpot> m_hLaserSpot;

// public VR members:
public:
	float vr_spawnYaw{ 0.f };
	bool vr_IsJustSpawned{ false };
	bool vr_IsJustRestored{ false };

	Vector m_vrLedgeTargetPosition;
	Vector m_vrLedgePullStartPosition;
	float m_vrLedgePullSpeed{ 0.f };
	float m_vrLedgePullStartTime{ 0.f };
	bool m_vrIsPullingOnLedge{ false };
	bool m_vrWasPullingOnLedge{ false };

	VRController& GetController(VRControllerID id) { return m_vrControllers[id]; }


// private VR methods:
private:

	bool CheckVRTRainButtonTouched(const Vector& buttonLeftPos, const Vector& buttonRightPos);

	void GetTeleporterPose(Vector& position, Vector& dir);
	void GetFlashlightPose(Vector& position, Vector& dir);

	void UpdateFlashlight();
	void UpdateVRTele();
	void UpdateVRLaserSpot();

	void ClearLadderGrabbingControllers();

	// Called by PostThink()
	void VRUseOrUnuseTank();
	bool IsTankVRControlled(entvars_t* pevTank);
	CBaseEntity* VRFindTank(const char* func_tank_classname);

	// virtual backpack, called by PostThink()
	void VRDoTheBackPackThing();
	void VRSwitchBackPackItem();
	bool IsInBackPackArea(const Vector& pos);
	bool IsInNonBackPackArea(const Vector& pos);

	VRGroundEntityHandler& GetGroundEntityHandler()
	{
		if (!m_vrGroundEntityHandler)
		{
			m_vrGroundEntityHandler = std::make_unique<VRGroundEntityHandler>(this);
		}
		return *m_vrGroundEntityHandler;
	}

	void VRHandleRetinaScanners();

// public VR methods:
public:
	void StartVRTele();
	void StopVRTele();

	const Vector GetWeaponPosition();
	const Vector GetWeaponAngles();
	const Vector GetWeaponViewAngles();
	const Vector GetWeaponVelocity();
	const Vector GetClientOrigin();   // Used by UpdateClientData to send player origin to client
	const Vector GetClientViewOfs();  // Used by UpdateClientData to send player view_ofs to client
	bool IsWeaponUnderWater();
	bool IsWeaponPositionValid();

	void UpdateVRHeadset(const int timestamp, const Vector2D& hmdOffset, const float offsetZ, const Vector& forward, const Vector2D& hmdYawOffsetDelta, float prevYaw, float currentYaw, bool hasReceivedRestoreYawMsg, bool hasReceivedSpawnYaw);
	void UpdateVRController(const VRControllerID vrControllerID, const int timestamp, const bool isValid, const bool isMirrored, const Vector& offset, const Vector& angles, const Vector& velocity, bool dragOn);

	void StoreVROffsetsForLevelchange();

	void PlayMeleeSmackSound(CBaseEntity* pSmackedEntity, const int weaponId, const Vector& pos, const Vector& velocity);

	void PlayVRWeaponAnimation(int iAnim, int body);
	void PlayVRWeaponMuzzleflash();

	void SetCurrentUpwardsTriggerPush(CBaseEntity* pEntity);
	CBaseEntity* GetCurrentUpwardsTriggerPush();

	void SetFlashlightPose(const Vector& offset, const Vector& angles);
	void ClearFlashlightPose();

	void SetTeleporterPose(const Vector& offset, const Vector& angles);
	void ClearTeleporterPose();

	void DoLongJump();
	void RestartCurrentMap();

	void HolsterWeapon();

	float GetAnalogFire();
	void SetAnalogFire(float analogfire);

	// Used by CChangeLevel::InTransitionVolume
	// Set by VRControllerTeleporter
	bool vr_didJustTeleportThroughChangeLevel{ false };

	bool HasSuit();

	// for immersive ladder climbing
	void SetLadderGrabbingController(VRControllerID controller, CBaseEntity* pLadder);
	void ClearLadderGrabbingController(VRControllerID controller);
	bool IsLadderGrabbingController(VRControllerID controller, CBaseEntity* pLadder);
	int GetGrabbedLadderEntIndex();

	void StartPullingLedge(const Vector& ledgeTargetPosition, float speed);
	void StopPullingLedge();

	void HandleSpeechCommand(VRSpeechCommand command);

	// For tanks (used in CFuncTank::TrackTarget())
	Vector GetTankControlAngles();

	// Checks if the weapon can be fired (prevents shooting when controller is pointed through walls)
	bool VRCanAttack();
};

#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

extern int gmsgHudText;
extern BOOL gInitHUD;

#endif  // PLAYER_H
