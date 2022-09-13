#pragma once

#include "vector.h"
#include "saverestore.h"

class CTalkMonster;
class CBasePlayer;

class CWorldsSmallestCup : public CGib
{
public:
	virtual int ObjectCaps() { return FCAP_ACROSS_TRANSITION; }  // | FCAP_MUST_SPAWN; }

	virtual void Spawn() override;
	void EXPORT CupThink(void);

	virtual int Save(CSave& save) override;
	virtual int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	virtual bool IsDraggable() override { return true; }
	virtual void HandleDragStart() override;
	virtual void HandleDragStop() override;
	virtual void HandleDragUpdate(const Vector& origin, const Vector& velocity, const Vector& angles) override;

	static void EnsureInstance(CBasePlayer* pPlayer);

private:
	bool AmIInKleinersFace(CTalkMonster* pKleiner);
	bool IsFallingOutOfWorld();

	Vector m_spawnOrigin;

	static EHANDLE<CWorldsSmallestCup> m_instance;
};
