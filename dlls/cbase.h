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

Class Hierachy

CBaseEntity
	CBaseDelay
		CBaseToggle
			CBaseItem
			CBaseMonster
				CBaseCycler
				CBasePlayer
				CBaseGroup
*/

#define MAX_PATH_SIZE 10  // max number of nodes available for a path.

// These are caps bits to indicate what an object's capabilities (currently used for save/restore and level transitions)
#define FCAP_CUSTOMSAVE        0x00000001
#define FCAP_ACROSS_TRANSITION 0x00000002  // should transfer between transitions
#define FCAP_MUST_SPAWN        0x00000004  // Spawn after restore
#define FCAP_DONT_SAVE         0x80000000  // Don't save this
#define FCAP_IMPULSE_USE       0x00000008  // can be used by the player
#define FCAP_CONTINUOUS_USE    0x00000010  // can be used by the player
#define FCAP_ONOFF_USE         0x00000020  // can be used by the player
#define FCAP_DIRECTIONAL_USE   0x00000040  // Player sends +/- 1 when using (currently only tracktrains)
#define FCAP_MASTER            0x00000080  // Can be used to "master" other entities (like multisource)

// UNDONE: This will ignore transition volumes (trigger_transition), but not the PVS!!!
#define FCAP_FORCE_TRANSITION 0x00000080  // ALWAYS goes across transitions

#include "saverestore.h"
#include "schedule.h"

#ifndef MONSTEREVENT_H
#include "monsterevent.h"
#endif

#include <functional>
#include <unordered_set>

#include "VRCommons.h"

// C functions for external declarations that call the appropriate C++ methods

#ifdef _WIN32
#define EXPORT _declspec(dllexport)
#else
#define EXPORT /* */
#endif

extern "C" EXPORT int GetEntityAPI(DLL_FUNCTIONS * pFunctionTable, int interfaceVersion);
extern "C" EXPORT int GetEntityAPI2(DLL_FUNCTIONS * pFunctionTable, int* interfaceVersion);

extern int DispatchSpawn(edict_t* pent);
extern void DispatchKeyValue(edict_t* pentKeyvalue, KeyValueData* pkvd);
extern void DispatchTouch(edict_t* pentTouched, edict_t* pentOther);
extern void DispatchUse(edict_t* pentUsed, edict_t* pentOther);
extern void DispatchThink(edict_t* pent);
extern void DispatchBlocked(edict_t* pentBlocked, edict_t* pentOther);
extern void DispatchSave(edict_t* pent, SAVERESTOREDATA* pSaveData);
extern int DispatchRestore(edict_t* pent, SAVERESTOREDATA* pSaveData, int globalEntity);
extern void DispatchObjectCollsionBox(edict_t* pent);
extern void SaveWriteFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount);
extern void SaveReadFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount);
extern void SaveGlobalState(SAVERESTOREDATA* pSaveData);
extern void RestoreGlobalState(SAVERESTOREDATA* pSaveData);
extern void ResetGlobalState(void);

typedef enum
{
	USE_OFF = 0,
	USE_ON = 1,
	USE_SET = 2,
	USE_TOGGLE = 3
} USE_TYPE;

