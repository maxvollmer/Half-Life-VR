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
#ifndef WEAPONS_H
#define WEAPONS_H

#include "effects.h"

#ifdef CLIENT_DLL
extern void VRRegisterRecoil(float intensity);
#endif

class CBasePlayer;
extern int gmsgWeapPickup;

void DeactivateSatchels(CBasePlayer* pOwner);

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	void Spawn(void);

	typedef enum
	{
		SATCHEL_DETONATE = 0,
		SATCHEL_RELEASE
	} SATCHELCODE;

	static CGrenade* ShootTimed(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float time);
	static CGrenade* ShootContact(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity);
	static void UseSatchelCharges(entvars_t* pevOwner, SATCHELCODE code);

	void Explode(Vector vecSrc, Vector vecAim);
	void Explode(TraceResult* pTrace, int bitsDamageType);
	void EXPORT Smoke(void);

	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT SlideTouch(CBaseEntity* pOther);
	void EXPORT ExplodeTouch(CBaseEntity* pOther);
	void EXPORT DangerSoundThink(void);
	void EXPORT PreDetonate(void);
	void EXPORT Detonate(void);
	void EXPORT DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT TumbleThink(void);

	virtual void BounceSound(void);
	virtual void Killed(entvars_t* pevAttacker, int bitsDamageType, int iGib);

	BOOL m_fRegisteredSound = FALSE;  // whether or not this grenade has issued its DANGER sound to the world sound list yet.


	virtual bool IsDraggable() override;
	virtual void HandleDragStart() override;
	virtual void HandleDragStop() override;
	virtual void HandleDragUpdate(const Vector& origin, const Vector& velocity, const Vector& angles) override;
	virtual void BaseBalled(CBaseEntity* pPlayer, const Vector& velocity) override;

private:
	void (CBaseEntity ::* m_backupThink)(void) = nullptr;
	void (CBaseEntity ::* m_backupTouch)(CBaseEntity* pOther) = nullptr;
};


// constant items
#define ITEM_HEALTHKIT 1
#define ITEM_ANTIDOTE  2
#define ITEM_SECURITY  3
#define ITEM_BATTERY   4

#define WEAPON_NONE    0
#define WEAPON_CROWBAR 1
#define WEAPON_GLOCK   2
#define WEAPON_PYTHON  3
#define WEAPON_MP5     4
//#define WEAPON_CHAINGUN			5
#define WEAPON_CROSSBOW    6
#define WEAPON_SHOTGUN     7
#define WEAPON_RPG         8
#define WEAPON_GAUSS       9
#define WEAPON_EGON        10
#define WEAPON_HORNETGUN   11
#define WEAPON_HANDGRENADE 12
#define WEAPON_TRIPMINE    13
#define WEAPON_SATCHEL     14
#define WEAPON_SNARK       15
#define WEAPON_BAREHAND    16

#define WEAPON_ALLWEAPONS (~(1 << WEAPON_SUIT))

#define WEAPON_SUIT 31  // ?????

#define MAX_WEAPONS 32


#define MAX_NORMAL_BATTERY 100


// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT     0
#define GLOCK_WEIGHT       10
#define PYTHON_WEIGHT      15
#define MP5_WEIGHT         15
#define SHOTGUN_WEIGHT     15
#define CROSSBOW_WEIGHT    10
#define RPG_WEIGHT         20
#define GAUSS_WEIGHT       20
#define EGON_WEIGHT        20
#define HORNETGUN_WEIGHT   10
#define HANDGRENADE_WEIGHT 5
#define SNARK_WEIGHT       5
#define SATCHEL_WEIGHT     -10
#define TRIPMINE_WEIGHT    -10


