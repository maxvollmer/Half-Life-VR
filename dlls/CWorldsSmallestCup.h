#pragma once

#include "vector.h"

class CWorldsSmallestCup : public CBaseEntity
{
public:
	virtual int ObjectCaps() { return FCAP_ACROSS_TRANSITION; }// | FCAP_MUST_SPAWN; }

	virtual void Spawn() override;
	virtual void Precache() override;
	void EXPORT CupThink(void);

	std::unordered_set<VRControllerID>		m_isBeingDragged;

private:
	bool AmIInKleinersFace(CBaseEntity* pKleiner);

	EHANDLE							m_hKleiner;
	float							m_flKleinerFaceStart{ 0.f };

	std::unordered_set<EHANDLE, EHANDLE::Hash, EHANDLE::Equal>		m_hAlreadySpokenKleiners;

	static EHANDLE					m_instance;
};