extern void FireTargets(const char* targetName, CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

typedef void (CBaseEntity::* BASEPTR)(void);
typedef void (CBaseEntity::* ENTITYFUNCPTR)(CBaseEntity* pOther);
typedef void (CBaseEntity::* USEPTR)(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

// For CLASSIFY
#define CLASS_NONE             0
#define CLASS_MACHINE          1
#define CLASS_PLAYER           2
#define CLASS_HUMAN_PASSIVE    3
#define CLASS_HUMAN_MILITARY   4
#define CLASS_ALIEN_MILITARY   5
#define CLASS_ALIEN_PASSIVE    6
#define CLASS_ALIEN_MONSTER    7
#define CLASS_ALIEN_PREY       8
#define CLASS_ALIEN_PREDATOR   9
#define CLASS_INSECT           10
#define CLASS_PLAYER_ALLY      11
#define CLASS_PLAYER_BIOWEAPON 12  // hornets and snarks.launched by players
#define CLASS_ALIEN_BIOWEAPON  13  // hornets and snarks.launched by the alien menace
#define CLASS_BARNACLE         99  // special because no one pays attention to it, and it eats a wide cross-section of creatures.

class CBaseEntity;
class CBaseMonster;
class CBasePlayerItem;
class CSquadMonster;
class CTalkMonster;


#define SF_NORESPAWN (1 << 30)  // !!!set this bit on guns and stuff that should never respawn.

#ifndef CLIENT_DLL
//
// EHANDLE<CBaseEntity>. Safe way to point to CBaseEntities who may die between frames
//
template <class ENTITY>
class EHANDLE
{
private:
	edict_t* m_pent{ nullptr };
	int m_serialnumber{ 0 };

public:
	EHANDLE()
	{
	}

	EHANDLE(ENTITY* pEntity)
	{
		if (pEntity && pEntity->pev)
		{
			m_pent = ENT(pEntity->pev);
			if (m_pent)
				m_serialnumber = m_pent->serialnumber;
		}
	}

	template <class ENTITY2>
	EHANDLE(const EHANDLE<ENTITY2>& other) :
		EHANDLE{ dynamic_cast<ENTITY*>(static_cast<CBaseEntity*>(const_cast<EHANDLE<ENTITY2>&>(other).operator ENTITY2* ())) }
	{
	}

	template<>
	EHANDLE(const EHANDLE<ENTITY>& other)
	{
		m_pent = other.m_pent;
		m_serialnumber = other.m_serialnumber;
	}

	edict_t* Get(void) const
	{
		if ((FWorldEnt(m_pent) || !FNullEnt(m_pent)) && m_pent->serialnumber == m_serialnumber)
		{
			return m_pent;
		}
		return nullptr;
	};

	edict_t* Set(edict_t* pent)
	{
		m_pent = pent;
		if (pent)
		{
			m_serialnumber = m_pent->serialnumber;
		}
		else
		{
			m_serialnumber = 0;
		}
		return pent;
	};

	operator ENTITY* ()
	{
		return static_cast<ENTITY*>(GET_PRIVATE(Get()));
	};

	operator const ENTITY* () const
	{
		return static_cast<const ENTITY*>(GET_PRIVATE(Get()));
	};

	operator int() const
	{
		return Get() != nullptr;
	}

	ENTITY* operator->()
	{
		return static_cast<ENTITY*>(GET_PRIVATE(Get()));
	}

	const ENTITY* operator->() const
	{
		return static_cast<ENTITY*>(GET_PRIVATE(Get()));
	}

	template<class ENTITY2>
	bool operator==(const EHANDLE<ENTITY2>& other) const
	{
		return Get() == other.Get();
	}

	template<class ENTITY2>
	bool operator!=(const EHANDLE<ENTITY2>& other) const
	{
		return !(operator==(other));
	}

	class Hash
	{
	public:
		std::size_t operator()(const EHANDLE& e) const
		{
			std::hash<int> intHasher;
			std::hash<edict_t*> entHasher;
			return intHasher(e.m_serialnumber) ^ entHasher(e.m_pent);
		}
	};

	class Equal
	{
	public:
		bool operator()(const EHANDLE& e1, const EHANDLE& e2) const
		{
			return e1.m_serialnumber == e2.m_serialnumber && e1.m_pent == e2.m_pent;
		}
	};
};
#else
//
// EHANDLE<CBaseEntity>.
// In client.dll we simply hold the entity pointer, since they are all fake global entities that live for the duration of the game anyways
//
template <class ENTITY>
class EHANDLE
{
private:
	ENTITY* ent{ nullptr };

public:
	EHANDLE()
	{
	}

	EHANDLE(ENTITY* pEntity)
	{
		ent = pEntity;
	}

	template <class ENTITY2>
	EHANDLE(EHANDLE<ENTITY2> other) :
		EHANDLE{ dynamic_cast<ENTITY*>(static_cast<CBaseEntity*>(other.operator ENTITY2* ())) }
	{
	}

	edict_t* Get(void) const
	{
		return nullptr;
	};

	edict_t* Set(edict_t* pent)
	{
		return pent;
	};

	operator ENTITY* ()
	{
		return ent;
	};

	operator int() const
	{
		return ent != nullptr;
	}

	ENTITY* operator->()
	{
		return ent;
	}

	template <class ENTITY2>
	bool operator==(EHANDLE<ENTITY2>& other)
	{
		return ent == other.ent;
	}

	template <class ENTITY2>
	bool operator!=(EHANDLE<ENTITY2>& other)
	{
		return !(operator==(other));
	}

	class Hash
	{
	public:
		std::size_t operator()(const EHANDLE& e) const
		{
			std::hash<ENTITY*> entHasher;
			return entHasher(e.ent);
		}
	};

	class Equal
	{
	public:
		bool operator()(const EHANDLE& e1, const EHANDLE& e2) const
		{
			return e1.ent == e2.ent;
		}
	};
};
#endif



// For real rotation of rotating buttons in VR - Max Vollmer, 2019-05-26
class VRRotatableEnt;


//
// Base Entity.  All entity types derive from this
//
class CBaseEntity
{
public:
	// Constructor.  Set engine to use C/C++ callback functions
	// pointers to engine data
	entvars_t* pev = nullptr;  // Don't need to save/restore this pointer, the engine resets it

	// path corners
	EHANDLE<CBaseEntity> m_pGoalEnt;  // path corner we are heading towards
	EHANDLE<CBaseEntity> m_pLink;     // used for temporary link-list operations.

	// initialization functions
	virtual void Spawn(void) { return; }
	virtual void Precache(void) { return; }
	virtual void KeyValue(KeyValueData* pkvd) { pkvd->fHandled = FALSE; }
	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);
	virtual int ObjectCaps(void) { return FCAP_ACROSS_TRANSITION; }
	virtual void Activate(void) {}

	// Setup the object->object collision box (pev->mins / pev->maxs is the object->world collision box)
	virtual void SetObjectCollisionBox(void);

	// Classify - returns the type of group (i.e, "houndeye", or "human military" so that monsters with different classnames
	// still realize that they are teammates. (overridden for monsters that form groups)
	virtual int Classify(void) { return CLASS_NONE; };
	virtual void DeathNotice(entvars_t* pevChild) {}  // monster maker children use this to tell the monster maker that they have died.


	static TYPEDESCRIPTION m_SaveData[];

	virtual void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	virtual int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int TakeHealth(float flHealth, int bitsDamageType);
	virtual void Killed(entvars_t* pevAttacker, int bitsDamageType, int iGib);

	// Made BloodColor() non-virtual and moved m_bloodColor member here
	// to avoid issues when temporarily disabling blood in VR melee code.
	// - Max Vollmer, 2019-04-13
	int m_bloodColor{ DONT_BLEED };
	int BloodColor() { return m_bloodColor; }

	virtual void TraceBleed(float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	virtual BOOL IsTriggered(CBaseEntity* pActivator) { return TRUE; }
	virtual int GetToggleState(void) { return TS_AT_TOP; }
	virtual void AddPoints(int score, BOOL bAllowNegativeScore) {}
	virtual void AddPointsToTeam(int score, BOOL bAllowNegativeScore) {}
	virtual BOOL AddPlayerItem(CBasePlayerItem* pItem) { return 0; }
	virtual BOOL RemovePlayerItem(CBasePlayerItem* pItem) { return 0; }
	virtual int GiveAmmo(int iAmount, const char* szName, int iMax, int* pIndex = nullptr) { return -1; };
	virtual float GetDelay(void) { return 0; }
	virtual int IsMoving(void) { return pev->velocity != g_vecZero; }
	virtual void OverrideReset(void) {}
	virtual int DamageDecal(int bitsDamageType);
	// This is ONLY used by the node graph to test movement through a door
	virtual void SetToggleState(int state) {}
	virtual void StartSneaking(void) {}
	virtual void StopSneaking(void) {}
	virtual BOOL OnControls(entvars_t* pevOther) { return FALSE; }
	virtual BOOL IsSneaking(void) { return FALSE; }
	virtual BOOL IsAlive(void) { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	virtual BOOL IsBSPModel(void) { return pev->solid == SOLID_BSP || pev->movetype == MOVETYPE_PUSHSTEP; }
	virtual BOOL ReflectGauss(void) { return (IsBSPModel() && !pev->takedamage); }
	virtual BOOL HasTarget(string_t targetname) { return FStrEq(STRING(targetname), STRING(pev->targetname)); }
	virtual BOOL IsInWorld(void);
	virtual BOOL IsPlayer(void) { return FALSE; }
	virtual BOOL IsNetClient(void) { return FALSE; }
	virtual const char* TeamID(void) { return ""; }

	// Moved SetTransparency from CSprite to CBaseEntity - Max Vollmer, 2017-08-27
	inline void SetTransparency(int rendermode, int r, int g, int b, int a, int fx)
	{
		pev->rendermode = rendermode;
		pev->rendercolor.x = r;
		pev->rendercolor.y = g;
		pev->rendercolor.z = b;
		pev->renderamt = a;
		pev->renderfx = fx;
	}

	//	virtual void	SetActivator( CBaseEntity *pActivator ) {}
	virtual CBaseEntity* GetNextTarget(void);

	// fundamental callbacks
	void (CBaseEntity ::* m_pfnThink)(void) = nullptr;
	void (CBaseEntity ::* m_pfnTouch)(CBaseEntity* pOther) = nullptr;
	void (CBaseEntity ::* m_pfnUse)(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) = nullptr;
	void (CBaseEntity ::* m_pfnBlocked)(CBaseEntity* pOther) = nullptr;

	virtual void Think(void)
	{
		if (m_pfnThink)
		{
			(this->*m_pfnThink)();
		}
	};
	virtual void Touch(CBaseEntity* pOther)
	{
		if (m_pfnTouch)
		{
			(this->*m_pfnTouch)(pOther);
		}
	};
	virtual void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
	{
		if (m_pfnUse)
			(this->*m_pfnUse)(pActivator, pCaller, useType, value);
	}
	virtual void Blocked(CBaseEntity* pOther)
	{
		if (m_pfnBlocked)
			(this->*m_pfnBlocked)(pOther);
	};

	// allow engine to allocate instance data
	void* operator new(size_t stAllocateBlock, entvars_t* pev)
	{
		return ALLOC_PRIVATE(ENT(pev), stAllocateBlock);
	};

	// don't use this.
#if _MSC_VER >= 1200  // only build this code if MSVC++ 6.0 or higher
	void operator delete(void* pMem, entvars_t* pev)
	{
		pev->flags |= FL_KILLME;
	};
#endif

	virtual void UpdateOnRemove();

	// common member functions
	void EXPORT SUB_Remove(void);
	void EXPORT SUB_DoNothing(void);
	void EXPORT SUB_StartFadeOut(void);
	void EXPORT SUB_FadeOut(void);
	void EXPORT SUB_CallUseToggle(void) { this->Use(this, this, USE_TOGGLE, 0); }
	int ShouldToggle(USE_TYPE useType, BOOL currentState);
	void FireBullets(ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int iDamage = 0, entvars_t* pevAttacker = nullptr);
	Vector FireBulletsPlayer(ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int iDamage = 0, entvars_t* pevAttacker = nullptr, int shared_rand = 0);

	virtual CBaseEntity* Respawn(void) { return nullptr; }

	void SUB_UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value);
	// Do the bounding boxes of these two intersect?
	int Intersects(CBaseEntity* pOther);
	void MakeDormant(void);
	int IsDormant(void);
	BOOL IsLockedByMaster(void) { return FALSE; }


	inline static CBaseEntity* InstanceOrWorld(const edict_t* pent)
	{
		if (FNullEnt(pent))
			return static_cast<CBaseEntity*>(GET_PRIVATE(ENT(0)));
		else
			return static_cast<CBaseEntity*>(GET_PRIVATE(pent));
	}

	inline static CBaseEntity* InstanceOrWorld(const entvars_t* pev) { return InstanceOrWorld(ENT(pev)); }
	inline static CBaseEntity* InstanceOrWorld(int eoffset) { return InstanceOrWorld(ENT(eoffset)); }

	inline static CBaseEntity* UnsafeInstance(const edict_t* pent)
	{
		return static_cast<CBaseEntity*>(GET_PRIVATE(pent));
	}

	template<class T>
	inline static EHANDLE<T> SafeInstance(const edict_t* pent)
	{
		if (FNullEnt(pent) && !FWorldEnt(pent))
		{
			return nullptr;
		}
		else
		{
			return dynamic_cast<T*>(static_cast<CBaseEntity*>(GET_PRIVATE(pent)));
		}
	}

	template<>
	inline static EHANDLE<CBaseEntity> SafeInstance(const edict_t* pent)
	{
		if (FNullEnt(pent) && !FWorldEnt(pent))
		{
			return nullptr;
		}
		else
		{
			return UnsafeInstance(pent);
		}
	}

	template<class T>
	inline static EHANDLE<T> SafeInstance(const entvars_t* pev)
	{
		return SafeInstance<T>(ENT(pev));
	}

	template<class T>
	inline static EHANDLE<T> SafeInstance(int eoffset)
	{
		return SafeInstance<T>(ENT(eoffset));
	}


	// Ugly code to lookup all functions to make sure they are exported when set.
#ifdef _DEBUG
	void FunctionCheck(const void* pFunction, const char* name)
	{
		if (pFunction && !NAME_FOR_FUNCTION(reinterpret_cast<unsigned long>(pFunction)))
		{
			ALERT(at_error, "No EXPORT: %s:%s (%08lx)\n", STRING(pev->classname), name, (unsigned long)pFunction);
		}
	}

	BASEPTR ThinkSet(BASEPTR func, const char* name)
	{
		m_pfnThink = func;
		FunctionCheck(reinterpret_cast<void*>(*(reinterpret_cast<std::size_t*>(reinterpret_cast<char*>(this) + (offsetof(CBaseEntity, m_pfnThink))))), name);
		return func;
	}
	ENTITYFUNCPTR TouchSet(ENTITYFUNCPTR func, const char* name)
	{
		m_pfnTouch = func;
		FunctionCheck(reinterpret_cast<void*>(*(reinterpret_cast<std::size_t*>(reinterpret_cast<char*>(this) + (offsetof(CBaseEntity, m_pfnTouch))))), name);
		return func;
	}
	USEPTR UseSet(USEPTR func, const char* name)
	{
		m_pfnUse = func;
		FunctionCheck(reinterpret_cast<void*>(*(reinterpret_cast<std::size_t*>(reinterpret_cast<char*>(this) + (offsetof(CBaseEntity, m_pfnUse))))), name);
		return func;
	}
	ENTITYFUNCPTR BlockedSet(ENTITYFUNCPTR func, const char* name)
	{
		m_pfnBlocked = func;
		FunctionCheck(reinterpret_cast<void*>(*(reinterpret_cast<std::size_t*>(reinterpret_cast<char*>(this) + (offsetof(CBaseEntity, m_pfnBlocked))))), name);
		return func;
	}

#endif


	// virtual functions used by a few classes

	// used by monsters that are created by the MonsterMaker
	virtual void UpdateOwner(void) { return; };


	// NOTE: szName must be a pointer to constant memory, e.g. "monster_class" because the entity
	// will keep a pointer to it after this call.
	template <class T>
	static T* Create(const char* szName, const Vector& vecOrigin, const Vector& vecAngles, edict_t* pentOwner = nullptr)
	{
		edict_t* pent = CREATE_NAMED_ENTITY(MAKE_STRING(szName));
		if (FNullEnt(pent))
		{
			ALERT(at_console, "nullptr pent in Create!\n");
			return nullptr;
		}

		CBaseEntity* pEntity = SafeInstance<CBaseEntity>(pent);
		if (!pEntity)
		{
			ALERT(at_console, "nullptr pEntity in Create!\n");
			REMOVE_ENTITY(pent);
			return nullptr;
		}

		T* pTypedEntity = dynamic_cast<T*>(pEntity);
		if (!pTypedEntity)
		{
			ALERT(at_console, "Incompatible class_name and type used in Create!\n");
			REMOVE_ENTITY(pent);
			return nullptr;
		}

		pTypedEntity->pev->owner = pentOwner;
		pTypedEntity->pev->origin = vecOrigin;
		pTypedEntity->pev->angles = vecAngles;
		DispatchSpawn(pTypedEntity->edict());
		return pTypedEntity;
	}

	virtual BOOL FBecomeProne(void) { return FALSE; };
	inline edict_t* edict() { return pev->pContainingEntity; };
	inline EOFFSET eoffset() { return OFFSET(edict()); };
	inline int entindex() { return ENTINDEX(edict()); };

	virtual Vector Center() { return (pev->absmax + pev->absmin) * 0.5; };  // center point of entity
	virtual Vector EyePosition() { return pev->origin + pev->view_ofs; };   // position of eyes
	virtual Vector EarPosition() { return pev->origin + pev->view_ofs; };   // position of ears
	virtual Vector BodyTarget(const Vector& posSrc) { return Center(); };   // position to shoot at

	virtual int Illumination() { return GETENTITYILLUM(ENT(pev)); };

	virtual BOOL FVisible(CBaseEntity* pEntity);
	virtual BOOL FVisible(const Vector& vecOrigin);

	//We use this variables to store each ammo count.
	int ammo_9mm = 0;
	int ammo_357 = 0;
	int ammo_bolts = 0;
	int ammo_buckshot = 0;
	int ammo_rockets = 0;
	int ammo_uranium = 0;
	int ammo_hornets = 0;
	int ammo_argrens = 0;
	//Special stuff for grenades and satchels.
	float m_flStartThrow = 0.f;
	float m_flReleaseThrow = 0.f;
	int m_chargeReady = 0;
	int m_fInAttack = 0;

	enum EGON_FIRESTATE
	{
		FIRE_OFF,
		FIRE_CHARGE
	};
	int m_fireState = 0;


	// For easy detection of female NPCs to change audio files in sound.cpp - Max Vollmer, 2018-11-23
	virtual bool IsFemaleNPC() { return false; }


	// This is used by the VR controller touch/interaction code to determine if something is touched directly after mapchange/load,
	// we want to treat those entities as not touched, because otherwise those levelchanges near the tentacle monster bring the payer
	// into an infinite mapchange-loop (as the next map has a button to go back in the same place)
	const float m_spawnTime{
#ifdef CLIENT_DLL
		0
#else
		gpGlobals->time
#endif
	};

	// Currently overriden by func_wall for gordon's coffee cup and retina scanners, and trigger_multiple for xen jump thingies
	virtual bool CheckIsSpecialVREntity() { return true; }

	// For real rotation of rotating buttons in VR - Max Vollmer, 2019-05-26
	virtual VRRotatableEnt* MyRotatableEntPtr() { return nullptr; }

	// Prevents being pushed up by xen jumps when using the teleporter on them
	virtual bool IsXenJumpTrigger() { return false; }

	// For draggable entities
	virtual bool IsDraggable() { return false; }
	virtual void HandleDragStart() {}
	virtual void HandleDragStop() {}
	virtual void HandleDragUpdate(const Vector& origin, const Vector& velocity, const Vector& angles)
	{
		// default implementation just copies values in
		pev->origin = origin;
		pev->velocity = velocity;
		pev->angles = angles;
	}
	virtual void BaseBalled(CBaseEntity* pPlayer, const Vector& velocity) {}
	void EXPORT DragStartThink(void)
	{
		m_pfnThink = nullptr;
		HandleDragStart();
	}
	void EXPORT DragStopThink(void)
	{
		m_pfnThink = nullptr;
		HandleDragStop();
	}
	void EXPORT DragThink(void);

	inline bool IsBeingDragged() { return m_vrDragger && m_vrDragController != VRControllerID::INVALID; }

	EHANDLE<CBaseEntity> m_vrDragger;
	VRControllerID m_vrDragController{ VRControllerID::INVALID };
	Vector m_vrDragOriginOffset;
	Vector m_vrDragAnglesOffset;

	// overriden by NPCs that react to the player throwing stuff at them
	virtual void GibAttack(EHANDLE<CBaseEntity> thrower, const Vector& pos, int bloodcolor) {}

	// Remember if entity is in PVS this frame (last frame? doesn't matter), set in AddToFullPack in client.cpp - Max Vollmer, 2020-03-08
	bool m_isInPVS = false;
};



// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a
// member function of a base class.  static_cast is a sleezy way around that problem.

#ifdef _DEBUG

#define SetThink(a)   ThinkSet(static_cast<void (CBaseEntity::*)(void)>(a), #a)
#define SetTouch(a)   TouchSet(static_cast<void (CBaseEntity::*)(CBaseEntity*)>(a), #a)
#define SetUse(a)     UseSet(static_cast<void (CBaseEntity::*)(CBaseEntity * pActivator, CBaseEntity * pCaller, USE_TYPE useType, float value)>(a), #a)
#define SetBlocked(a) BlockedSet(static_cast<void (CBaseEntity::*)(CBaseEntity*)>(a), #a)

#else

#define SetThink(a)   m_pfnThink = static_cast<void (CBaseEntity::*)(void)>(a)
#define SetTouch(a)   m_pfnTouch = static_cast<void (CBaseEntity::*)(CBaseEntity*)>(a)
#define SetUse(a)     m_pfnUse = static_cast<void (CBaseEntity::*)(CBaseEntity * pActivator, CBaseEntity * pCaller, USE_TYPE useType, float value)>(a)
#define SetBlocked(a) m_pfnBlocked = static_cast<void (CBaseEntity::*)(CBaseEntity*)>(a)

#endif


class CPointEntity : public CBaseEntity
{
public:
	void Spawn(void);
	virtual int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
};


typedef struct locksounds  // sounds that doors and buttons make when locked/unlocked
{
	string_t sLockedSound = iStringNull;       // sound a door makes when it's locked
	string_t sLockedSentence = iStringNull;    // sentence group played when door is locked
	string_t sUnlockedSound = iStringNull;     // sound a door makes when it's unlocked
	string_t sUnlockedSentence = iStringNull;  // sentence group played when door is unlocked

	int iLockedSentence = 0;    // which sentence in sentence group to play next
	int iUnlockedSentence = 0;  // which sentence in sentence group to play next

	float flwaitSound = 0.f;     // time delay between playing consecutive 'locked/unlocked' sounds
	float flwaitSentence = 0.f;  // time delay between playing consecutive sentences
	BYTE bEOFLocked = 0;       // true if hit end of list of locked sentences
	BYTE bEOFUnlocked = 0;     // true if hit end of list of unlocked sentences
} locksound_t;

void PlayLockSounds(entvars_t* pev, locksound_t* pls, int flocked, int fbutton);

//
// MultiSouce
//

#define MAX_MULTI_TARGETS 16  // maximum number of targets a single multi_manager entity may be assigned.
#define MS_MAX_TARGETS    32

class CMultiSource : public CPointEntity
{
public:
	void Spawn();
	void KeyValue(KeyValueData* pkvd);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	int ObjectCaps(void) { return (CPointEntity::ObjectCaps() | FCAP_MASTER); }
	BOOL IsTriggered(CBaseEntity* pActivator);
	void EXPORT Register(void);
	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE<CBaseEntity> m_rgEntities[MS_MAX_TARGETS];
	int m_rgTriggered[MS_MAX_TARGETS] = { 0 };

	int m_iTotal = 0;
	string_t m_globalstate = iStringNull;
};


//
// generic Delay entity.
//
class CBaseDelay : public CBaseEntity
{
public:
	float m_flDelay = 0.f;
	int m_iszKillTarget = 0;

	virtual void KeyValue(KeyValueData* pkvd);
	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];
	// common member functions
	void SUB_UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value);
	void EXPORT DelayThink(void);
};