// weapon clip/carry ammo capacities
#define URANIUM_MAX_CARRY      100
#define _9MM_MAX_CARRY         250
#define _357_MAX_CARRY         36
#define BUCKSHOT_MAX_CARRY     125
#define BOLT_MAX_CARRY         50
#define ROCKET_MAX_CARRY       5
#define HANDGRENADE_MAX_CARRY  10
#define SATCHEL_MAX_CARRY      5
#define TRIPMINE_MAX_CARRY     10  // tripmines are fun, allow player to carry 10
#define SNARK_MAX_CARRY        15
#define HORNET_MAX_CARRY       8
#define M203_GRENADE_MAX_CARRY 10

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP -1

//#define CROWBAR_MAX_CLIP		WEAPON_NOCLIP
#define GLOCK_MAX_CLIP       17
#define PYTHON_MAX_CLIP      6
#define MP5_MAX_CLIP         50
#define MP5_DEFAULT_AMMO     25
#define SHOTGUN_MAX_CLIP     8
#define CROSSBOW_MAX_CLIP    5
#define RPG_MAX_CLIP         1
#define GAUSS_MAX_CLIP       WEAPON_NOCLIP
#define EGON_MAX_CLIP        WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP   WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP     WEAPON_NOCLIP
#define TRIPMINE_MAX_CLIP    WEAPON_NOCLIP
#define SNARK_MAX_CLIP       WEAPON_NOCLIP


// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE       17
#define PYTHON_DEFAULT_GIVE      6
#define MP5_DEFAULT_GIVE         25
#define MP5_DEFAULT_AMMO         25
#define MP5_M203_DEFAULT_GIVE    0
#define SHOTGUN_DEFAULT_GIVE     12
#define CROSSBOW_DEFAULT_GIVE    5
#define RPG_DEFAULT_GIVE         1
#define GAUSS_DEFAULT_GIVE       20
#define EGON_DEFAULT_GIVE        20
#define HANDGRENADE_DEFAULT_GIVE 5
#define SATCHEL_DEFAULT_GIVE     1
#define TRIPMINE_DEFAULT_GIVE    1
#define SNARK_DEFAULT_GIVE       5
#define HIVEHAND_DEFAULT_GIVE    8

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE   20
#define AMMO_GLOCKCLIP_GIVE    GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE       PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE      MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE     200
#define AMMO_M203BOX_GIVE      2
#define AMMO_BUCKSHOTBOX_GIVE  12
#define AMMO_CROSSBOWCLIP_GIVE CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE      RPG_MAX_CLIP
#define AMMO_URANIUMBOX_GIVE   20
#define AMMO_SNARKBOX_GIVE     5

// bullet types
typedef enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM,       // glock
	BULLET_PLAYER_MP5,       // mp5
	BULLET_PLAYER_357,       // python
	BULLET_PLAYER_BUCKSHOT,  // shotgun
	BULLET_PLAYER_CROWBAR,   // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;


#define ITEM_FLAG_SELECTONEMPTY     1
#define ITEM_FLAG_NOAUTORELOAD      2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY 4
#define ITEM_FLAG_LIMITINWORLD      8
#define ITEM_FLAG_EXHAUSTIBLE       16  // A player can totally exhaust their ammo supply and lose this weapon

typedef struct
{
	int iSlot = 0;
	int iPosition = 0;
	const char* pszAmmo1 = nullptr;  // ammo 1 type
	int iMaxAmmo1 = 0;         // max ammo 1
	const char* pszAmmo2 = nullptr;  // ammo 2 type
	int iMaxAmmo2 = 0;         // max ammo 2
	const char* pszName = nullptr;
	int iMaxClip = 0;
	int iId = 0;
	int iFlags = 0;
	int iWeight = 0;  // this value used to determine this weapon's importance in autoselection.
} ItemInfo;

typedef struct
{
	const char* pszName = nullptr;
	int iId = 0;
} AmmoInfo;

inline bool IsWeaponWithRange(int iId)
{
	return iId == WEAPON_GLOCK || iId == WEAPON_PYTHON || iId == WEAPON_MP5 || iId == WEAPON_CROSSBOW || iId == WEAPON_SHOTGUN || iId == WEAPON_RPG || iId == WEAPON_GAUSS || iId == WEAPON_EGON || iId == WEAPON_HORNETGUN;
}

