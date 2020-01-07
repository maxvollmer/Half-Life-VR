
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


VRController::~VRController()
{
	if (m_hLaserSpot)
	{
		UTIL_Remove(m_hLaserSpot);
		m_hLaserSpot = nullptr;
	}

#ifdef RENDER_DEBUG_HITBOXES
	for (auto beam : m_hDebugBBoxes)
	{
		UTIL_Remove(beam);
	}
	m_hDebugBBoxes.clear();
#endif
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

	m_isBlocked = !UTIL_CheckClearSight(pPlayer->EyePosition(), m_position, ignore_monsters, dont_ignore_glass, pPlayer->edict()) || VRPhysicsHelper::Instance().CheckIfLineIsBlocked(pPlayer->EyePosition(), m_position);

	UpdateModel(pPlayer);

	UpdateHitBoxes();

#ifdef RENDER_DEBUG_HITBOXES
	DebugDrawHitBoxes(pPlayer);
#endif

	UpdateLaserSpot();

	SendEntityDataToClient(pPlayer, id);
}

void VRController::UpdateLaserSpot()
{
	if (IsWeaponWithVRLaserSpot(m_weaponId) && CVAR_GET_FLOAT("vr_enable_aim_laser") != 0.f)
	{
		if (!m_hLaserSpot)
		{
			CLaserSpot* pLaserSpot = CLaserSpot::CreateSpot();
			pLaserSpot->Revive();
			pLaserSpot->pev->scale = 0.1f;
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
}

void VRController::SendEntityDataToClient(CBasePlayer* pPlayer, VRControllerID id)
{
	extern int gmsgVRControllerEnt;
	MESSAGE_BEGIN(MSG_ONE, gmsgVRControllerEnt, nullptr, pPlayer->pev);

	WRITE_BYTE(IsMirrored() ? 1 : 0);
	WRITE_BYTE(GetModel()->pev->body);
	WRITE_BYTE(GetModel()->pev->skin);

	WRITE_LONG(GetModel()->pev->sequence);

	WRITE_FLOAT(GetModel()->pev->frame);
	WRITE_FLOAT(GetModel()->pev->framerate);
	WRITE_FLOAT(GetModel()->pev->animtime);

	WRITE_STRING(STRING(GetModel()->pev->model));

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

	if (m_hitboxes.size() == numhitboxes && FStrEq(STRING(m_hitboxModelName), STRING(pModel->pev->model)) && gpGlobals->time < (m_hitboxLastUpdateTime + (1.f / 25.f)))
		return;

	int numattachments = GetNumAttachments(pmodel);

	m_hitboxes.clear();
	m_attachments.clear();

	m_radius = 0.f;

	std::vector<StudioHitBox> studiohitboxes;
	studiohitboxes.resize(numhitboxes);
	std::vector<StudioAttachment> studioattachments;
	studioattachments.resize(numattachments);
	if (GetHitboxesAndAttachments(pModel->pev, pmodel, pModel->pev->sequence, pModel->pev->frame, studiohitboxes.data(), studioattachments.data(), IsMirrored()))
	{
		for (const auto& studiohitbox : studiohitboxes)
		{
			float l1 = studiohitbox.mins.Length();
			float l2 = studiohitbox.maxs.Length();

			// Skip hitboxes too far away or too big
			if (l1 > 256.f || l2 > 256.f)
			{
				int fjskajfes = 0;  // to set breakpoint in debug mode
				continue;
			}

			m_hitboxes.push_back(HitBox{ studiohitbox.origin, studiohitbox.angles, studiohitbox.mins, studiohitbox.maxs });
			m_radius = max(m_radius, l1);
			m_radius = max(m_radius, l2);
		}
		for (const auto& studioattachment : studioattachments)
		{
			m_attachments.push_back(studioattachment.pos);
		}
	}
	m_hitboxFrame = pModel->pev->frame;
	m_hitboxSequence = pModel->pev->sequence;
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

bool VRController::AddDraggedEntity(EHANDLE<CBaseEntity> hEntity) const
{
	if (m_draggedEntities.count(hEntity) != 0)
		return false;

	EHANDLE<CBaseEntity> hPlayer = m_hPlayer;  // need copy due to const

	m_draggedEntities.insert(std::pair<EHANDLE<CBaseEntity>, DraggedEntityPositions>(hEntity, DraggedEntityPositions{ GetOffset(), GetPosition(), hEntity->pev->origin, hPlayer->pev->origin, GetOffset(), GetPosition(), hEntity->pev->origin, hPlayer->pev->origin }));
	return true;
}

bool VRController::RemoveDraggedEntity(EHANDLE<CBaseEntity> hEntity) const
{
	if (m_draggedEntities.count(hEntity) == 0)
		return false;

	m_draggedEntities.erase(hEntity);
	return true;
}

bool VRController::IsDraggedEntity(EHANDLE<CBaseEntity> hEntity) const
{
	return m_draggedEntities.count(hEntity) != 0;
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
	auto draggedEntityData = m_draggedEntities.find(hEntity);
	if (draggedEntityData != m_draggedEntities.end())
	{
		controllerStartOffset = draggedEntityData->second.controllerStartOffset;
		controllerStartPos = draggedEntityData->second.controllerStartPos;
		entityStartOrigin = draggedEntityData->second.entityStartOrigin;
		playerStartOrigin = draggedEntityData->second.playerStartOrigin;
		controllerLastOffset = draggedEntityData->second.controllerLastOffset;
		controllerLastPos = draggedEntityData->second.controllerLastPos;
		entityLastOrigin = draggedEntityData->second.entityLastOrigin;
		playerLastOrigin = draggedEntityData->second.playerLastOrigin;
		draggedEntityData->second.controllerLastOffset = GetOffset();
		draggedEntityData->second.controllerLastPos = GetPosition();
		draggedEntityData->second.entityLastOrigin = hEntity->pev->origin;
		draggedEntityData->second.playerLastOrigin = EHANDLE<CBaseEntity>{ m_hPlayer }
		->pev->origin;
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
	if (index >= 0 && index <= m_attachments.size())
	{
		attachment = m_attachments[index];
		return true;
	}
	return false;
}