class CBaseAnimating : public CBaseDelay
{
public:
	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	// Basic Monster Animation functions
	float StudioFrameAdvance(float flInterval = 0.0);  // accumulate animation frame time from last time called until now
	int GetSequenceFlags(void);
	int LookupActivity(int activity);
	int LookupActivityHeaviest(int activity);
	int LookupSequence(const char* label);
	void ResetSequenceInfo();
	void DispatchAnimEvents(float flFutureInterval = 0.1);                      // Handle events that have happend since last time called up until X seconds into the future
	virtual void HandleClientAnimEvent(ClientAnimEvent_t* pEvent) { return; };  // Client side events for VR controller weapon models (fixes some issues) - Max Vollmer - 2019-04-13
	virtual void HandleAnimEvent(MonsterEvent_t* pEvent) { return; };
	float SetBoneController(int iController, float flValue);
	void InitBoneControllers(void);
	float SetBlending(int iBlender, float flValue);
	void GetBonePosition(int iBone, Vector& origin, Vector& angles);
	void GetAutomovement(Vector& origin, Vector& angles, float flInterval = 0.1);
	int FindTransition(int iEndingSequence, int iGoalSequence, int* piDir);
	void GetAttachment(int iAttachment, Vector& origin, Vector& angles);
	void SetBodygroup(int iGroup, int iValue);
	int GetBodygroup(int iGroup);
	int ExtractBbox(int sequence, float* mins, float* maxs);
	void SetSequenceBox(void);