inline bool IsWeapon(int iId)
{
	return iId == WEAPON_CROWBAR || iId == WEAPON_GLOCK || iId == WEAPON_PYTHON || iId == WEAPON_MP5 || iId == WEAPON_CROSSBOW || iId == WEAPON_SHOTGUN || iId == WEAPON_RPG || iId == WEAPON_GAUSS || iId == WEAPON_EGON || iId == WEAPON_HANDGRENADE || iId == WEAPON_TRIPMINE || iId == WEAPON_SATCHEL || iId == WEAPON_SNARK;
}

inline bool IsExplosiveWeapon(int iId)
{
	return iId == WEAPON_SNARK || iId == WEAPON_HANDGRENADE || iId == WEAPON_TRIPMINE || iId == WEAPON_SATCHEL;
}

inline bool IsWeaponWithVRLaserSpot(int iId)
{
	return IsWeaponWithRange(iId) && iId != WEAPON_RPG && iId != WEAPON_HORNETGUN;
}

// Items that the player has in their inventory that they can use
class CBasePlayerItem : public CBaseAnimating
{
public:
	virtual void SetObjectCollisionBox(void);

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	virtual int AddToPlayer(CBasePlayer* pPlayer);                      // return TRUE if the item you want the item added to the player inventory
	virtual int AddDuplicate(CBasePlayerItem* pItem) { return FALSE; }  // return TRUE if you want your duplicate removed from world
	void EXPORT DestroyItem(void);
	void EXPORT DefaultTouch(CBaseEntity* pOther);  // default weapon touch
	void EXPORT FallThink(void);                    // when an item is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize(void);                  // make a weapon visible and tangible
	void EXPORT AttemptToMaterialize(void);         // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn(void);                     // copy a weapon
	void FallInit(void);
	void CheckRespawn(void);
	virtual int GetItemInfo(ItemInfo* p) { return 0; };  // returns 0 if struct not filled out
	virtual BOOL CanDeploy(void) { return TRUE; };
	virtual BOOL Deploy()  // returns is deploy was successful
	{
		return TRUE;
	};

	virtual BOOL CanHolster(void) { return TRUE; };  // can this weapon be put away right now?
	virtual void Holster(int skiplocal = 0);
	virtual void UpdateItemInfo(void) { return; };

	virtual void ItemPreFrame(void) { return; }   // called each frame by the player PreThink
	virtual void ItemPostFrame(void) { return; }  // called each frame by the player PostThink

	virtual void Drop(void);
	virtual void Kill(void);
	virtual void AttachToPlayer(CBasePlayer* pPlayer);

	virtual int PrimaryAmmoIndex() { return -1; };
	virtual int SecondaryAmmoIndex() { return -1; };

	virtual int UpdateClientData(CBasePlayer* pPlayer) { return 0; }

	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];

	EHANDLE<CBasePlayer> m_pPlayer;
	EHANDLE<CBasePlayerItem> m_pNext;
	int m_iId = 0;  // WEAPON_???

	virtual int iItemSlot(void) { return 0; }  // return 0 to MAX_ITEMS_SLOTS, used in hud

	int iItemPosition(void) { return ItemInfoArray[m_iId].iPosition; }
	const char* pszAmmo1(void) { return ItemInfoArray[m_iId].pszAmmo1; }
	int iMaxAmmo1(void) { return ItemInfoArray[m_iId].iMaxAmmo1; }
	const char* pszAmmo2(void) { return ItemInfoArray[m_iId].pszAmmo2; }
	int iMaxAmmo2(void) { return ItemInfoArray[m_iId].iMaxAmmo2; }
	const char* pszName(void) { return ItemInfoArray[m_iId].pszName; }
	int iMaxClip(void) { return ItemInfoArray[m_iId].iMaxClip; }
	int iWeight(void) { return ItemInfoArray[m_iId].iWeight; }
	int iFlags(void) { return ItemInfoArray[m_iId].iFlags; }

	// int		m_iIdPrimary = 0;										// Unique Id for primary ammo
	// int		m_iIdSecondary = 0;										// Unique Id for secondary ammo
};


