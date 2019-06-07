
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "VRControllerModel.h"
#include "VRController.h"
#include "VRPhysicsHelper.h"
#include "VRModelHelper.h"



void VRController::Update(CBasePlayer *pPlayer, const int timestamp, const bool isValid, const bool isMirrored, const Vector & offset, const Vector & angles, const Vector & velocity, bool isDragging, VRControllerID id, int weaponId)
{
	// Filter out outdated updates
	if (timestamp <= m_lastUpdateClienttime && m_lastUpdateServertime >= gpGlobals->time)
	{
		return;
	}

	// Store data
	m_id = id;
	m_isValid = isValid;
	m_offset = offset;
	m_position = pPlayer->GetClientOrigin() + offset;
	m_angles = angles;
	m_velocity = velocity;
	m_lastUpdateServertime = gpGlobals->time;
	m_lastUpdateClienttime = timestamp;
	m_isDragging = m_isValid && isDragging;
	m_isMirrored = isMirrored;
	m_weaponId = weaponId;

	UpdateModel(pPlayer);

	ExtractBBoxIfPossibleAndNecessary();

	UpdateLaserSpot();

	SendEntityDataToClient(pPlayer, id);
}

void VRController::UpdateLaserSpot()
{
	if (IsWeaponWithVRLaserSpot(m_weaponId) && CVAR_GET_FLOAT("vr_enable_aim_laser") != 0.f)
	{
		if (!m_hLaserSpot)
		{
			CLaserSpot *pLaserSpot = CLaserSpot::CreateSpot();
			pLaserSpot->Revive();
			m_hLaserSpot = pLaserSpot;
		}

		TraceResult tr;
		UTIL_TraceLine(GetPosition(), GetPosition() + (GetAim() * 8192.f), dont_ignore_monsters, GetModel()->edict(), &tr);
		UTIL_SetOrigin(m_hLaserSpot->pev, tr.vecEndPos);
	}
	else
	{
		if (m_hLaserSpot)
		{
			UTIL_Remove(m_hLaserSpot);
			m_hLaserSpot = nullptr;
		}
	}
}

void VRController::UpdateModel(CBasePlayer* pPlayer)
{
	if (m_weaponId == WEAPON_BAREHAND)
	{
		m_modelName = MAKE_STRING("models/v_gordon_hand.mdl");
	}
	else
	{
		m_modelName = pPlayer->pev->viewmodel;
	}

	CVRControllerModel *pModel = (CVRControllerModel*)GetModel();
	pModel->pev->origin = GetPosition();
	pModel->pev->angles = GetAngles();
	pModel->pev->velocity = GetVelocity();
	UTIL_SetOrigin(pModel->pev, pModel->pev->origin);

	if (pModel->pev->model != m_modelName)
	{
		pModel->SetModel(m_modelName);
	}

	if (m_isValid)
	{
		pModel->TurnOn();
		if (m_isBBoxValid)
		{
			pModel->pev->mins = m_mins;
			pModel->pev->maxs = m_maxs;
			pModel->pev->size = m_maxs - m_mins;
			pModel->pev->absmin = pModel->pev->origin + m_mins;
			pModel->pev->absmax = pModel->pev->origin + m_maxs;
		}
	}
	else
	{
		pModel->TurnOff();
		m_isBBoxValid = false;
	}

	if (m_isDragging && m_weaponId == WEAPON_BAREHAND && pModel->pev->sequence != 1)
	{
		PlayWeaponAnimation(1, 0);
	}
}

void VRController::SendEntityDataToClient(CBasePlayer *pPlayer, VRControllerID id)
{
	extern int gmsgVRControllerEnt;
	MESSAGE_BEGIN(MSG_ALL, gmsgVRControllerEnt, NULL);
	WRITE_ENTITY(ENTINDEX(GetModel()->edict()));
	WRITE_BYTE((id == VRControllerID::WEAPON) ? 1 : 0);
	WRITE_PRECISE_VECTOR(GetModel()->pev->origin);
	WRITE_PRECISE_VECTOR(GetModel()->pev->angles);
	WRITE_BYTE(IsMirrored() ? 1 : 0);
	WRITE_BYTE(IsValid() ? 1 : 0);
	MESSAGE_END();
}