	// animation needs
	float m_flFrameRate = 0.f;       // computed FPS for current sequence
	float m_flGroundSpeed = 0.f;     // computed linear movement rate for current sequence
	float m_flLastEventCheck = 0.f;  // last time the event list was checked
	BOOL m_fSequenceFinished = FALSE;  // flag set when StudioAdvanceFrame moves across a frame boundry
	BOOL m_fSequenceLoops = FALSE;     // true if the sequence loops
};


//
// generic Toggle entity.
//
#define SF_ITEM_USE_ONLY 256  //  ITEM_USE_ONLY = BUTTON_USE_ONLY = DOOR_USE_ONLY!!!

class CBaseToggle : public CBaseAnimating
{
public:
	void KeyValue(KeyValueData* pkvd);

	TOGGLE_STATE m_toggle_state = TS_AT_TOP;
	float m_flActivateFinished = 0.f;  //like attack_finished, but for doors
	float m_flMoveDistance = 0.f;      // how far a door should slide or rotate
	float m_flWait = 0.f;
	float m_flLip = 0.f;
	float m_flTWidth = 0.f;   // for plats
	float m_flTLength = 0.f;  // for plats

	Vector m_vecPosition1;
	Vector m_vecPosition2;
	Vector m_vecAngle1;
	Vector m_vecAngle2;

