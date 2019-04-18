#pragma once

#include "vector.h"

class CWorldsSmallestCup : public CBaseEntity
{
public:
	static TYPEDESCRIPTION m_SaveData[];
	virtual int Save(CSave &save) override;
	virtual int Restore(CRestore &restore) override;

	virtual int ObjectCaps() { return FCAP_ACROSS_TRANSITION | FCAP_MUST_SPAWN; }

	virtual void Spawn() override;
	virtual void Precache() override;
	void EXPORT CupThink(void);

	// Must be BOOL not bool for save/restore
	BOOL		m_isBeingDragged;
};
