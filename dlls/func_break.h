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
#ifndef FUNC_BREAK_H
#define FUNC_BREAK_H

typedef enum
{
	expRandom,
	expDirected
} Explosions;
typedef enum
{
	matGlass = 0,
	matWood,
	matMetal,
	matFlesh,
	matCinderBlock,
	matCeilingTile,
	matComputer,
	matUnbreakableGlass,
	matRocks,
	matNone,
	matLastMaterial
} Materials;

#define NUM_SHARDS 6  // this many shards spawned when breakable objects break;

class CBreakable : public CBaseDelay
{
public:
	// basic functions
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData* pkvd);
	void EXPORT BreakTouch(CBaseEntity* pOther);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void DamageSound(void);

	// breakables use an overridden takedamage
	virtual int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	// To spark when hit
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);

	BOOL IsBreakable(void);
	BOOL SparkWhenHit(void);

	int DamageDecal(int bitsDamageType);

	void EXPORT Die(void);
	virtual int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	inline BOOL Explodable(void) { return ExplosionMagnitude() > 0; }
	inline int ExplosionMagnitude(void) { return pev->impulse; }
	inline void ExplosionSetMagnitude(int magnitude) { pev->impulse = magnitude; }

	static void MaterialSoundPrecache(Materials precacheMaterial);
	static void MaterialSoundRandom(edict_t* pEdict, Materials soundMaterial, float volume);
	static const char** MaterialSoundList(Materials precacheMaterial, int& soundCount);

	static const char* pSoundsWood[];
	static const char* pSoundsFlesh[];
	static const char* pSoundsGlass[];
	static const char* pSoundsMetal[];
	static const char* pSoundsConcrete[];
	static const char* pSpawnObjects[];

	static TYPEDESCRIPTION m_SaveData[];

	Materials m_Material = matGlass;
	Explosions m_Explosion = expRandom;
	int m_idShard = 0;
	float m_angle = 0.f;
	int m_iszGibModel = 0;
	int m_iszSpawnObject = 0;
	int m_iSmokeTrail = 0;

	int m_numGibBodies = 0;

private:

	void VRSpawnBreakModels(
		const Vector& pos,
		const Vector& size,
		Vector direction,
		float random,
		float life,
		int count,
		const char* model,
		int modelIndex,
		int material,
		int body, int numBodies,
		char flags);

	// returns number of gibs actually spawned
	int VRSpawnGibs(const char* model,
		const Vector& pos,
		const Vector& size,
		Vector direction,
		float random,
		float life,
		int count,
		int material,
		int body, int numBodies,
		char flags);

	void VRSpawnTempEnts(
		int modelIndex,
		const Vector& pos,
		const Vector& size,
		Vector direction,
		float random,
		float life,
		int count,
		char flags);

	int VRGetGibCount(int totalcount);

	enum class KillMethod
	{
		UNKNOWN,
		MELEE,
		PROJECTILE,
		TRIGGER,
		TOUCH
	};

	// Remember who killed us and how (for deciding how many interactive gibs to spawn)
	EHANDLE<CBaseEntity> m_hKiller;
	KillMethod m_killMethod{ KillMethod::UNKNOWN };
};

class CPushable : public CBreakable
{
public:
	void Spawn(void);
	void Precache(void);
	void Touch(CBaseEntity* pOther);
	void Move(CBaseEntity* pMover, int push);
	void KeyValue(KeyValueData* pkvd);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	virtual int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_CONTINUOUS_USE; }
	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	inline float MaxSpeed(void) { return m_maxSpeed; }

	// breakables use an overridden takedamage
	virtual int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void EmitPushSound(float length);

	static TYPEDESCRIPTION m_SaveData[];

	static char* m_soundNames[3];
	int m_lastSound = 0;  // no need to save/restore, just keeps the same sound from playing twice in a row
	float m_maxSpeed = 0.f;
	float m_soundTime = 0.f;

	int m_usehull{ 0 };
};

#endif  // FUNC_BREAK_H