// Simple class that wraps a weapon time difference,
// so that all weapons can be sped down using analog fire input
// - Max Vollmer, 2019-04-13
/* this doesn't work, leave it for now
class WeaponTime
{
public:
	WeaponTime(float value) :
		m_weaponTimeBase{ UTIL_WeaponTimeBase() },
		m_offset{ value - UTIL_WeaponTimeBase() }
	{
		if (m_offset == std::numeric_limits<float>::infinity())
		{
			// TODO: Fix this
			m_offset = 1.f;
		}
	}
	WeaponTime(const WeaponTime& other) :
		m_weaponTimeBase{ other.m_weaponTimeBase },
		m_offset{ other.m_offset }
	{
		if (m_offset == std::numeric_limits<float>::infinity())
		{
			// TODO: Fix this
			m_offset = 1.f;
		}
	}
	WeaponTime() {}

	operator float()
	{
		extern float CalculateWeaponTimeOffset(float offset);
		return m_weaponTimeBase + CalculateWeaponTimeOffset(m_offset);
	}

	// we need the raw value for sending time to client (used in client.cpp)
	const float GetRaw() const { return m_weaponTimeBase + m_offset; }

	// Used in CBasePlayer::PostThink, (the original statement caused the value to grow to infinity)
	void DecrementTo(float target)
	{
		extern float CalculateWeaponTimeOffsetReverse(float offset);
		float decrementedValue = float(*this) - gpGlobals->frametime;
		if (decrementedValue < target)
		{
			(*this) = CalculateWeaponTimeOffsetReverse(target);
		}
		else
		{
			m_weaponTimeBase = UTIL_WeaponTimeBase();
			m_offset = CalculateWeaponTimeOffsetReverse(decrementedValue - UTIL_WeaponTimeBase());
		}
	}

private:
	float m_weaponTimeBase{ 0.f };
	float m_offset{ 0.f };
};
*/

