
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"

#include "weapons.h"

#include "VRController.h"
#include "VRPhysicsHelper.h"
#include "VRModelHelper.h"



void VRController::Update(CBasePlayer *pPlayer, const int timestamp, const bool isValid, const Vector & offset, const Vector & angles, const Vector & velocity, bool isDragging, int weaponId)
{
	// Filter out outdated updates
	if (timestamp <= m_lastUpdateClienttime && m_lastUpdateServertime >= gpGlobals->time)
	{
		return;
	}

	// Store data
	m_isValid = isValid;
	m_offset = offset;
	m_position = pPlayer->GetClientOrigin() + offset;
	m_angles = angles;
	m_velocity = velocity;
	m_lastUpdateServertime = gpGlobals->time;
	m_lastUpdateClienttime = timestamp;
	m_isDragging = m_isValid && isDragging;
	m_weaponId = weaponId;

	m_isTeleporterBlocked =
		!m_isValid
		|| !UTIL_CheckClearSight(pPlayer->EyePosition(), m_position, ignore_monsters, dont_ignore_glass, pPlayer->edict())
		|| VRPhysicsHelper::Instance().CheckIfLineIsBlocked(pPlayer->EyePosition(), m_position);

	if (m_weaponId == WEAPON_BAREHAND)
	{
		m_modelName = MAKE_STRING("models/v_gordon_hand.mdl");
	}
	else
	{
		m_modelName = pPlayer->pev->viewmodel;
	}

	CBaseEntity *pModel = GetModel();
	pModel->pev->origin = GetPosition();
	pModel->pev->angles = GetAngles();
	pModel->pev->velocity = GetVelocity();
	UTIL_SetOrigin(pModel->pev, pModel->pev->origin);

	if (pModel->pev->model != m_modelName)
	{
		pModel->pev->model = m_modelName;
		SET_MODEL(pModel->edict(), STRING(m_modelName));
	}

	ExtractBBoxIfPossibleAndNecessary();

	if (m_isValid)
	{
		pModel->pev->effects &= ~EF_NODRAW;
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
		pModel->pev->effects |= EF_NODRAW;
		m_isBBoxValid = false;
	}
}

void VRController::ExtractBBoxIfPossibleAndNecessary()
{
	if (!IsValid())
		return;

	CBaseEntity *pModel = GetModel();
	if (m_isBBoxValid && m_bboxModelName == m_modelName && m_bboxModelSequence == pModel->pev->sequence)
		return;

	auto& modelInfo = VRModelHelper::GetModelInfo(pModel);
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
		CSprite *pModel = CSprite::SpriteCreate(m_modelName ? STRING(m_modelName) : "models/v_gordon_hand.mdl", GetPosition(), FALSE);
		pModel->m_maxFrame = 255;
		pModel->pev->framerate = 1.f;
		pModel->TurnOn();
		m_hModel = pModel;
	}
	return m_hModel;
}

void VRController::PlayWeaponAnimation(int iAnim, int body)
{
	if (!IsValid())
		return;

	CBaseEntity *pModel = GetModel();
	auto& modelInfo = VRModelHelper::GetModelInfo(pModel);

	if (modelInfo.m_isValid && iAnim < modelInfo.m_numSequences)
	{
		ALERT(at_console, "VRController::PlayWeaponAnimation: Playing sequence %i of %i for %s (%s)\n", iAnim, modelInfo.m_numSequences, STRING(m_modelName), STRING(pModel->pev->model));
		pModel->pev->sequence = iAnim;
		pModel->pev->body = body;
		pModel->pev->framerate = modelInfo.m_sequences[iAnim].framerate;
	}
	else
	{
		ALERT(at_console, "VRController::PlayWeaponAnimation: Invalid sequence %i of %i for %s (%s)\n", iAnim, modelInfo.m_numSequences, STRING(m_modelName), STRING(pModel->pev->model));
		pModel->pev->sequence = 0;
		pModel->pev->body = 0;
		pModel->pev->framerate = 1.f;
	}

	pModel->pev->frame = 0;
	pModel->pev->animtime = gpGlobals->time;
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