void VRController::ExtractBBoxIfPossibleAndNecessary()
{
	if (!IsValid())
		return;

	CBaseEntity *pModel = GetModel();
	if (m_isBBoxValid && m_bboxModelName == m_modelName && m_bboxModelSequence == pModel->pev->sequence)
		return;

	auto& modelInfo = VRModelHelper::GetInstance().GetModelInfo(pModel);
	if (modelInfo.m_isValid && pModel->pev->sequence < modelInfo.m_numSequences)
	{
		m_bboxModelName = m_modelName;
		m_bboxModelSequence = pModel->pev->sequence;
		m_mins = modelInfo.m_sequences[m_bboxModelSequence].bboxMins;
		m_maxs = modelInfo.m_sequences[m_bboxModelSequence].bboxMaxs;
		m_radius = modelInfo.m_sequences[m_bboxModelSequence].bboxRadius;
		m_radiusSquared = m_radius * m_radius;
		m_isBBoxValid = m_radius > EPSILON;
	}
	else
	{
		m_isBBoxValid = false;
	}
}

CBaseEntity* VRController::GetModel() const
{
	if (!m_hModel)
	{
		CVRControllerModel *pModel = CVRControllerModel::Create(m_modelName ? STRING(m_modelName) : "models/v_gordon_hand.mdl", GetPosition());
		if (IsValid())
		{
			pModel->TurnOn();
		}
		m_hModel = pModel;
	}
	return m_hModel;
}

void VRController::PlayWeaponAnimation(int iAnim, int body)
{
	if (!IsValid())
		return;

	CVRControllerModel *pModel = (CVRControllerModel*)GetModel();
	auto& modelInfo = VRModelHelper::GetInstance().GetModelInfo(pModel);
	if (modelInfo.m_isValid && iAnim < modelInfo.m_numSequences)
	{
		// ALERT(at_console, "VRController::PlayWeaponAnimation: Playing sequence %i of %i with framerate %.0f for %s (%s)\n", iAnim, modelInfo.m_numSequences, modelInfo.m_sequences[iAnim].framerate, STRING(m_modelName), STRING(pModel->pev->model));
		pModel->SetSequence(iAnim);
	}
	else
	{
		// ALERT(at_console, "VRController::PlayWeaponAnimation: Invalid sequence %i of %i for %s (%s)\n", iAnim, modelInfo.m_numSequences, STRING(m_modelName), STRING(pModel->pev->model));
		pModel->SetSequence(0);
	}
}

void VRController::PlayWeaponMuzzleflash()
{
	if (!IsValid())
		return;

	CBaseEntity *pModel = GetModel();
	pModel->pev->effects |= EF_MUZZLEFLASH;
}

bool VRController::AddTouchedEntity(EHANDLE hEntity) const
{
	if (m_touchedEntities.count(hEntity) != 0)
		return false;

	m_touchedEntities.insert(hEntity);
	return true;
}

bool VRController::RemoveTouchedEntity(EHANDLE hEntity) const
{
	if (m_touchedEntities.count(hEntity) == 0)
		return false;

	m_touchedEntities.erase(hEntity);
	return true;
}

bool VRController::AddDraggedEntity(EHANDLE hEntity) const
{
	if (m_draggedEntities.count(hEntity) != 0)
		return false;

	m_draggedEntities.insert(hEntity);
	return true;
}

bool VRController::RemoveDraggedEntity(EHANDLE hEntity) const
{
	if (m_draggedEntities.count(hEntity) == 0)
		return false;

	m_draggedEntities.erase(hEntity);
	return true;
}

bool VRController::IsDraggedEntity(EHANDLE hEntity) const
{
	return m_draggedEntities.count(hEntity) != 0;
}

bool VRController::AddHitEntity(EHANDLE hEntity) const
{
	if (m_hitEntities.count(hEntity) != 0)
		return false;

	m_hitEntities.insert(hEntity);
	return true;
}

bool VRController::RemoveHitEntity(EHANDLE hEntity) const
{
	if (m_hitEntities.count(hEntity) == 0)
		return false;

	m_hitEntities.erase(hEntity);
	return true;
}

const Vector VRController::GetGunPosition() const
{
	Vector pos;
	if (VRModelHelper::GetInstance().GetAttachment(GetModel(), VR_MUZZLE_ATTACHMENT, pos))
	{
		return pos;
	}
	else
	{
		return GetPosition();
	}
}

const Vector VRController::GetAim() const
{
	Vector pos1;
	Vector pos2;
	bool result1 = VRModelHelper::GetInstance().GetAttachment(GetModel(), VR_MUZZLE_ATTACHMENT, pos1);
	bool result2 = VRModelHelper::GetInstance().GetAttachment(GetModel(), VR_MUZZLE_ATTACHMENT + 1, pos2);
	if (result1 && result2 && pos2 != pos1)
	{
		return (pos2 - pos1).Normalize();
	}
	UTIL_MakeAimVectors(GetAngles());
	return gpGlobals->v_forward;
}
