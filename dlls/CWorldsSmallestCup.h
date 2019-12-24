#pragma once

#include "vector.h"

class CTalkMonster;

class CWorldsSmallestCup : public CBaseEntity
{
public:
	virtual int ObjectCaps() { return FCAP_ACROSS_TRANSITION; }  // | FCAP_MUST_SPAWN; }

	virtual void Spawn() override;
	virtual void Precache() override;
	void EXPORT CupThink(void);

private:
	bool AmIInKleinersFace(CTalkMonster* pKleiner);
	bool IsFallingOutOfWorld();

	EHANDLE<CTalkMonster> m_hKleiner;
	float m_flKleinerFaceStart{ 0.f };

	std::unordered_set<EHANDLE<CTalkMonster>, EHANDLE<CTalkMonster>::Hash, EHANDLE<CTalkMonster>::Equal> m_hAlreadySpokenKleiners;

	static EHANDLE<CWorldsSmallestCup> m_instance;
};
