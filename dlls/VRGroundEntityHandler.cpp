
#include <algorithm>

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"

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
		"maps/c0a0d.bsp"
	};
}

void VRGroundEntityHandler::HandleMovingWithSolidGroundEntities()
{
	DetectAndSetGroundEntity();
	SendGroundEntityToClient();
	MoveWithGroundEntity();
}

void VRGroundEntityHandler::DetectAndSetGroundEntity()
{
	CBaseEntity* pGroundEntity = nullptr;

	float vr_force_introtrainride  = CVAR_GET_FLOAT("vr_force_introtrainride");
	std::string mapName{ STRING(INDEXENT(0)->v.model) };

	if (vr_force_introtrainride != 0.f && IntroTrainRideMapNames.count(mapName) != 0)
	{
		pGroundEntity = UTIL_FindEntityByTargetname(nullptr, "train");
	}
	else
	{
		CBaseEntity* pEntity = nullptr;
		while (UTIL_FindAllEntities(&pEntity))
		{
			if (CheckIfPotentialGroundEntityForPlayer(pEntity))
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
		m_isTouchingGroundEntityFloor = false; // TODO: Check if player absmin.z is inside groundentity's bsp model
	}
	else
	{
		if (m_hGroundEntity && m_pPlayer->pev->groundentity == m_hGroundEntity->edict())
		{
			m_pPlayer->pev->groundentity = nullptr;
		}
		m_isTouchingGroundEntityFloor = false;
	}

	if (m_hGroundEntity != pGroundEntity)
	{
		if (pGroundEntity)
		{
			ALERT(at_console, "GROUNDENTITY: %s\n", STRING(pGroundEntity->pev->targetname));
		}
		else
		{
			ALERT(at_console, "NO GROUNDENTITY\n");
		}
	}

	m_hGroundEntity = pGroundEntity;
}

CBaseEntity* VRGroundEntityHandler::ChoseBetterGroundEntityForPlayer(CBaseEntity *pEntity1, CBaseEntity *pEntity2)
{
	if (!pEntity1)
		return pEntity2;

	if (!pEntity2)
		return pEntity1;

	// Check with one moves faster
	if (pEntity1->pev->velocity.LengthSquared() > pEntity2->pev->velocity.LengthSquared())
		return pEntity1;

	if (pEntity2->pev->velocity.LengthSquared() > pEntity1->pev->velocity.LengthSquared())
		return pEntity2;

	// Check with one rotates faster
	if (pEntity1->pev->avelocity.LengthSquared() > pEntity2->pev->avelocity.LengthSquared())
		return pEntity1;

	if (pEntity2->pev->avelocity.LengthSquared() > pEntity1->pev->avelocity.LengthSquared())
		return pEntity2;

	// Both move and rotate at the same speed, check which one is closer

	return pEntity2;
}

bool VRGroundEntityHandler::CheckIfPotentialGroundEntityForPlayer(CBaseEntity *pEntity)
{
	if (pEntity->pev->solid != SOLID_BSP)
		return false;

	if (ValidGroundEntityClassnames.count(std::string{ STRING(pEntity->pev->classname) }) == 0)
		return false;

	if (pEntity->pev->velocity.LengthSquared() == 0.f && pEntity->pev->avelocity.LengthSquared() == 0.f)
		return false;

	// We use the physics engine to do proper collision detection with the bsp model.
	// We half the player's width to avoid being "pulled" or slung around by objects nearby.
	// We extend the player's height and lower the origin a bit to make sure we detect movement even if slightly above ground (HMD jittering etc.)
	return VRPhysicsHelper::Instance().ModelIntersectsCapsule(
		pEntity,
		m_pPlayer->pev->origin - Vector{ 0.f, 0.f, 8.f },
		/*radius*/m_pPlayer->pev->size.x * 0.5f,
		/*height*/m_pPlayer->pev->size.z + 8.f);
}

void VRGroundEntityHandler::SendGroundEntityToClient()
{
	extern int gmsgVRGroundEntity;
	MESSAGE_BEGIN(MSG_ONE, gmsgVRGroundEntity, NULL, m_pPlayer->pev);
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

void VRGroundEntityHandler::MoveWithGroundEntity()
{
	if (!m_hGroundEntity)
		return;

	Vector groundEntityMoved = m_hGroundEntity->pev->origin - m_lastGroundEntityOrigin;
	Vector groundEntityRotated = m_hGroundEntity->pev->angles - m_lastGroundEntityAngles;

	Vector newOrigin = m_pPlayer->pev->origin + groundEntityMoved;
	if (groundEntityRotated.LengthSquared() != 0.f)
	{
		Vector newOriginOffset = newOrigin - m_hGroundEntity->pev->origin;
		Vector newOriginOffsetRotated = VRPhysicsHelper::Instance().RotateVectorInline(newOriginOffset, groundEntityRotated);
		newOrigin = m_hGroundEntity->pev->origin + newOriginOffsetRotated;
	}
	m_pPlayer->pev->origin = newOrigin;

	m_lastGroundEntityOrigin = m_hGroundEntity->pev->origin;
	m_lastGroundEntityAngles = m_hGroundEntity->pev->angles;
	//m_lastGroundEntityOffset = groundEntityOffset;





	// TODO
	// TODO Movement with ground entity needs to be calculated in client in combination with client movement
	// TODO


	/*
	// Calculate ground velocity
	Vector groundVelocity = m_hGroundEntity->pev->velocity;

	// Add avelocity
	if (m_hGroundEntity->pev->avelocity.Length() > 0.01f)
	{
		groundVelocity = groundVelocity + VRPhysicsHelper::Instance().AngularVelocityToLinearVelocity(m_hGroundEntity->pev->avelocity, m_pPlayer->pev->origin - m_hGroundEntity->pev->origin);
	}

	// Don't slow down falling when moving downwards and player mins still inside ground entity
	if (groundVelocity.z < 0 && m_hGroundEntity->pev->absmin.z < m_pPlayer->pev->absmin.z) {
		groundVelocity.z = (std::min)(groundVelocity.z, m_pPlayer->pev->velocity.z);
	}

	// Apply ground velocity and get new origin
	Vector newOrigin = m_pPlayer->pev->origin + (groundVelocity * gpGlobals->frametime);

	// Make sure new origin is valid (don't get moved through walls/into void etc)
	Vector dir = (newOrigin - m_pPlayer->pev->origin).Normalize();
	Vector newEyePosition = newOrigin + m_pPlayer->pev->view_ofs;
	Vector predictedEyePosition = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + (dir * 16.f);
	int newEyeContents = UTIL_PointContents(newEyePosition);
	int predictedEyeContents = UTIL_PointContents(predictedEyePosition);
	if (newEyeContents != CONTENTS_SKY && newEyeContents != CONTENTS_SOLID
		&& predictedEyeContents != CONTENTS_SKY && predictedEyeContents != CONTENTS_SOLID)
	{
		// Apply new origin
		m_pPlayer->pev->origin = newOrigin;
	}
	else
	{
		// When we're moving up- or downwards, and the groundentity's center is valid,
		// "slide" on x/y towards the center of the groundentity origin to avoid ceilings/corners/walls
		Vector groundEntityCenter = (m_hGroundEntity->pev->absmin + m_hGroundEntity->pev->absmax) / 2;
		int groundEntityCenterContents = UTIL_PointContents(groundEntityCenter);
		if (groundEntityCenterContents != CONTENTS_SKY && groundEntityCenterContents != CONTENTS_SOLID)
		{
			Vector dirToCenter2D = groundEntityCenter - newOrigin;
			dirToCenter2D.z = 0.f;
			float distanceToCenter2D = dirToCenter2D.Length();
			if (distanceToCenter2D > 8.f)
			{
				dirToCenter2D = dirToCenter2D.Normalize();
				float moveDistance = (std::min)((std::max)(groundVelocity.Length() * gpGlobals->frametime, 1.f), distanceToCenter2D);
				m_pPlayer->pev->origin.x = newOrigin.x + dirToCenter2D.x * moveDistance;
				m_pPlayer->pev->origin.y = newOrigin.y + dirToCenter2D.y * moveDistance;
				m_pPlayer->pev->origin.z = newOrigin.z;
			}
			else
			{
				ALERT(at_console, "distanceToCenter2D <= 8! pev->origin.z: %f\n", m_pPlayer->pev->origin.z);
			}
		}
		else
		{
			ALERT(at_console, "groundEntityCenterContents are solid! pev->origin.z: %f\n", m_pPlayer->pev->origin.z);
		}
	}
	*/
}

#if 0
void VRGroundEntityHandler::MoveWithGroundEntity()
{
	if (!m_hGroundEntity)
	{
		return;
	}

	/*
	// Take distance player has moved through input or other means
	//Vector playerMoved = m_pPlayer->pev->origin - m_lastPlayerOrigin;

	// Reset player origin
	//m_pPlayer->pev->origin = m_lastPlayerOrigin;

	// Calculate the rotated offset
	Vector angleDifference = m_hGroundEntity->pev->angles - m_initialGroundEntityAngles;
	Vector rotatedOffset = VRPhysicsHelper::Instance().RotateVectorInline(m_initialPlayerOffsetToGroundEntity, angleDifference);

	// Calculate new player origin as moving with ground entity
	m_pPlayer->pev->origin = m_hGroundEntity->pev->origin + rotatedOffset;
	*/

	/*
	// Add player movement back in
	m_pPlayer->pev->origin = m_pPlayer->pev->origin + playerMoved;

	// Never move into or fall through ground entities
	if (m_isTouchingGroundEntityFloor || m_pPlayer->pev->absmin.z < m_hGroundEntity->pev->absmin.z)
	{
		m_pPlayer->pev->origin.z = m_hGroundEntity->pev->absmin.z - m_pPlayer->pev->mins.z;
		m_pPlayer->pev->velocity.z = (std::max)(m_pPlayer->pev->velocity.z, 0.f);
	}

	// Remember last player origin to account for actual player movement
	m_lastPlayerOrigin = m_pPlayer->pev->origin;
	*/

	/*
	// Make sure new origin is valid (don't get moved through walls/into void etc)
	Vector dir = (newOrigin - m_pPlayer->pev->origin).Normalize();
	Vector newEyePosition = newOrigin + m_pPlayer->pev->view_ofs;
	Vector predictedEyePosition = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + (dir * 16.f);
	int newEyeContents = UTIL_PointContents(newEyePosition);
	int predictedEyeContents = UTIL_PointContents(predictedEyePosition);
	if (newEyeContents != CONTENTS_SKY && newEyeContents != CONTENTS_SOLID
		&& predictedEyeContents != CONTENTS_SKY && predictedEyeContents != CONTENTS_SOLID)
	{
		// Apply new origin
		m_pPlayer->pev->origin = newOrigin;
	}
	else
	{
		// When we're moving up- or downwards, and the groundentity's center is valid,
		// "slide" on x/y towards the center of the groundentity origin to avoid ceilings/corners/walls
		Vector groundEntityCenter = (m_hGroundEntity->pev->absmin + m_hGroundEntity->pev->absmax) / 2;
		int groundEntityCenterContents = UTIL_PointContents(groundEntityCenter);
		if (groundEntityCenterContents != CONTENTS_SKY && groundEntityCenterContents != CONTENTS_SOLID)
		{
			Vector dirToCenter2D = groundEntityCenter - m_pPlayer->pev->origin;
			dirToCenter2D.z = 0.f;
			float distanceToCenter2D = dirToCenter2D.Length();
			if (distanceToCenter2D > 8.f)
			{
				dirToCenter2D = dirToCenter2D.Normalize();
				float moveDistance = (std::min)((std::max)(groundVelocity.Length() * gpGlobals->frametime, 1.f), distanceToCenter2D);
				m_pPlayer->pev->origin.x += dirToCenter2D.x * moveDistance;
				m_pPlayer->pev->origin.y += dirToCenter2D.y * moveDistance;
				m_pPlayer->pev->origin.z = newOrigin.z;
			}
			else
			{
				ALERT(at_console, "distanceToCenter2D <= 8! pev->origin.z: %f\n", m_pPlayer->pev->origin.z);
			}
		}
		else
		{
			ALERT(at_console, "groundEntityCenterContents are solid! pev->origin.z: %f\n", m_pPlayer->pev->origin.z);
		}
	}
	*/
}
#endif
