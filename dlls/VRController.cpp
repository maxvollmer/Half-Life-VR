
#include <algorithm>

#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"
#include "VRControllerModel.h"
#include "VRController.h"
#include "VRPhysicsHelper.h"
#include "VRModelHelper.h"

#ifdef RENDER_DEBUG_BBOXES
#include "VRDebugBBoxDrawer.h"
static VRDebugBBoxDrawer g_VRDebugBBoxDrawer;
#endif

namespace
{
	enum HandModelSequences
	{
		IDLE = 0,
		POINT_START,
		POINT_END,
		WAIT_START,
		WAIT_END,
		HALFGRAB_START,
		HALFGRAB_END,
		FULLGRAB_START,
		FULLGRAB_END
	};

	std::unordered_map<std::string, const char*> g_animatedWeaponModelNameMap = {
		{"models/v_357.mdl", "models/animov/v_357.mdl"},
		{"models/v_9mmar.mdl", "models/animov/v_9mmar.mdl"},
		{"models/v_9mmhandgun.mdl", "models/animov/v_9mmhandgun.mdl"},
		{"models/v_crossbow.mdl", "models/animov/v_crossbow.mdl"},
		{"models/v_crowbar.mdl", "models/animov/v_crowbar.mdl"},
		{"models/v_egon.mdl", "models/animov/v_egon.mdl"},
		{"models/v_gauss.mdl", "models/animov/v_gauss.mdl"},
		{"models/v_grenade.mdl", "models/animov/v_grenade.mdl"},
		{"models/v_hgun.mdl", "models/animov/v_hgun.mdl"},
		{"models/v_rpg.mdl", "models/animov/v_rpg.mdl"},
		{"models/v_satchel.mdl", "models/animov/v_satchel.mdl"},
		{"models/v_satchel_radio.mdl", "models/animov/v_satchel_radio.mdl"},
		{"models/v_shotgun.mdl", "models/animov/v_shotgun.mdl"},
		{"models/v_squeak.mdl", "models/animov/v_squeak.mdl"},
		{"models/v_tripmine.mdl", "models/animov/v_tripmine.mdl"},

		{"models/SD/v_357.mdl", "models/SD/animov/v_357.mdl"},
		{"models/SD/v_9mmar.mdl", "models/SD/animov/v_9mmar.mdl"},
		{"models/SD/v_9mmhandgun.mdl", "models/SD/animov/v_9mmhandgun.mdl"},
		{"models/SD/v_crossbow.mdl", "models/SD/animov/v_crossbow.mdl"},
		{"models/SD/v_crowbar.mdl", "models/SD/animov/v_crowbar.mdl"},
		{"models/SD/v_egon.mdl", "models/SD/animov/v_egon.mdl"},
		{"models/SD/v_gauss.mdl", "models/SD/animov/v_gauss.mdl"},
		{"models/SD/v_grenade.mdl", "models/SD/animov/v_grenade.mdl"},
		{"models/SD/v_hgun.mdl", "models/SD/animov/v_hgun.mdl"},
		{"models/SD/v_rpg.mdl", "models/SD/animov/v_rpg.mdl"},
		{"models/SD/v_satchel.mdl", "models/SD/animov/v_satchel.mdl"},
		{"models/SD/v_satchel_radio.mdl", "models/SD/animov/v_satchel_radio.mdl"},
		{"models/SD/v_shotgun.mdl", "models/SD/animov/v_shotgun.mdl"},
		{"models/SD/v_squeak.mdl", "models/SD/animov/v_squeak.mdl"},
		{"models/SD/v_tripmine.mdl", "models/SD/animov/v_tripmine.mdl"},
	};

	string_t GetAnimatedWeaponModelName(string_t modelName)
	{
		auto it = g_animatedWeaponModelNameMap.find(STRING(modelName));
		if (it != g_animatedWeaponModelNameMap.end())
			return MAKE_STRING(it->second);
		else
			return modelName;
	}
}  // namespace

const char* GetAnimatedWeaponModelName(const char* modelName)
{
	auto it = g_animatedWeaponModelNameMap.find(modelName);
	if (it != g_animatedWeaponModelNameMap.end())
		return it->second;
	else
		return modelName;
}


