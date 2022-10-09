
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

CVRControllerModel* CVRControllerModel::Create(const char* pModelName, const Vector& origin)
{
	CVRControllerModel* pModel = GetClassPtr<CVRControllerModel>(nullptr);
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
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 0;
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
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 0;
}

void CVRControllerModel::TurnOn()
{
	// already on!
	if (!(pev->effects & EF_NODRAW) && m_pfnThink)
		return;

	pev->effects &= ~EF_NODRAW;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 0;
	SetSequence(0);
	SetThink(&CVRControllerModel::ControllerModelThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CVRControllerModel::SetSequence(int sequence)
{
	pev->sequence = sequence;
	pev->frame = 0.f;
	ResetSequenceInfo();
}

void CVRControllerModel::ControllerModelThink()
{
	if (m_fSequenceFinished)
	{
		if (m_fSequenceLoops)
		{
			SetSequence(pev->sequence);
		}
	}
	else
	{
		float flInterval = StudioFrameAdvance();
		DispatchAnimEvents(flInterval);
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

void CVRControllerModel::HandleClientAnimEvent(ClientAnimEvent_t* pEvent)
{
	// TODO
}

void CVRControllerModel::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	// TODO
}
