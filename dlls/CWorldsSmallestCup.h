#pragma once

#include "vector.h"

class CWorldsSmallestCup : public CBaseEntity
{
public:
	virtual int ObjectCaps() { return FCAP_ACROSS_TRANSITION; }  // | FCAP_MUST_SPAWN; }

	virtual void Spawn() override;
	virtual void Precache() override;
	void EXPORT CupThink(void);

private:
	bool AmIInKleinersFace(CBaseEntity* pKleiner);
	bool IsFallingOutOfWorld();

	EHANDLE<CBaseEntity> m_hKleiner;
	float m_flKleinerFaceStart{ 0.f };

	std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_hAlreadySpokenKleiners;

	static EHANDLE<CBaseEntity> m_instance;
};