	int m_cTriggersLeft = 0;  // trigger_counter only, # of activations remaining
	float m_flHeight = 0.f;
	EHANDLE<CBaseEntity> m_hActivator;
	void (CBaseToggle::* m_pfnCallWhenMoveDone)(void) = nullptr;
	Vector m_vecFinalDest;
	Vector m_vecFinalAngle;

	int m_bitsDamageInflict = 0;  // DMG_ damage type that the door or tigger does

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	virtual int GetToggleState(void) { return m_toggle_state; }
	virtual float GetDelay(void) { return m_flWait; }

	// common member functions
	void LinearMove(Vector vecDest, float flSpeed);
	void EXPORT LinearMoveDone(void);
	void AngularMove(Vector vecDestAngle, float flSpeed);
	void EXPORT AngularMoveDone(void);
	BOOL IsLockedByMaster(void);

	static float AxisValue(int flags, const Vector& angles);
	static void AxisDir(entvars_t* pev);
	static float AxisDelta(int flags, const Vector& angle1, const Vector& angle2);

	string_t m_sMaster = iStringNull;  // If this button has a master switch, this is the targetname.
						 // A master switch must be of the multisource type. If all
						 // of the switches in the multisource have been triggered, then
						 // the button will be allowed to operate. Otherwise, it will be
						 // deactivated.
};
#define SetMoveDone(a) m_pfnCallWhenMoveDone = static_cast<void (CBaseToggle::*)(void)>(a)


