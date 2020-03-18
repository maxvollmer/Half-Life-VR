
#include <algorithm>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "pm_defs.h"
#include "com_model.h"

#include "VRGroundEntityHandler.h"
#include "VRPhysicsHelper.h"

namespace
{
	const std::unordered_set<std::string> ValidGroundEntityClassnames =
	{
		"func_train",
		"func_tracktrain",
		"func_plat",
		"func_platrot",
		"func_door_rotating",
		//"func_rotating"
	};
	const std::unordered_set<std::string> IntroTrainRideMapNames =
	{
		"maps/c0a0.bsp",
		"maps/c0a0a.bsp",
		"maps/c0a0b.bsp",
		"maps/c0a0c.bsp",
		"maps/c0a0d.bsp" };
}  // namespace

void VRGroundEntityHandler::HandleMovingWithSolidGroundEntities()
{
	if (!m_pPlayer)
		return;

	DetectAndSetGroundEntity();
	SendGroundEntityToClient();
	MoveWithGroundEntity();
}

void VRGroundEntityHandler::DetectAndSetGroundEntity()
{
	extern bool VRGlobalGetNoclipMode();
	if (VRGlobalGetNoclipMode())
	{
		m_pPlayer->pev->groundentity = nullptr;
		m_hGroundEntity = nullptr;
		return;
	}

	edict_t* pentground = nullptr;

	// Take train if conditions are met
	bool forceIntroTrainRide = CVAR_GET_FLOAT("vr_force_introtrainride") != 0.f;
	std::string mapName{ STRING(INDEXENT(0)->v.model) };
	if (forceIntroTrainRide && IntroTrainRideMapNames.count(mapName) != 0)
	{
		pentground = FIND_ENTITY_BY_STRING(nullptr, "targetname", "train");
		if (!FNullEnt(pentground))
		{
			// if we fell out, teleport us back in
			if (!UTIL_PointInsideBBox(m_pPlayer->pev->origin, pentground->v.absmin, pentground->v.absmax) || m_pPlayer->pev->origin.z < (pentground->v.origin.z + pentground->v.mins.z))
			{
				m_pPlayer->pev->origin = pentground->v.origin;
				m_pPlayer->pev->origin.z += 64.f;
				m_pPlayer->pev->velocity = Vector{};
			}
		}
	}

	if (!pentground)
	{
		// If engine has found a groundentity, take it
		if (!FNullEnt(m_pPlayer->pev->groundentity))
		{
			pentground = m_pPlayer->pev->groundentity;
		}

		// Try to find a ground entity using PM code
		if (!pentground)
		{
			extern playermove_t* pmove;
			if (pmove && pmove->numphysent > 1 && pmove->physents[0].model->needload == 0)
			{
				int entindex = pmove->PM_TestPlayerPosition(m_pPlayer->pev->origin, nullptr);
				if (entindex > 0)
				{
					edict_t *pentground = INDEXENT(pmove->physents[entindex].info);
					if (IsExcludedAsGroundEntity(pentground))
					{
						pentground = nullptr;
					}
				}
			}
		}

		// Use physics engine to find potentially better ground entity
		for (int index = 1; index < gpGlobals->maxEntities; index++)
		{
			edict_t* pent = INDEXENT(index);
			if (FNullEnt(pent))
				continue;

			if (CheckIfPotentialGroundEntityForPlayer(pent) && !IsExcludedAsGroundEntity(pent))
			{
				pentground = ChoseBetterGroundEntityForPlayer(pentground, pent);
			}
		}
	}

	if (pentground && m_hGroundEntity.Get() != pentground)
	{
		m_lastGroundEntityOrigin = pentground->v.origin;
		m_lastGroundEntityAngles = pentground->v.angles;
	}

	if (pentground)
	{
		m_pPlayer->pev->groundentity = pentground;
	}
	else
	{
		if (m_pPlayer->pev->groundentity == m_hGroundEntity.Get())
		{
			m_pPlayer->pev->groundentity = nullptr;
		}
	}

	m_hGroundEntity = CBaseEntity::SafeInstance<CBaseEntity>(pentground);
}

edict_t* VRGroundEntityHandler::ChoseBetterGroundEntityForPlayer(edict_t* pent1, edict_t* pent2)
{
	if (!pent1)
		return pent2;

	if (!pent2)
		return pent1;

	// Check which one moves faster
	if (pent1->v.velocity.LengthSquared() > pent2->v.velocity.LengthSquared())
		return pent1;

	if (pent2->v.velocity.LengthSquared() > pent1->v.velocity.LengthSquared())
		return pent2;

	// Check which one rotates faster
	if (pent1->v.avelocity.LengthSquared() > pent2->v.avelocity.LengthSquared())
		return pent1;

	if (pent2->v.avelocity.LengthSquared() > pent1->v.avelocity.LengthSquared())
		return pent2;

	// Both move and rotate at the same speed, check which one is closer

	return pent2;
}

