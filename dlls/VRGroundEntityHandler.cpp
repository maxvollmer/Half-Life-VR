
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

	CBaseEntity* pGroundEntity = nullptr;

	// Take train if conditions are met
	bool forceIntroTrainRide = CVAR_GET_FLOAT("vr_force_introtrainride") != 0.f;
	std::string mapName{ STRING(INDEXENT(0)->v.model) };
	if (forceIntroTrainRide && IntroTrainRideMapNames.count(mapName) != 0)
	{
		pGroundEntity = UTIL_FindEntityByTargetname(nullptr, "train");
		if (pGroundEntity)
		{
			// if we fell out, teleport us back in
			if (!UTIL_PointInsideBBox(m_pPlayer->pev->origin, pGroundEntity->pev->absmin, pGroundEntity->pev->absmax) || m_pPlayer->pev->origin.z < (pGroundEntity->pev->origin.z + pGroundEntity->pev->mins.z))
			{
				m_pPlayer->pev->origin = pGroundEntity->pev->origin;
				m_pPlayer->pev->origin.z += 64.f;
				m_pPlayer->pev->velocity = Vector{};
			}
		}
	}

	if (!pGroundEntity)
	{
		// If engine has found a groundentity, take it
		if (m_pPlayer->pev->groundentity && !m_pPlayer->pev->groundentity->free && !FNullEnt(m_pPlayer->pev->groundentity))
		{
			pGroundEntity = CBaseEntity::SafeInstance<CBaseEntity>(m_pPlayer->pev->groundentity);
		}

		// Try to find a ground entity using PM code
		if (!pGroundEntity)
		{
			extern playermove_t* pmove;
			if (pmove && pmove->numphysent > 1 && pmove->physents[0].model->needload == 0)
			{
				int entindex = pmove->PM_TestPlayerPosition(m_pPlayer->pev->origin, nullptr);
				if (entindex > 0)
				{
					pGroundEntity = CBaseEntity::SafeInstance<CBaseEntity>(INDEXENT(pmove->physents[entindex].info));
					if (IsExcludedAsGroundEntity(pGroundEntity))
					{
						pGroundEntity = nullptr;
					}
				}
			}
		}

		// Use physics engine to find potentially better ground entity
		CBaseEntity* pEntity = nullptr;
		while (UTIL_FindAllEntities(&pEntity))
		{
			if (CheckIfPotentialGroundEntityForPlayer(pEntity) && !IsExcludedAsGroundEntity(pEntity))
			{
				pGroundEntity = ChoseBetterGroundEntityForPlayer(pGroundEntity, pEntity);
			}
		}
	}

	if (pGroundEntity && m_hGroundEntity != pGroundEntity)
	{
		m_lastGroundEntityOrigin = pGroundEntity->pev->origin;
		m_lastGroundEntityAngles = pGroundEntity->pev->angles;
	}

	if (pGroundEntity)
	{
		m_pPlayer->pev->groundentity = pGroundEntity->edict();
	}
	else
	{
		if (m_hGroundEntity && m_pPlayer->pev->groundentity == m_hGroundEntity->edict())
		{
			m_pPlayer->pev->groundentity = nullptr;
		}
	}

	m_hGroundEntity = pGroundEntity;
}

CBaseEntity* VRGroundEntityHandler::ChoseBetterGroundEntityForPlayer(CBaseEntity* pEntity1, CBaseEntity* pEntity2)
{
	if (!pEntity1)
		return pEntity2;

	if (!pEntity2)
		return pEntity1;

	// Check which one moves faster
	if (pEntity1->pev->velocity.LengthSquared() > pEntity2->pev->velocity.LengthSquared())
		return pEntity1;

	if (pEntity2->pev->velocity.LengthSquared() > pEntity1->pev->velocity.LengthSquared())
		return pEntity2;

	// Check which one rotates faster
	if (pEntity1->pev->avelocity.LengthSquared() > pEntity2->pev->avelocity.LengthSquared())
		return pEntity1;

	if (pEntity2->pev->avelocity.LengthSquared() > pEntity1->pev->avelocity.LengthSquared())
		return pEntity2;

	// Both move and rotate at the same speed, check which one is closer

	return pEntity2;
}

bool VRGroundEntityHandler::CheckIfPotentialGroundEntityForPlayer(CBaseEntity* pEntity)
{
	if (pEntity->pev->solid != SOLID_BSP)
		return false;

	if (ValidGroundEntityClassnames.count(std::string{ STRING(pEntity->pev->classname) }) == 0)
		return false;

	if (pEntity->pev->velocity.LengthSquared() == 0.f && pEntity->pev->avelocity.LengthSquared() == 0.f)
		return false;

	// Detect if player is in control area of a usable train
	if ((pEntity->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pEntity->OnControls(m_pPlayer->pev))
		return true;

	// We use the physics engine to do proper collision detection with the bsp model.
	// We half the player's width to avoid being "pulled" or slung around by objects nearby.
	// We extend the player's height and lower the origin a bit to make sure we detect movement even if slightly above ground (HMD jittering etc.)
	return VRPhysicsHelper::Instance().ModelIntersectsCapsule(
		pEntity,
		m_pPlayer->pev->origin - Vector{ 0.f, 0.f, 8.f },
		/*radius*/ m_pPlayer->pev->size.x,
		/*height*/ m_pPlayer->pev->size.z + 8.f);
}

bool VRGroundEntityHandler::IsExcludedAsGroundEntity(CBaseEntity* pEntity)
{
	if (!pEntity || pEntity->pev->size.x < EPSILON || pEntity->pev->size.y < EPSILON || pEntity->pev->size.z < EPSILON)
		return true;

	if (FClassnameIs(pEntity->pev, "func_door_rotating"))
	{
		if (pEntity->pev->size.z > pEntity->pev->size.y&& pEntity->pev->size.z > pEntity->pev->size.x)
		{
			constexpr const float DoorRatio = 4.f;
			float ratio = pEntity->pev->size.x / pEntity->pev->size.y;
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