// people gib if their health is <= this at the time of death
#define GIB_HEALTH_VALUE -30

#define ROUTE_SIZE      8  // how many waypoints a monster can store at one time
#define MAX_OLD_ENEMIES 4  // how many old enemies to remember

#define bits_CAP_DUCK       (1 << 0)   // crouch
#define bits_CAP_JUMP       (1 << 1)   // jump/leap
#define bits_CAP_STRAFE     (1 << 2)   // strafe ( walk/run sideways)
#define bits_CAP_SQUAD      (1 << 3)   // can form squads
#define bits_CAP_SWIM       (1 << 4)   // proficiently navigate in water
#define bits_CAP_CLIMB      (1 << 5)   // climb ladders/ropes
#define bits_CAP_USE        (1 << 6)   // open doors/push buttons/pull levers
#define bits_CAP_HEAR       (1 << 7)   // can hear forced sounds
#define bits_CAP_AUTO_DOORS (1 << 8)   // can trigger auto doors
#define bits_CAP_OPEN_DOORS (1 << 9)   // can open manual doors
#define bits_CAP_TURN_HEAD  (1 << 10)  // can turn head, always bone controller 0

#define bits_CAP_RANGE_ATTACK1 (1 << 11)  // can do a range attack 1
#define bits_CAP_RANGE_ATTACK2 (1 << 12)  // can do a range attack 2
#define bits_CAP_MELEE_ATTACK1 (1 << 13)  // can do a melee attack 1
#define bits_CAP_MELEE_ATTACK2 (1 << 14)  // can do a melee attack 2