// inventory items that
class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);


	virtual void KeyValue(KeyValueData* pkvd) override
	{
		CBasePlayerItem::KeyValue(pkvd);
		// Hackhack, set pev->scale here for all world weapons,
		// as they are way too big in VR.
		// TODO: Once all weapon models are fixed, remove this code.
		pev->scale = 0.7f;
	}


	static TYPEDESCRIPTION m_SaveData[];

	// generic weapon versions of CBasePlayerItem calls
	virtual int AddToPlayer(CBasePlayer* pPlayer);
	virtual int AddDuplicate(CBasePlayerItem* pItem);

	virtual int ExtractAmmo(CBasePlayerWeapon* pWeapon);      //{ return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo(CBasePlayerWeapon* pWeapon);  // { return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up

	virtual int AddWeapon(void)
	{
		ExtractAmmo(this);
		return TRUE;
	};  // Return TRUE if you want to add yourself to the player

	// generic "shared" ammo handlers
	BOOL AddPrimaryAmmo(int iCount, const char* szName, int iMaxClip, int iMaxCarry);
	BOOL AddSecondaryAmmo(int iCount, const char* szName, int iMaxCarry);

	virtual void UpdateItemInfo(void) {};  // updates HUD state

	int m_iPlayEmptySound = 0;
	int m_fFireOnEmpty = 0;  // True when the gun is empty and the player is still holding down the
						 // attack key(s)
	virtual BOOL PlayEmptySound(void);
	virtual void ResetEmptySound(void);

	virtual void SendWeaponAnim(int iAnim, int skiplocal = 1, int body = 0);  // skiplocal is 1 if client is predicting weapon animations

	virtual BOOL CanDeploy(void);
	virtual BOOL IsUseable(void);
	BOOL DefaultDeploy(char* szViewModel, char* szWeaponModel, int iAnim, char* szAnimExt, int skiplocal = 0, int body = 0);
	int DefaultReload(int iClipSize, int iAnim, float fDelay, int body = 0);

	virtual void ItemPostFrame(void);  // called each frame by the player PostThink
	// called by CBasePlayerWeapons ItemPostFrame()
	virtual void PrimaryAttack(void) { return; }         // do "+ATTACK"
	virtual void SecondaryAttack(void) { return; }       // do "+ATTACK2"
	virtual void Reload(void) { return; }                // do "+RELOAD"
	virtual void WeaponIdle(void) { return; }            // called when no buttons pressed
	virtual int UpdateClientData(CBasePlayer* pPlayer);  // sends hud info to client dll, if things have changed
	virtual void RetireWeapon(void);
	virtual BOOL ShouldWeaponIdle(void) { return FALSE; };
	virtual void Holster(int skiplocal = 0);
	virtual BOOL UseDecrement(void) { return FALSE; };

	int PrimaryAmmoIndex();
	int SecondaryAmmoIndex();

	void PrintState(void);

	float m_flPumpTime = 0.f;
	int m_fInSpecialReload = 0;         // Are we in the middle of a reload for the shotguns
	float m_flNextPrimaryAttack = 0.f;    // soonest time ItemPostFrame will call PrimaryAttack
	float m_flNextSecondaryAttack = 0.f;  // soonest time ItemPostFrame will call SecondaryAttack
	float m_flTimeWeaponIdle = 0.f;       // soonest time ItemPostFrame will call WeaponIdle
	int m_iPrimaryAmmoType = 0;         // "primary" ammo index into players m_rgAmmo[]
	int m_iSecondaryAmmoType = 0;       // "secondary" ammo index into players m_rgAmmo[]
	int m_iClip = 0;                    // number of shots left in the primary weapon clip, -1 it not used
	int m_iClientClip = 0;              // the last version of m_iClip sent to hud dll
	int m_iClientWeaponState = 0;       // the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int m_fInReload = 0;                // Are we in the middle of a reload;

	int m_iDefaultAmmo = 0;  // how much ammo you get when you pick up this weapon as placed by a level designer.
};


class CBasePlayerAmmo : public CBaseEntity
{
public:
	virtual void Spawn(void);
	void EXPORT DefaultTouch(CBaseEntity* pOther);  // default weapon touch
	virtual BOOL AddAmmo(CBaseEntity* pOther) { return TRUE; };

	CBaseEntity* Respawn(void);
	void EXPORT Materialize(void);
};


extern DLL_GLOBAL short g_sModelIndexLaser;  // holds the index for the laser beam
extern DLL_GLOBAL const char* g_pModelNameLaser;

extern DLL_GLOBAL short g_sModelIndexLaserDot;    // holds the index for the laser beam dot
extern DLL_GLOBAL short g_sModelIndexFireball;    // holds the index for the fireball
extern DLL_GLOBAL short g_sModelIndexSmoke;       // holds the index for the smoke cloud
extern DLL_GLOBAL short g_sModelIndexWExplosion;  // holds the index for the underwater explosion
extern DLL_GLOBAL short g_sModelIndexBubbles;     // holds the index for the bubbles model
extern DLL_GLOBAL short g_sModelIndexBloodDrop;   // holds the sprite index for blood drops
extern DLL_GLOBAL short g_sModelIndexBloodSpray;  // holds the sprite index for blood spray (bigger)

extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker);
extern void AddMultiDamage(entvars_t* pevInflictor, CBaseEntity* pEntity, float flDamage, int bitsDamageType);

extern void DecalGunshot(TraceResult* pTrace, int iBulletType);
extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern int DamageDecal(CBaseEntity* pEntity, int bitsDamageType);
extern void RadiusDamage(Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType);

