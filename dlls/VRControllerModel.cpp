
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"
#include "activity.h"
#include "VRControllerModel.h"

LINK_ENTITY_TO_CLASS(vr_controllermodel, CVRControllerModel);

CVRControllerModel* CVRControllerModel::Create(const char *pModelName, const Vector &origin)
{
	CVRControllerModel* pModel = GetClassPtr((CVRControllerModel*)nullptr);
	pModel->pev->classname = MAKE_STRING("vr_controllermodel");
	pModel->pev->model = MAKE_STRING(pModelName);
	pModel->pev->origin = origin;
	pModel->Spawn();
	return pModel;
}

void CVRControllerModel::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));
	pev->scale = GetWeaponScale(STRING(pev->model));
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	TurnOff();
}

void CVRControllerModel::SetModel(string_t model)
{
	// already set!
	if (pev->model == model)
		return;

	pev->model = model;
	SET_MODEL(ENT(pev), STRING(pev->model));
	pev->scale = GetWeaponScale(STRING(pev->model));
	SetSequence(0);
}

void CVRControllerModel::TurnOff()
{
	// already off!
	if ((pev->effects & EF_NODRAW) && !m_pfnThink)
		return;

	SetSequence(0);
	SetThink(nullptr);
	pev->effects |= EF_NODRAW;
}

void CVRControllerModel::TurnOn()
{
	// already on!
	if (!(pev->effects & EF_NODRAW) && m_pfnThink)
		return;

	pev->effects &= ~EF_NODRAW;
	SetSequence(0);
	SetThink(&CVRControllerModel::Think);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CVRControllerModel::SetSequence(int sequence)
{
	pev->sequence = sequence;
	ResetSequenceInfo();
}

void CVRControllerModel::Think()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	float flInterval = StudioFrameAdvance();
	DispatchAnimEvents(flInterval);
	if (m_fSequenceFinished)
	{
		int iSequence;
		if (m_fSequenceLoops)
		{
			iSequence = LookupActivity(Activity::ACT_IDLE);
		}
		else
		{
			iSequence = LookupActivityHeaviest(Activity::ACT_IDLE);
		}
		if (iSequence != ACTIVITY_NOT_AVAILABLE)
		{
			pev->sequence = iSequence;
		}
		else
		{
			pev->sequence = 0;
		}
		ResetSequenceInfo();
	}
}

void CVRControllerModel::HandleClientAnimEvent(ClientAnimEvent_t *pEvent)
{
	// TODO
}

void CVRControllerModel::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	// TODO
}