void VRController::Update(CBasePlayer* pPlayer, const int timestamp, const bool isValid, const bool isMirrored, const Vector& offset, const Vector& angles, const Vector& velocity, bool isDragging, VRControllerID id, int weaponId)
{
	m_hPlayer = pPlayer;

	// Filter out outdated updates
	if (timestamp <= m_lastUpdateClienttime && m_lastUpdateServertime >= gpGlobals->time)
	{
		return;
	}

	if (m_isValid)
	{
		m_previousAngles = m_angles;
	}
	else
	{
		m_previousAngles = angles;
	}

	bool m_wasDragging = m_isDragging;

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

	if (!m_wasDragging && m_isDragging)
	{
		m_dragStartTime = gpGlobals->time;
	}
	else if (!m_isDragging)
	{
		m_dragStartTime = 0.f;
	}

	m_isBlocked = !UTIL_CheckClearSight(pPlayer->EyePosition(), m_position, ignore_monsters, dont_ignore_glass, pPlayer->edict()) || VRPhysicsHelper::Instance().CheckIfLineIsBlocked(pPlayer->EyePosition(), m_position);

	UpdateModel(pPlayer);

	ClearOutDeadEntities();

#ifdef RENDER_DEBUG_BBOXES
	if (m_isValid)
	{
		if (m_isMirrored) g_VRDebugBBoxDrawer.SetColor(255, 0, 0);
		else g_VRDebugBBoxDrawer.SetColor(0, 255, 0);

		g_VRDebugBBoxDrawer.ClearAllBut(GetModel());
		g_VRDebugBBoxDrawer.DrawBBoxes(GetModel(), IsMirrored());
	}
	else
	{
		g_VRDebugBBoxDrawer.ClearAll();
	}
#endif
}

float GetVibrateIntensity(VRController::TouchType touch)
{
	switch (touch)
	{
	case VRController::TouchType::LIGHT_TOUCH:
		return 0.1f;
	case VRController::TouchType::NORMAL_TOUCH:
		return 0.5f;
	case VRController::TouchType::HARD_TOUCH:
		return 1.f;
	default:
		return 0.f;
	}
}

void VRController::PostFrame()
{
	float vibrateintensity = 0.f;
	for (auto it = m_touches.begin(); it != m_touches.end();)
	{
		vibrateintensity = (std::max)(vibrateintensity, GetVibrateIntensity(it->first));
		if (it->second <= gpGlobals->time)
		{
			it = m_touches.erase(it);
		}
		else
		{
			it++;
		}
	}

	if (vibrateintensity > 0.f)
	{
		// Send vibrate message to client
		extern int gmsgVRTouch;
		MESSAGE_BEGIN(MSG_ONE, gmsgVRTouch, nullptr, m_hPlayer->pev);
		WRITE_BYTE(IsMirrored() ? 1 : 0);
		WRITE_FLOAT(vibrateintensity);
		MESSAGE_END();
	}

	SendEntityDataToClient(m_hPlayer, m_id);
}

void VRController::AddTouch(TouchType touch, float duration)
{
	float touchuntil = gpGlobals->time + duration;
	m_touches[touch] = (std::max)(m_touches[touch], touchuntil);
}

void VRController::ClearOutDeadEntities()
{
	for (auto& it = m_touchedEntities.begin(); it != m_touchedEntities.end();)
		if (!it->Get()) it = m_touchedEntities.erase(it);
		else it++;

	for (auto& it = m_hitEntities.begin(); it != m_hitEntities.end();)
		if (!it->Get()) it = m_hitEntities.erase(it);
		else it++;

	if (!m_draggedEntity)
	{
		m_draggedEntity = nullptr;
		m_draggedEntityPositions.reset();
	}
}