typedef struct
{
	EHANDLE<CBaseEntity> pEntity;
	float amount = 0.f;
	int type = 0;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;


#define LOUD_GUN_VOLUME   1000
#define NORMAL_GUN_VOLUME 600
#define QUIET_GUN_VOLUME  200

#define BRIGHT_GUN_FLASH 512
#define NORMAL_GUN_FLASH 256
#define DIM_GUN_FLASH    128

#define BIG_EXPLOSION_VOLUME    2048
#define NORMAL_EXPLOSION_VOLUME 1024
#define SMALL_EXPLOSION_VOLUME  512

#define WEAPON_ACTIVITY_VOLUME 64

#define VECTOR_CONE_1DEGREES  Vector(0.00873, 0.00873, 0.00873)
#define VECTOR_CONE_2DEGREES  Vector(0.01745, 0.01745, 0.01745)
#define VECTOR_CONE_3DEGREES  Vector(0.02618, 0.02618, 0.02618)
#define VECTOR_CONE_4DEGREES  Vector(0.03490, 0.03490, 0.03490)
#define VECTOR_CONE_5DEGREES  Vector(0.04362, 0.04362, 0.04362)
#define VECTOR_CONE_6DEGREES  Vector(0.05234, 0.05234, 0.05234)
#define VECTOR_CONE_7DEGREES  Vector(0.06105, 0.06105, 0.06105)
#define VECTOR_CONE_8DEGREES  Vector(0.06976, 0.06976, 0.06976)
#define VECTOR_CONE_9DEGREES  Vector(0.07846, 0.07846, 0.07846)
#define VECTOR_CONE_10DEGREES Vector(0.08716, 0.08716, 0.08716)
#define VECTOR_CONE_15DEGREES Vector(0.13053, 0.13053, 0.13053)
#define VECTOR_CONE_20DEGREES Vector(0.17365, 0.17365, 0.17365)

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo.
//=========================================================
class CWeaponBox : public CBaseEntity
{
	void Precache(void);
	void Spawn(void);
	void Touch(CBaseEntity* pOther);
	void KeyValue(KeyValueData* pkvd);
	BOOL IsEmpty(void);
	int GiveAmmo(int iCount, const char* szName, int iMax, int* pIndex = nullptr) override;
	void SetObjectCollisionBox(void);

public:
	void EXPORT Kill(void);
	int Save(CSave& save);
	int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];

	BOOL HasWeapon(CBasePlayerItem* pCheckItem);
	BOOL PackWeapon(CBasePlayerItem* pWeapon);
	BOOL PackAmmo(int iszName, int iCount);

	EHANDLE<CBasePlayerItem> m_rgpPlayerItems[MAX_ITEM_TYPES];  // one slot for each

	int m_rgiszAmmo[MAX_AMMO_SLOTS] = { 0 };  // ammo names
	int m_rgAmmo[MAX_AMMO_SLOTS] = { 0 };     // ammo quantities

	int m_cAmmoTypes = 0;  // how many ammo types packed into this box (if packed by a level designer)
};

#ifdef CLIENT_DLL
bool bIsMultiplayer(void);
void LoadVModel(char* szViewModel, CBasePlayer* m_pPlayer);
#endif

class CGlock : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 2; }
	int GetItemInfo(ItemInfo* p);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void GlockFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);
	BOOL Deploy(void);
	void Reload(void);
	void WeaponIdle(void);

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	int m_iShell = 0;


	unsigned short m_usFireGlock1 = 0;
	unsigned short m_usFireGlock2 = 0;
};


float GetMeleeSwingSpeed();
float GetWeaponScale(const char* weaponModelName);

class CCrowbar : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 1; }
	void EXPORT SwingAgain(void);
	void EXPORT Smack(void);
	int GetItemInfo(ItemInfo* p);

	virtual void ItemPostFrame(void);

	void PrimaryAttack(void);
	int Swing(int fFirst);
	BOOL Deploy(void);
	void Holster(int skiplocal = 0);
	int m_iSwing = 0;
	TraceResult m_trHit;

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usCrowbar = 0;