#define bits_CAP_FLY (1 << 15)  // can fly, move all around

#define bits_CAP_DOORS_GROUP (bits_CAP_USE | bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS)

// used by suit voice to indicate damage sustained and repaired type to player

// instant damage

#define DMG_GENERIC    0          // generic damage was done
#define DMG_CRUSH      (1 << 0)   // crushed by falling or moving object
#define DMG_BULLET     (1 << 1)   // shot
#define DMG_SLASH      (1 << 2)   // cut, clawed, stabbed
#define DMG_BURN       (1 << 3)   // heat burned
#define DMG_FREEZE     (1 << 4)   // frozen
#define DMG_FALL       (1 << 5)   // fell too far
#define DMG_BLAST      (1 << 6)   // explosive blast damage
#define DMG_CLUB       (1 << 7)   // crowbar, punch, headbutt
#define DMG_SHOCK      (1 << 8)   // electric shock
#define DMG_SONIC      (1 << 9)   // sound pulse shockwave
#define DMG_ENERGYBEAM (1 << 10)  // laser or other high energy beam
#define DMG_NEVERGIB   (1 << 12)  // with this bit OR'd in, no damage type will be able to gib victims upon death
#define DMG_ALWAYSGIB  (1 << 13)  // with this bit OR'd in, any damage type can be made to gib victims upon death.
#define DMG_DROWN      (1 << 14)  // Drowning
// time-based damage
#define DMG_TIMEBASED (~(0x3fff))  // mask for time-based damage

#define DMG_PARALYZE     (1 << 15)  // slows affected creature down
#define DMG_NERVEGAS     (1 << 16)  // nerve toxins, very bad
#define DMG_POISON       (1 << 17)  // blood poisioning
#define DMG_RADIATION    (1 << 18)  // radiation exposure
#define DMG_DROWNRECOVER (1 << 19)  // drowning recovery
#define DMG_ACID         (1 << 20)  // toxic chemicals or acid burns
#define DMG_SLOWBURN     (1 << 21)  // in an oven
#define DMG_SLOWFREEZE   (1 << 22)  // in a subzero freezer
#define DMG_MORTAR       (1 << 23)  // Hit by air raid (done to distinguish grenade from mortar)

// these are the damage types that are allowed to gib corpses
#define DMG_GIB_CORPSE (DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB)

// these are the damage types that have client hud art
#define DMG_SHOWNHUD (DMG_POISON | DMG_ACID | DMG_FREEZE | DMG_SLOWFREEZE | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK)

// NOTE: tweak these values based on gameplay feedback:

