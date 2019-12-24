#pragma once

#include "vector.h"

class CVRControllerModel : public CBaseAnimating
{
public:
	// Don't save/restore
	virtual int Save(CSave& save) override { return 0; }
	virtual int Restore(CRestore& restore) override { return 0; }

	virtual int ObjectCaps() { return FCAP_DONT_SAVE; }

	void Spawn() override;
	void EXPORT ControllerModelThink(void);
	void HandleClientAnimEvent(ClientAnimEvent_t* pEvent) override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void SetSequence(int sequence);
	void TurnOff();
	void TurnOn();
	void SetModel(string_t model);
	static CVRControllerModel* Create(const char* pModelName, const Vector& origin);
};