#ifndef CLIENT_DLL
	// Stuff for VR swinging
	bool playedWooshSound = false;
	float lastWooshSoundTime = 0;
#endif
};

class CPython : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 2; }
	int GetItemInfo(ItemInfo* p);
	int AddToPlayer(CBasePlayer* pPlayer);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void Holster(int skiplocal = 0);
	void Reload(void);
	void WeaponIdle(void);
	float m_flSoundDelay = 0.f;

	BOOL m_fInZoom = FALSE;  // don't save this.

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usFirePython = 0;
};

class CMP5 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 3; }
	int GetItemInfo(ItemInfo* p);
	int AddToPlayer(CBasePlayer* pPlayer);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	int SecondaryAmmoIndex(void);
	BOOL Deploy(void);
	void Reload(void);
	void WeaponIdle(void);
	float m_flNextAnimTime = 0.f;
	int m_iShell = 0;

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usMP5 = 0;
	unsigned short m_usMP52 = 0;
};

class CCrossbow : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot() { return 3; }
	int GetItemInfo(ItemInfo* p);

	void FireBolt(void);
	void FireSniperBolt(void);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	int AddToPlayer(CBasePlayer* pPlayer);
	BOOL Deploy();
	void Holster(int skiplocal = 0);
	void Reload(void);
	void WeaponIdle(void);

	// no zoom in VR!
	//int m_fInZoom; // don't save this

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usCrossbow = 0;
	unsigned short m_usCrossbow2 = 0;
};

class CShotgun : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save(CSave& save);
	int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
#endif


	void Spawn(void);
	void Precache(void);
	int iItemSlot() { return 3; }
	int GetItemInfo(ItemInfo* p);
	int AddToPlayer(CBasePlayer* pPlayer);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy();
	void Reload(void);
	void WeaponIdle(void);
	int m_fInReload = 0;
	float m_flNextReload = 0.f;
	int m_iShell = 0;

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usDoubleFire = 0;
	unsigned short m_usSingleFire = 0;
};

class CLaserSpot : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);

	int ObjectCaps(void) { return FCAP_DONT_SAVE; }

public:
	void Suspend(float flSuspendTime);
	void EXPORT Revive(void);

	static CLaserSpot* CreateSpot(void);
};

class CRpg : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save(CSave& save);
	int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	void Reload(void);
	int iItemSlot(void) { return 4; }
	int GetItemInfo(ItemInfo* p);
	int AddToPlayer(CBasePlayer* pPlayer);

	BOOL Deploy(void);
	BOOL CanHolster(void);
	void Holster(int skiplocal = 0);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);

	void UpdateSpot(void);
	BOOL ShouldWeaponIdle(void) { return TRUE; };

	EHANDLE<CLaserSpot> m_pSpot;
	int m_fSpotActive = 0;
	int m_cActiveRockets = 0;  // how many missiles in flight from this launcher right now?

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usRpg = 0;
};

class CRpgRocket : public CGrenade
{
public:
	int Save(CSave& save);
	int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
	void Spawn(void);
	void Precache(void);
	void EXPORT FollowThink(void);
	void EXPORT IgniteThink(void);
	void EXPORT RocketTouch(CBaseEntity* pOther);
	static CRpgRocket* CreateRpgRocket(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, CRpg* pLauncher);

	int m_iTrail = 0;
	float m_flIgniteTime = 0.f;
	EHANDLE<CRpg> m_pLauncher;  // pointer back to the launcher that fired me.
};

class CGauss : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save(CSave& save);
	int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 4; }
	int GetItemInfo(ItemInfo* p);
	int AddToPlayer(CBasePlayer* pPlayer);

	BOOL Deploy(void);
	void Holster(int skiplocal = 0);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);

	void StartFire(void);
	void Fire(Vector vecOrigSrc, Vector vecDirShooting, float flDamage);
	float GetFullChargeTime(void);
	int m_iBalls = 0;
	int m_iGlow = 0;
	int m_iBeam = 0;
	int m_iSoundState = 0;  // don't save this

	// was this weapon just fired primary or secondary?
	// we need to know so we can pick the right set of effects.
	BOOL m_fPrimaryFire = FALSE;

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usGaussFire = 0;
	unsigned short m_usGaussSpin = 0;
};