#define PARALYZE_DURATION 2    // number of 2 second intervals to take damage
#define PARALYZE_DAMAGE   1.0  // damage to take each 2 second interval

#define NERVEGAS_DURATION 2
#define NERVEGAS_DAMAGE   5.0

#define POISON_DURATION 5
#define POISON_DAMAGE   2.0

#define RADIATION_DURATION 2
#define RADIATION_DAMAGE   1.0

#define ACID_DURATION 2
#define ACID_DAMAGE   5.0

#define SLOWBURN_DURATION 2
#define SLOWBURN_DAMAGE   1.0

#define SLOWFREEZE_DURATION 2
#define SLOWFREEZE_DAMAGE   1.0


#define itbd_Paralyze     0
#define itbd_NerveGas     1
#define itbd_Poison       2
#define itbd_Radiation    3
#define itbd_DrownRecover 4
#define itbd_Acid         5
#define itbd_SlowBurn     6
#define itbd_SlowFreeze   7
#define CDMG_TIMEBASED    8

// when calling KILLED(), a value that governs gib behavior is expected to be
// one of these three values
#define GIB_NORMAL 0  // gib if entity was overkilled
#define GIB_NEVER  1  // never gib, no matter how much death damage is done ( freezing, etc )
#define GIB_ALWAYS 2  // always gib ( Houndeye Shock, Barnacle Bite )

class CBaseMonster;
class CCineMonster;
class CSound;

#include "basemonster.h"


char* ButtonSound(int sound);  // get string of button sound number


//
// Generic Button
//
class CBaseButton : public CBaseToggle
{
public:
	void Spawn(void);
	virtual void Precache(void);
	void RotSpawn(void);
	virtual void KeyValue(KeyValueData* pkvd);

	void ButtonActivate();
	void SparkSoundCache(void);

	void EXPORT ButtonShot(void);
	void EXPORT ButtonTouch(CBaseEntity* pOther);
	void EXPORT ButtonSpark(void);
	void EXPORT TriggerAndWait(void);
	void EXPORT ButtonReturn(void);
	void EXPORT ButtonBackHome(void);
	void EXPORT ButtonUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	virtual int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	enum BUTTON_CODE
	{
		BUTTON_NOTHING,
		BUTTON_ACTIVATE,
		BUTTON_RETURN
	};
	BUTTON_CODE ButtonResponseToTouch(void);

	static TYPEDESCRIPTION m_SaveData[];
	// Buttons that don't take damage can be IMPULSE used
	virtual int ObjectCaps(void) { return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | (pev->takedamage ? 0 : FCAP_IMPULSE_USE); }

	BOOL m_fStayPushed = FALSE;  // button stays pushed in until touched again?
	BOOL m_fRotating = FALSE;    // a rotating button?  default is a sliding button.

	string_t m_strChangeTarget = iStringNull;  // if this field is not null, this is an index into the engine string array.
								 // when this button is touched, it's target entity's TARGET field will be set
								 // to the button's ChangeTarget. This allows you to make a func_train switch paths, etc.

	locksound_t m_ls;  // door lock sounds

	BYTE m_bLockedSound = 0;  // ordinals from entity selection
	BYTE m_bLockedSentence = 0;
	BYTE m_bUnlockedSound = 0;
	BYTE m_bUnlockedSentence = 0;
	int m_sounds = 0;
};

//
// Weapons
//

#define BAD_WEAPON 0x00007FFF

//
// Converts a entvars_t * to a class pointer
// It will allocate the class and entity if necessary
//
template<class T>
T* GetClassPtr(entvars_t* pev)
{
	// allocate entity if necessary
	if (pev == nullptr)
		pev = VARS(CREATE_ENTITY());

	// get the private data
	void* privateData = GET_PRIVATE(ENT(pev));

	if (privateData != nullptr)
	{
		// return existing private data
		return dynamic_cast<T*>(static_cast<CBaseEntity*>(privateData));
	}
	else
	{
		// allocate new private data
		T* t = new (pev) T;
		t->pev = pev;
		return t;
	}
}


/*
bit_PUSHBRUSH_DATA | bit_TOGGLE_DATA
bit_MONSTER_DATA
bit_DELAY_DATA
bit_TOGGLE_DATA | bit_DELAY_DATA | bit_MONSTER_DATA
bit_PLAYER_DATA | bit_MONSTER_DATA
bit_MONSTER_DATA | CYCLER_DATA
bit_LIGHT_DATA
path_corner_data
bit_MONSTER_DATA | wildcard_data
bit_MONSTER_DATA | bit_GROUP_DATA
boid_flock_data
boid_data
CYCLER_DATA
bit_ITEM_DATA
bit_ITEM_DATA | func_hud_data
bit_TOGGLE_DATA | bit_ITEM_DATA
EOFFSET
env_sound_data
env_sound_data
push_trigger_data
*/

#define TRACER_FREQ 4  // Tracers fire every 4 bullets

typedef struct _SelAmmo
{
	BYTE Ammo1Type;
	BYTE Ammo1;
	BYTE Ammo2Type;
	BYTE Ammo2;
} SelAmmo;


// this moved here from world.cpp, to allow classes to be derived from it
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================
class CWorld : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData* pkvd);
};