void VRController::UpdateModel(CBasePlayer* pPlayer)
{
	if (m_weaponId == WEAPON_BAREHAND)
	{
		if (pPlayer->HasSuit())
		{
			m_modelName = MAKE_STRING("models/v_hand_hevsuit.mdl");
		}
		else
		{
			m_modelName = MAKE_STRING("models/v_hand_labcoat.mdl");
		}
	}
	else
	{
		m_modelName = pPlayer->pev->viewmodel;
	}

	if (CVAR_GET_FLOAT("vr_use_animated_weapons") != 0.f)
	{
		m_modelName = GetAnimatedWeaponModelName(m_modelName);
	}

	CVRControllerModel* pModel = GetModel();
	pModel->pev->origin = GetPosition();
	pModel->pev->angles = GetAngles();
	pModel->pev->velocity = GetVelocity();
	UTIL_SetOrigin(pModel->pev, pModel->pev->origin);

	if (!FStrEq(STRING(pModel->pev->model), STRING(m_modelName)))
	{
		pModel->SetModel(m_modelName);
		PlayWeaponAnimation(0, 0);
	}

	if (m_weaponId == WEAPON_BAREHAND && m_hasFlashlight)
	{
		if (pPlayer->FlashlightIsOn())
		{
			pModel->pev->body = 3;
		}
		else
		{
			pModel->pev->body = 1;
		}
	}
	else
	{
		pModel->pev->body = 0;
	}

	if (m_weaponId == WEAPON_BAREHAND)
	{
		if (m_isDragging && pModel->pev->sequence != FULLGRAB_START)
		{
			PlayWeaponAnimation(FULLGRAB_START, 0);
		}
		else if (!m_isDragging && pModel->pev->sequence == FULLGRAB_START)
		{
			PlayWeaponAnimation(FULLGRAB_END, 0);
		}
	}

	UpdateHitBoxes();

	if (m_isValid)
	{
		pModel->TurnOn();
		pModel->pev->mins = Vector{ -m_radius, -m_radius, -m_radius };
		pModel->pev->maxs = Vector{ m_radius, m_radius, m_radius };
		pModel->pev->size = pModel->pev->maxs - pModel->pev->mins;
		pModel->pev->absmin = pModel->pev->origin + pModel->pev->mins;
		pModel->pev->absmax = pModel->pev->origin + pModel->pev->maxs;
	}
	else
	{
		pModel->TurnOff();
	}
}

void VRController::SendEntityDataToClient(CBasePlayer* pPlayer, VRControllerID id)
{
	extern int gmsgVRControllerEnt;
	MESSAGE_BEGIN(MSG_ONE, gmsgVRControllerEnt, nullptr, pPlayer->pev);

	WRITE_BYTE(IsMirrored() ? 1 : 0);
	WRITE_BYTE(GetModel()->pev->body);
	WRITE_BYTE(GetModel()->pev->skin);
	WRITE_FLOAT(GetModel()->pev->scale);

	WRITE_LONG(GetModel()->pev->sequence);
	WRITE_FLOAT(GetModel()->pev->frame);
	WRITE_FLOAT(GetModel()->pev->framerate);
	WRITE_FLOAT(GetModel()->pev->animtime);

	WRITE_STRING(STRING(GetModel()->pev->model));

	if (IsDragging() && m_draggedEntity && m_draggedEntityPositions)
	{
		// Send dragged entity offsets to client
		WRITE_BYTE(1);

		WRITE_ENTITY(ENTINDEX(m_draggedEntity->edict()));

		WRITE_PRECISE_VECTOR(m_draggedEntity->m_vrDragOriginOffset);
		WRITE_PRECISE_VECTOR(m_draggedEntity->m_vrDragAnglesOffset);

		WRITE_BYTE(m_draggedEntity->pev->body);
		WRITE_BYTE(m_draggedEntity->pev->skin);
		WRITE_FLOAT(m_draggedEntity->pev->scale);

		WRITE_LONG(m_draggedEntity->pev->sequence);
		WRITE_FLOAT(m_draggedEntity->pev->frame);
		WRITE_FLOAT(m_draggedEntity->pev->framerate);
		WRITE_FLOAT(m_draggedEntity->pev->animtime);

		WRITE_LONG(m_draggedEntity->pev->effects);

		WRITE_BYTE(m_draggedEntity->pev->rendermode);
		WRITE_BYTE(m_draggedEntity->pev->renderamt);
		WRITE_BYTE(m_draggedEntity->pev->renderfx);
		WRITE_BYTE(static_cast<unsigned int>(m_draggedEntity->pev->rendercolor.x));
		WRITE_BYTE(static_cast<unsigned int>(m_draggedEntity->pev->rendercolor.y));
		WRITE_BYTE(static_cast<unsigned int>(m_draggedEntity->pev->rendercolor.z));

		WRITE_STRING(STRING(m_draggedEntity->pev->model));
	}
	else
	{
		// no dragged entity
		WRITE_BYTE(0);
	}

	MESSAGE_END();
}

