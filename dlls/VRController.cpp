
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"

#include "weapons.h"

#include "VRController.h"
#include "VRPhysicsHelper.h"

extern void* FindStudioModelByName(const char* name);
extern int ExtractBbox(void *pmodel, int sequence, float *mins, float *maxs);


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

	if (m_isValid && GetModel()->ExtractBbox(GetModel()->pev->sequence, m_mins, m_maxs))
	{
		m_isBBoxValid = ((m_maxs - m_mins).LengthSquared() > EPSILON);
	}
	else
	{
		m_isBBoxValid = false;
	}

	CBaseAnimating *pModel = GetModel();
	pModel->pev->origin = GetPosition();
	pModel->pev->angles = GetAngles();
	pModel->pev->velocity = GetVelocity();
	if (isValid)
	{
		pModel->pev->model = m_modelName;
		pModel->pev->effects &= ~EF_NODRAW;
	}
	else
	{
		pModel->pev->model = iStringNull;
		pModel->pev->effects |= EF_NODRAW;
	}
}

CBaseAnimating* VRController::GetModel()
{
	if (!m_hModel)
	{
		m_hModel = CSprite::SpriteCreate(STRING(m_modelName), GetPosition(), TRUE);
	}
	return dynamic_cast<CBaseAnimating*>((CBaseEntity*)m_hModel);
}

void VRController::PlayWeaponAnimation(int iAnim, int body)
{
	CBaseAnimating *pModel = GetModel();
	pModel->pev->sequence = iAnim;
	pModel->pev->body = body;
	pModel->pev->frame = 0;
	pModel->ResetSequenceInfo();
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