class CEgon : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save(CSave& save);
	int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 4; }
	int GetItemInfo(ItemInfo* p);
	int AddToPlayer(CBasePlayer* pPlayer);

	BOOL Deploy(void);
	void Holster(int skiplocal = 0);

	void UpdateEffect(const Vector& startPoint, const Vector& endPoint, float timeBlend);

	void CreateEffect(void);
	void DestroyEffect(void);

	void EndAttack(void);
	void Attack(void);
	void PrimaryAttack(void);
	void WeaponIdle(void);

	float m_flAmmoUseTime = 0.f;  // since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval(void);
	float GetDischargeInterval(void);

	void Fire(const Vector& vecOrigSrc, const Vector& vecDir);

	BOOL HasAmmo(void);

	void UseAmmo(int count);

	enum EGON_FIREMODE
	{
		FIRE_NARROW,
		FIRE_WIDE
	};

	EHANDLE<CBeam> m_pBeam;
	EHANDLE<CBeam> m_pNoise;
	EHANDLE<CSprite> m_pSprite;

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

	unsigned short m_usEgonStop = 0;

private:
	float m_shootTime = 0.f;
	EGON_FIREMODE m_fireMode = FIRE_NARROW;
	float m_shakeTime = 0.f;
	BOOL m_deployed = FALSE;

	unsigned short m_usEgonFire = 0;
};

class CHgun : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 4; }
	int GetItemInfo(ItemInfo* p);
	int AddToPlayer(CBasePlayer* pPlayer);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	BOOL IsUseable(void);
	void Holster(int skiplocal = 0);
	void Reload(void);
	void WeaponIdle(void);
	float m_flNextAnimTime = 0.f;

	float m_flRechargeTime = 0.f;

	int m_iFirePhase = 0;  // don't save me.

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usHornetFire = 0;
};



class CHandGrenade : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 5; }
	int GetItemInfo(ItemInfo* p);

	void PrimaryAttack(void);
	BOOL Deploy(void);
	BOOL CanHolster(void);
	void Holster(int skiplocal = 0);
	void WeaponIdle(void);

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}
};

class CSatchel : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save(CSave& save);
	int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 5; }
	int GetItemInfo(ItemInfo* p);
	int AddToPlayer(CBasePlayer* pPlayer);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	int AddDuplicate(CBasePlayerItem* pOriginal);
	BOOL CanDeploy(void);
	BOOL Deploy(void);
	BOOL IsUseable(void);

	void Holster(int skiplocal = 0);
	void WeaponIdle(void);
	void Throw(void);

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}
};


class CTripmine : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 5; }
	int GetItemInfo(ItemInfo* p);
	void SetObjectCollisionBox(void)
	{
		//!!!BUGBUG - fix the model!
		pev->absmin = pev->origin + Vector(-16, -16, -5);
		pev->absmax = pev->origin + Vector(16, 16, 28);
	}

	void PrimaryAttack(void);
	BOOL Deploy(void);
	void Holster(int skiplocal = 0);
	void WeaponIdle(void);

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usTripFire = 0;

#ifndef CLIENT_DLL
	// "ghost" tripmine used in VR to show where the mine will be placed
	void UpdateGhost();
	EHANDLE<CBaseEntity> m_hGhost;
#endif
};

class CSqueak : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 5; }
	int GetItemInfo(ItemInfo* p);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void Holster(int skiplocal = 0);
	void WeaponIdle(void);
	int m_fJustThrown = 0;

	void Throw();

	virtual BOOL UseDecrement(void)
	{
#if defined(CLIENT_WEAPONS)
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usSnarkFire = 0;
};


#endif  // WEAPONS_H