void VRController::UpdateHitBoxes()
{
	if (!IsValid())
	{
		ClearHitBoxes();
		return;
	}

	CBaseEntity* pModel = GetModel();
	void* pmodel = GET_MODEL_PTR(pModel->edict());
	if (!pmodel)
	{
		ClearHitBoxes();
		return;
	}

	int numhitboxes = GetNumHitboxes(pmodel);
	if (numhitboxes <= 0)
	{
		ClearHitBoxes();
		return;
	}

	// for more accurate weapon handling we should update every frame
#if 0
	if (m_hitboxes.size() == numhitboxes
		&& FStrEq(STRING(m_hitboxModelName), STRING(pModel->pev->model))
		&& m_hitboxFrame == pModel->pev->frame
		&& m_hitboxSequence == pModel->pev->sequence
		&& m_hitboxMirrored == m_isMirrored
		&& std::fabs(m_hitboxLastUpdateTime - gpGlobals->time) < 0.1f)
		return;
#endif

	int numattachments = GetNumAttachments(pmodel);

	m_hitboxes.clear();
	m_attachments.clear();

	m_radius = 0.f;

	float radiusSquared = 0.f;
	std::vector<StudioHitBox> studiohitboxes;
	studiohitboxes.resize(numhitboxes);
	std::vector<StudioAttachment> studioattachments;
	studioattachments.resize(numattachments);
	if (GetHitboxesAndAttachments(pModel->pev, pmodel, pModel->pev->sequence, pModel->pev->frame, studiohitboxes.data(), studioattachments.data(), IsMirrored()))
	{
		for (const auto& studiohitbox : studiohitboxes)
		{
			float distance = (studiohitbox.abscenter - GetPosition()).Length();
			float radius = (studiohitbox.maxs - studiohitbox.mins).Length();

			// Skip hitboxes too far away
			if (distance > 512.f)
			{
				continue;
			}

			m_hitboxes.push_back(studiohitbox);

			Vector localabsmin = studiohitbox.origin + studiohitbox.mins - GetPosition();
			Vector localabsmax = studiohitbox.origin + studiohitbox.maxs - GetPosition();

			radiusSquared = (std::max)(radiusSquared, localabsmin.LengthSquared());
			radiusSquared = (std::max)(radiusSquared, localabsmax.LengthSquared());
		}
		for (const auto& studioattachment : studioattachments)
		{
			m_attachments.push_back(studioattachment.pos);
		}
	}
	m_radius = std::sqrtf(radiusSquared);
	m_hitboxModelName = pModel->pev->model;
	m_hitboxFrame = pModel->pev->frame;
	m_hitboxSequence = pModel->pev->sequence;
	m_hitboxMirrored = m_isMirrored;
	m_hitboxLastUpdateTime = gpGlobals->time;
}