bool VRGroundEntityHandler::CheckIfPotentialGroundEntityForPlayer(edict_t* pent)
{
	if (pent->v.solid != SOLID_BSP)
		return false;

	if (ValidGroundEntityClassnames.count(STRING(pent->v.classname)) == 0)
		return false;

	if (pent->v.velocity.LengthSquared() == 0.f && pent->v.avelocity.LengthSquared() == 0.f)
		return false;

	EHANDLE<CBaseEntity> hEntity = CBaseEntity::SafeInstance<CBaseEntity>(pent);
	if (!hEntity)
		return false;

	// Detect if player is in control area of a usable train
	if ((hEntity->ObjectCaps() & FCAP_DIRECTIONAL_USE) && hEntity->OnControls(m_pPlayer->pev))
		return true;

	// We use the physics engine to do proper collision detection with the bsp model.
	// We half the player's width to avoid being "pulled" or slung around by objects nearby.
	// We extend the player's height and lower the origin a bit to make sure we detect movement even if slightly above ground (HMD jittering etc.)
	return VRPhysicsHelper::Instance().ModelIntersectsCapsule(
		hEntity,
		/*center*/ m_pPlayer->pev->origin - Vector{ 0.f, 0.f, 8.f },
		/*radius*/ m_pPlayer->pev->size.x,
		/*height*/ double(m_pPlayer->pev->size.z) + 8.0);
}

bool VRGroundEntityHandler::IsExcludedAsGroundEntity(edict_t* pent)
{
	if (FNullEnt(pent) || pent->v.size.x < EPSILON || pent->v.size.y < EPSILON || pent->v.size.z < EPSILON)
		return true;

	if (FClassnameIs(&pent->v, "func_door_rotating"))
	{
		if (pent->v.size.z > pent->v.size.y && pent->v.size.z > pent->v.size.x)
		{
			constexpr const float DoorRatio = 4.f;
			float ratio = pent->v.size.x / pent->v.size.y;
			if (ratio > DoorRatio || ratio < (1.f / DoorRatio))
				return true;
		}
	}

	return false;
}

void VRGroundEntityHandler::SendGroundEntityToClient()
{
	extern int gmsgVRGroundEntity;
	MESSAGE_BEGIN(MSG_ONE, gmsgVRGroundEntity, nullptr, m_pPlayer->pev);
	if (m_hGroundEntity)
	{
		WRITE_ENTITY(ENTINDEX(m_hGroundEntity->edict()));
	}
	else
	{
		WRITE_SHORT(0);
	}
	MESSAGE_END();
}

Vector VRGroundEntityHandler::CalculateNewOrigin()
{
	Vector groundEntityMoved = m_hGroundEntity->pev->origin - m_lastGroundEntityOrigin;
	Vector groundEntityRotated = m_hGroundEntity->pev->angles - m_lastGroundEntityAngles;
	m_lastGroundEntityOrigin = m_hGroundEntity->pev->origin;
	m_lastGroundEntityAngles = m_hGroundEntity->pev->angles;

	Vector newOrigin = m_pPlayer->pev->origin + groundEntityMoved;
	if (groundEntityRotated.LengthSquared() != 0.f)
	{
		Vector newOriginOffset = newOrigin - m_hGroundEntity->pev->origin;
		Vector newOriginOffsetRotated = VRPhysicsHelper::Instance().RotateVectorInline(newOriginOffset, groundEntityRotated);
		if (newOriginOffsetRotated.z != newOriginOffset.z)
		{
			// If rotating upwards or downwards give a little space to avoid getting stuck in the floor
			float deltaZ = fabs(newOriginOffsetRotated.z - newOriginOffset.z);
			newOriginOffsetRotated.z += (deltaZ * (m_pPlayer->pev->maxs.x * 1.5f)) / newOriginOffset.Length2D();
		}
		newOrigin = m_hGroundEntity->pev->origin + newOriginOffsetRotated;
	}

	return newOrigin;
}

bool VRGroundEntityHandler::CheckIfNewOriginIsValid(const Vector& newOrigin)
{
	Vector dir = newOrigin - m_pPlayer->pev->origin;
	Vector dirNorm = dir.Normalize();

	Vector newEyePosition = newOrigin + m_pPlayer->pev->view_ofs;
	Vector predictedEyePosition = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + (dirNorm * 16.f);

	int newEyeContents = UTIL_PointContents(newEyePosition);
	int predictedEyeContents = UTIL_PointContents(predictedEyePosition);

	bool eyePositionValid = newEyeContents != CONTENTS_SKY && newEyeContents != CONTENTS_SOLID;
	bool predictedEyePositionValid = predictedEyeContents != CONTENTS_SKY && predictedEyeContents != CONTENTS_SOLID;

	return eyePositionValid && predictedEyePositionValid;
}

void VRGroundEntityHandler::MoveWithGroundEntity()
{
	if (!m_hGroundEntity)
		return;

	Vector newOrigin = CalculateNewOrigin();
	//m_pPlayer->pev->origin = newOrigin;

	if (CheckIfNewOriginIsValid(newOrigin))
	{
		// Apply new origin
		m_pPlayer->pev->origin = newOrigin;
	}
	else
	{
		// When the target position is invalid, "slide" towards the center of the groundentity origin to avoid ceilings/corners/walls
		float distanceTravelled = (newOrigin - m_pPlayer->pev->origin).Length();
		Vector groundEntityCenter = (m_hGroundEntity->pev->absmin + m_hGroundEntity->pev->absmax) / 2;
		Vector dirToCenter = groundEntityCenter - newOrigin;
		if (dirToCenter.Length() > distanceTravelled)
		{
			newOrigin = newOrigin + (dirToCenter.Normalize() * distanceTravelled);
		}
		else
		{
			ALERT(at_console, "Error: Can't set groundentity target origin: would get moved into wall and distance to center is smaller than distance travelled!\n");
		}
	}
}