CVRControllerModel* VRController::GetModel() const
{
	if (!m_hModel)
	{
		CVRControllerModel* pModel = CVRControllerModel::Create(m_modelName ? STRING(m_modelName) : "models/v_hand_labcoat.mdl", GetPosition());
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

	CVRControllerModel* pModel = GetModel();
	auto& modelInfo = VRModelHelper::GetInstance().GetModelInfo(pModel);
	if (modelInfo.m_isValid && iAnim < modelInfo.m_numSequences)
	{
		//ALERT(at_console, "VRController::PlayWeaponAnimation: Playing sequence %i of %i with framerate %.0f for %s\n", iAnim, modelInfo.m_numSequences, modelInfo.m_sequences[iAnim].framerate, STRING(m_modelName));
		pModel->SetSequence(iAnim);
	}
	else
	{
		ALERT(at_console, "VRController::PlayWeaponAnimation: Invalid sequence %i of %i for %s\n", iAnim, modelInfo.m_numSequences, STRING(m_modelName));
		pModel->SetSequence(0);
	}
}

void VRController::PlayWeaponMuzzleflash()
{
	if (!IsValid())
		return;

	CVRControllerModel* pModel = GetModel();
	pModel->pev->effects |= EF_MUZZLEFLASH;
}

bool VRController::AddTouchedEntity(EHANDLE<CBaseEntity> hEntity) const
{
	if (m_touchedEntities.count(hEntity) != 0)
		return false;

	m_touchedEntities.insert(hEntity);
	return true;
}

bool VRController::RemoveTouchedEntity(EHANDLE<CBaseEntity> hEntity) const
{
	if (m_touchedEntities.count(hEntity) == 0)
		return false;

	m_touchedEntities.erase(hEntity);
	return true;
}

bool VRController::SetDraggedEntity(EHANDLE<CBaseEntity> hEntity) const
{
	if (m_draggedEntity && m_draggedEntity != hEntity)
		return false;

	if (m_draggedEntity == hEntity)
		return true;

	m_draggedEntity = hEntity;
	m_draggedEntityPositions.reset(new DraggedEntityPositions{ GetOffset(), GetPosition(), hEntity->pev->origin, m_hPlayer->pev->origin, GetOffset(), GetPosition(), hEntity->pev->origin, m_hPlayer->pev->origin });

	return true;
}

void VRController::ClearDraggedEntity() const
{
	m_draggedEntity = nullptr;
	m_draggedEntityPositions.reset();
}

bool VRController::RemoveDraggedEntity(EHANDLE<CBaseEntity> hEntity) const
{
	if (m_draggedEntity != hEntity)
		return false;

	ClearDraggedEntity();

	return true;
}

bool VRController::IsDraggedEntity(EHANDLE<CBaseEntity> hEntity) const
{
	return m_draggedEntity == hEntity;
}

bool VRController::HasDraggedEntity() const
{
	return m_draggedEntity;
}

bool VRController::GetDraggedEntityPositions(EHANDLE<CBaseEntity> hEntity,
	Vector& controllerStartOffset,
	Vector& controllerStartPos,
	Vector& entityStartOrigin,
	Vector& playerStartOrigin,
	Vector& controllerLastOffset,
	Vector& controllerLastPos,
	Vector& entityLastOrigin,
	Vector& playerLastOrigin) const
{
	if (m_draggedEntity == hEntity && m_draggedEntityPositions)
	{
		controllerStartOffset = m_draggedEntityPositions->controllerStartOffset;
		controllerStartPos = m_draggedEntityPositions->controllerStartPos;
		entityStartOrigin = m_draggedEntityPositions->entityStartOrigin;
		playerStartOrigin = m_draggedEntityPositions->playerStartOrigin;
		controllerLastOffset = m_draggedEntityPositions->controllerLastOffset;
		controllerLastPos = m_draggedEntityPositions->controllerLastPos;
		entityLastOrigin = m_draggedEntityPositions->entityLastOrigin;
		playerLastOrigin = m_draggedEntityPositions->playerLastOrigin;
		m_draggedEntityPositions->controllerLastOffset = GetOffset();
		m_draggedEntityPositions->controllerLastPos = GetPosition();
		m_draggedEntityPositions->entityLastOrigin = hEntity->pev->origin;
		m_draggedEntityPositions->playerLastOrigin = EHANDLE<CBaseEntity>{ m_hPlayer }->pev->origin;
		return true;
	}
	return false;
}

bool VRController::AddHitEntity(EHANDLE<CBaseEntity> hEntity) const
{
	if (m_hitEntities.count(hEntity) != 0)
		return false;

	m_hitEntities.insert(hEntity);
	return true;
}

bool VRController::RemoveHitEntity(EHANDLE<CBaseEntity> hEntity) const
{
	if (m_hitEntities.count(hEntity) == 0)
		return false;

	m_hitEntities.erase(hEntity);
	return true;
}

const Vector VRController::GetGunPosition() const
{
	Vector pos;
	bool result = GetAttachment(VR_MUZZLE_ATTACHMENT, pos);
	if (result)
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
	bool result1 = GetAttachment(VR_MUZZLE_ATTACHMENT, pos1);
	bool result2 = GetAttachment(VR_MUZZLE_ATTACHMENT + 1, pos2);
	if (result1 && result2 && pos2 != pos1)
	{
		return (pos2 - pos1).Normalize();
	}
	UTIL_MakeAimVectors(GetAngles());
	return gpGlobals->v_forward;
}

bool VRController::GetAttachment(size_t index, Vector& attachment) const
{
	if (index >= 0 && index < m_attachments.size())
	{
		attachment = m_attachments[index];
		return true;
	}
	return false;
}

bool VRController::IsDragging() const
{
	if (CVAR_GET_FLOAT("vr_drag_onlyhand") != 0.f && GetWeaponId() != WEAPON_BAREHAND)
	{
		return false;
	}
	return m_isDragging;
}

bool VRController::HasAnyEntites() const
{
	return m_draggedEntity || !m_touchedEntities.empty() || !m_hitEntities.empty();
}
