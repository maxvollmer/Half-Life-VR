
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"

#include "game.h"

#include "basemonster.h"
#include "talkmonster.h"
#include "weapons.h"
#include "explode.h"
#include "func_break.h"

#include "VRControllerInteractionManager.h"
#include "VRPhysicsHelper.h"
#include "CWorldsSmallestCup.h"
#include "VRModelHelper.h"
#include "VRMovementHandler.h"

#include "VRRotatableEnt.h"


#define SF_BUTTON_TOUCH_ONLY	256	// button only fires as a result of USE key.
#define SF_DOOR_USE_ONLY		256	// door must be opened by player's use button.

#define	bits_COND_CLIENT_PUSH (bits_COND_SPECIAL1)

constexpr const int VR_MAX_ANGRY_GUNPOINT_DIST = 256;
constexpr const int VR_INITIAL_ANGRY_GUNPOINT_TIME = 2;
constexpr const int VR_MIN_ANGRY_GUNPOINT_TIME = 3;
constexpr const int VR_MAX_ANGRY_GUNPOINT_TIME = 7;

constexpr const int VR_SHOULDER_TOUCH_TIME = 1.5f;
constexpr const int VR_STOP_SIGNAL_TIME = 1;

constexpr const int VR_SHOULDER_TOUCH_MIN_Z = 56;
constexpr const int VR_SHOULDER_TOUCH_MAX_Z = 62;
constexpr const int VR_SHOULDER_TOUCH_MIN_Y = 4;
constexpr const int VR_SHOULDER_TOUCH_MAX_Y = 12;

constexpr const int VR_STOP_SIGNAL_MIN_Z = 56;
constexpr const int VR_STOP_SIGNAL_MAX_Z = 72;
constexpr const int VR_STOP_SIGNAL_MIN_X = 8;
constexpr const int VR_STOP_SIGNAL_MAX_X = 36;
constexpr const int VR_STOP_SIGNAL_MIN_Y = -12;
constexpr const int VR_STOP_SIGNAL_MAX_Y = 12;

constexpr const float VR_RETINASCANNER_ACTIVATE_LOOK_TIME = 1.5f;

// Global VR related stuffs - Max Vollmer, 2018-04-02
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
std::unordered_map<EHANDLE, EHANDLE, EHANDLE::Hash, EHANDLE::Equal> g_vrRetinaScanners;
std::unordered_set<EHANDLE, EHANDLE::Hash, EHANDLE::Equal> g_vrRetinaScannerButtons;

EHANDLE g_vrHRetinaScanner;
float g_vrRetinaScannerLookTime = 0.f;
bool g_vrRetinaScannerUsed = false;


bool CheckShoulderTouch(CBaseMonster *pMonster, const Vector & pos)
{
	Vector posInMonsterLocalSpace = pos - pMonster->pev->origin;
	if (posInMonsterLocalSpace.z >= (VR_SHOULDER_TOUCH_MIN_Z*pMonster->pev->scale) && posInMonsterLocalSpace.z <= (VR_SHOULDER_TOUCH_MAX_Z*pMonster->pev->scale))
	{
		float angle = -pMonster->pev->angles.y * M_PI / 180.f;
		//float localX = cos(angle)*posInMonsterLocalSpace.x - sin(angle)*posInMonsterLocalSpace.y;
		float localY = sin(angle)*posInMonsterLocalSpace.x + cos(angle)*posInMonsterLocalSpace.y;
		if (fabs(localY) >= (VR_SHOULDER_TOUCH_MIN_Y*pMonster->pev->scale) && fabs(localY) <= (VR_SHOULDER_TOUCH_MAX_Y*pMonster->pev->scale))
		{
			return true;
		}
	}
	return false;
}

bool CheckStopSignal(CBaseMonster *pMonster, const Vector & pos, const Vector & angles)
{
	Vector posInMonsterLocalSpace = pos - pMonster->pev->origin;
	if (posInMonsterLocalSpace.z >= (VR_STOP_SIGNAL_MIN_Z*pMonster->pev->scale) && posInMonsterLocalSpace.z <= (VR_STOP_SIGNAL_MAX_Z*pMonster->pev->scale))
	{
		float angle = -pMonster->pev->angles.y * M_PI / 180.f;
		float localX = cos(angle)*posInMonsterLocalSpace.x - sin(angle)*posInMonsterLocalSpace.y;
		float localY = sin(angle)*posInMonsterLocalSpace.x + cos(angle)*posInMonsterLocalSpace.y;
		if (localX >= (VR_STOP_SIGNAL_MIN_X*pMonster->pev->scale) && localX <= (VR_STOP_SIGNAL_MAX_X*pMonster->pev->scale)
			&& localY >= (VR_STOP_SIGNAL_MIN_Y*pMonster->pev->scale) && localY <= (VR_STOP_SIGNAL_MAX_Y*pMonster->pev->scale))
		{
			// Stop signal is given when hand is pointing up and back of hand is facing away from ally
			Vector forward;
			Vector up;
			Vector monsterForward;
			UTIL_MakeAimVectorsPrivate(angles, forward, up, nullptr);
			UTIL_MakeAimVectorsPrivate(pMonster->pev->angles, monsterForward, nullptr, nullptr);
			bool isHandFacingUp = DotProduct(forward, Vector(0, 0, 1)) > 0.9f;
			bool isBackOfHandFacingAway = true;// DotProduct(up, monsterForward) > 0.9f;	// TODO: Buggy
			return isHandFacingUp && isBackOfHandFacingAway;
		}
	}
	return false;
}

bool WasJustThrownByPlayer(CBasePlayer* pPlayer, EHANDLE hEntity)
{
	if (hEntity->pev->owner != pPlayer->edict())
		return false;

	if (!FClassnameIs(hEntity->pev, "monster_satchel")
		&& !FClassnameIs(hEntity->pev, "grenade")
		&& !FClassnameIs(hEntity->pev, "monster_snark"))
		return false;

	return (gpGlobals->time - hEntity->m_spawnTime) < 2.f;
}

bool IsNonInteractingEntity(EHANDLE hEntity)
{
	return FClassnameIs(hEntity->pev, "func_wall")
		|| FClassnameIs(hEntity->pev, "func_illusionary")
		|| FClassnameIs(hEntity->pev, "vr_controllermodel");	// TODO/NOTE: If this mod gets ever patched up for multiplayer, and you want players to be able to crowbar-fight, this should probably be changed
}

bool VRControllerInteractionManager::CheckIfEntityAndControllerTouch(CBasePlayer* pPlayer, EHANDLE hEntity, const VRController& controller, VRPhysicsHelperModelBBoxIntersectResult* intersectResult)
{
	if (!controller.IsValid())
		return false;

	if (controller.GetRadius() <= 0.f)
		return false;

	if ((hEntity->pev->solid == SOLID_NOT || IsNonInteractingEntity(hEntity)) && !IsDraggableEntity(hEntity))
		return false;

	// Don't hit stuff we just threw ourselfes
	if (WasJustThrownByPlayer(pPlayer, hEntity))
		return false;

	// Rule out entities too far away
	CBaseEntity *pWorld = CBaseEntity::Instance(INDEXENT(0));
	if (hEntity != pWorld)
	{
		Vector entityCenter = (hEntity->pev->absmax + hEntity->pev->absmin) * 0.5f;
		float distance = (controller.GetPosition() - entityCenter).Length();
		float entityRadius = (std::max)(hEntity->pev->mins.Length(), hEntity->pev->maxs.Length());
		if (distance > (controller.GetRadius() + entityRadius))
		{
			return false;
		}
	}
	else
	{
		int kfjskgj = 0;
	}

	// Check each hitbox of current weapon
	for (auto hitbox : controller.GetHitBoxes())
	{
		if (VRPhysicsHelper::Instance().ModelIntersectsBBox(hEntity, hitbox.origin, hitbox.mins, hitbox.maxs, hitbox.angles, intersectResult) || UTIL_IsPointInEntity(hEntity, hitbox.origin))
		{
			if (!intersectResult->hasresult)
			{
				intersectResult->hitpoint = hitbox.origin;
				intersectResult->hitgroup = 0;
				intersectResult->hasresult = true;
			}
			return true;
		}
	}

	return false;
}

bool VRControllerInteractionManager::IsDraggableEntity(EHANDLE hEntity)
{
	return FClassnameIs(hEntity->pev, "vr_easteregg")
		|| FClassnameIs(hEntity->pev, "func_rot_button")
		|| FClassnameIs(hEntity->pev, "momentary_rot_button")
		|| FClassnameIs(hEntity->pev, "func_pushable")
		|| hEntity->IsDraggable();
}

void VRControllerInteractionManager::CheckAndPressButtons(CBasePlayer *pPlayer, const VRController& controller)
{
	CBaseEntity *pEntity = nullptr;
	while (UTIL_FindAllEntities(&pEntity))
	{
		if (pEntity == pPlayer)
		{
			continue;
		}

		if (pEntity == controller.GetModel())
		{
			continue;
		}

		// skip point entities (no model and/or no size)
		if (FStringNull(pEntity->pev->model) && pEntity->pev->size.LengthSquared() < EPSILON)
		{
			continue;
		}

		EHANDLE hEntity = pEntity;

		VRPhysicsHelperModelBBoxIntersectResult intersectResult;
		bool isTouching;
		bool didTouchChange;
		bool isDragging;
		bool didDragChange;
		bool isHitting;
		bool didHitChange;

		// If we are dragging something draggable, we override all the booleans to avoid "losing" the entity due to fast movements
		if (controller.IsDragging() && controller.GetWeaponId() == WEAPON_BAREHAND && controller.IsDraggedEntity(hEntity) && IsDraggableEntity(hEntity))
		{
			isTouching = true;
			didTouchChange = false;
			isDragging = true;
			didDragChange = false;
			isHitting = false;
			didHitChange = controller.RemoveHitEntity(hEntity);
		}
		else
		{
			isTouching = CheckIfEntityAndControllerTouch(pPlayer, hEntity, controller, &intersectResult);
			didTouchChange = isTouching ? controller.AddTouchedEntity(hEntity) : controller.RemoveTouchedEntity(hEntity);

			isDragging = isTouching && controller.GetWeaponId() == WEAPON_BAREHAND && controller.IsDragging();
			didDragChange = isDragging ? controller.AddDraggedEntity(hEntity) : controller.RemoveDraggedEntity(hEntity);

			if (isTouching)
			{
				Vector controllerSwingVelocity = controller.GetVelocity();
				if (intersectResult.hasresult)
				{
					Vector relativeHitPos = intersectResult.hitpoint - controller.GetPosition();
					Vector previousRotatedRelativeHitPos = VRPhysicsHelper::Instance().RotateVectorInline(relativeHitPos, controller.GetPreviousAngles());
					Vector rotatedRelativeHitPos = VRPhysicsHelper::Instance().RotateVectorInline(relativeHitPos, controller.GetAngles());
					controllerSwingVelocity = controllerSwingVelocity + rotatedRelativeHitPos - previousRotatedRelativeHitPos;
				}
				Vector relativeVelocity = pPlayer->pev->velocity + controllerSwingVelocity - hEntity->pev->velocity;
				isHitting = controllerSwingVelocity.Length() > GetMeleeSwingSpeed() && relativeVelocity.Length() > GetMeleeSwingSpeed();
			}
			else
			{
				isHitting = false;
			}
			didHitChange = isHitting ? controller.AddHitEntity(hEntity) : controller.RemoveHitEntity(hEntity);
		}

		if (!isDragging && didDragChange)
		{
			int i = 0;
		}

		float flHitDamage = 0.f;
		if (pPlayer->HasWeapons() && isHitting && didHitChange)
		{
			flHitDamage = DoDamage(pPlayer, hEntity, controller, intersectResult);
		}

		Interaction::InteractionInfo touching{ isTouching, didTouchChange };
		Interaction::InteractionInfo dragging{ isDragging, didDragChange };
		Interaction::InteractionInfo hitting{ isHitting, didHitChange };
		Interaction interaction{ touching, dragging, hitting, flHitDamage, intersectResult.hasresult ? intersectResult.hitpoint : controller.GetPosition() };

		if (HandleEasterEgg(pPlayer, hEntity, controller, isTouching, didTouchChange));	// easter egg first, obviously the most important
		else if (HandleRetinaScanners(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleButtons(pPlayer, hEntity, controller, isTouching, didTouchChange, isDragging, didDragChange));
		else if (HandleDoors(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleRechargers(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleTriggers(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleBreakables(pPlayer, hEntity, controller, isTouching, didTouchChange, isHitting, didHitChange, flHitDamage));
		else if (HandlePushables(pPlayer, hEntity, controller, interaction));
		else if (HandleGrabbables(pPlayer, hEntity, controller, interaction));
		else if (HandleAlliedMonsters(pPlayer, hEntity, controller, isTouching, didTouchChange, isHitting, didHitChange, flHitDamage));
		else if (isTouching && didTouchChange)
		{
			// Touch any entity not handled yet and set player velocity temporarily to controller velocity, as some entities (e.g. cockroaches) use the velocity in their touch logic
			Vector backupVel = pPlayer->pev->velocity;
			pPlayer->pev->velocity = controller.GetVelocity();
			hEntity->Touch(pPlayer);
			pPlayer->pev->velocity = backupVel;
		}
	}
}

bool IsSoftWeapon(int weaponId)
{
	switch (weaponId)
	{
	case WEAPON_HORNETGUN:
	case WEAPON_NONE:
	case WEAPON_BAREHAND:
	case WEAPON_SNARK:
		return true;
	default:
		return false;
	}
}

float VRControllerInteractionManager::DoDamage(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, const VRPhysicsHelperModelBBoxIntersectResult& intersectResult)
{
	if (hEntity->pev->solid == SOLID_NOT || hEntity->pev->solid == SOLID_TRIGGER)
		return 0.f;

	Vector controllerSwingVelocity = controller.GetVelocity();
	if (intersectResult.hasresult)
	{
		Vector relativeHitPos = intersectResult.hitpoint - controller.GetPosition();
		Vector previousRotatedRelativeHitPos = VRPhysicsHelper::Instance().RotateVectorInline(relativeHitPos, controller.GetPreviousAngles());
		Vector rotatedRelativeHitPos = VRPhysicsHelper::Instance().RotateVectorInline(relativeHitPos, controller.GetAngles());
		controllerSwingVelocity = controllerSwingVelocity + rotatedRelativeHitPos - previousRotatedRelativeHitPos;
	}

	float speed = (pPlayer->pev->velocity + controllerSwingVelocity - hEntity->pev->velocity).Length();
	float damage = UTIL_CalculateMeleeDamage(controller.GetWeaponId(), speed);
	if (damage > 0.f)
	{
		Vector impactPosition = intersectResult.hasresult ? intersectResult.hitpoint : controller.GetPosition();

		pPlayer->PlayMeleeSmackSound(hEntity, controller.GetWeaponId(), impactPosition, controllerSwingVelocity);

		int backupBlood = hEntity->m_bloodColor;
		if (IsSoftWeapon(controller.GetWeaponId()))
		{
			// Slapping with soft things (hands, snarks or hornetgun) just causes damage, but no blood or decals
			hEntity->m_bloodColor = DONT_BLEED;
		}
		TraceResult fakeTrace = { 0 };
		fakeTrace.pHit = hEntity->edict();
		fakeTrace.vecEndPos = impactPosition;
		fakeTrace.flFraction = 0.5f;
		if (intersectResult.hasresult)
		{
			fakeTrace.iHitgroup = intersectResult.hitgroup;
		}
		ClearMultiDamage();
		hEntity->TraceAttack(pPlayer->pev, damage, controllerSwingVelocity.Normalize(), &fakeTrace, UTIL_DamageTypeFromWeapon(controller.GetWeaponId()));
		ApplyMultiDamage(pPlayer->pev, pPlayer->pev);
		hEntity->m_bloodColor = backupBlood;

		// Entities that support being "baseballed" overwrite this method
		hEntity->BaseBalled(pPlayer, controllerSwingVelocity);

		// If you smack something with an explosive, it might just explode...
		// 25% chance that charged satchels go off when you smack something with the remote
		if (controller.GetWeaponId() == WEAPON_SATCHEL && pPlayer->m_pActiveItem->m_chargeReady && RANDOM_LONG(0, 4) == 0)
		{
			dynamic_cast<CBasePlayerWeapon*>(pPlayer->m_pActiveItem)->PrimaryAttack();
		}
		// 5% chance that an explosive explodes when hitting something
		else if (IsExplosiveWeapon(controller.GetWeaponId()) && RANDOM_LONG(0, 20) == 0 && pPlayer->m_rgAmmo[pPlayer->m_pActiveItem->PrimaryAmmoIndex()] > 0)
		{
			if (controller.GetWeaponId() == WEAPON_SNARK)
			{
				dynamic_cast<CSqueak*>(pPlayer->m_pActiveItem)->m_fJustThrown = 1;
				pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
				CBaseEntity *pSqueak = CBaseEntity::Create("monster_snark", controller.GetPosition(), controller.GetAngles(), pPlayer->edict());
				pSqueak->pev->health = -1;
				pSqueak->Killed(pPlayer->pev, DMG_CRUSH, GIB_ALWAYS);
				pSqueak = nullptr;
			}
			else
			{
				ExplosionCreate(controller.GetPosition(), controller.GetAngles(), pPlayer->edict(), pPlayer->m_pActiveItem->pev->dmg, TRUE);
			}

			pPlayer->m_rgAmmo[pPlayer->m_pActiveItem->PrimaryAmmoIndex()]--;
			dynamic_cast<CBasePlayerWeapon*>(pPlayer->m_pActiveItem)->m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;

			if (pPlayer->m_rgAmmo[pPlayer->m_pActiveItem->PrimaryAmmoIndex()] > 0)
			{
				dynamic_cast<CBasePlayerWeapon*>(pPlayer->m_pActiveItem)->Deploy();
			}
			else
			{
				dynamic_cast<CBasePlayerWeapon*>(pPlayer->m_pActiveItem)->RetireWeapon();
			}
		}
	}

	return damage;
}

bool VRControllerInteractionManager::HandleEasterEgg(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (FClassnameIs(hEntity->pev, "vr_easteregg"))
	{
		CWorldsSmallestCup* pWorldsSmallestCup = dynamic_cast<CWorldsSmallestCup*>((CBaseEntity*)hEntity);
		if (isTouching && controller.IsDragging())
		{
			pWorldsSmallestCup->m_isBeingDragged.insert(controller.GetID());
			pWorldsSmallestCup->pev->origin = controller.GetGunPosition();
			pWorldsSmallestCup->pev->angles = controller.GetAngles();
			pWorldsSmallestCup->pev->velocity = controller.GetVelocity();
		}
		else
		{
			pWorldsSmallestCup->m_isBeingDragged.erase(controller.GetID());
		}
		return true;
	}
	return false;
}

bool VRControllerInteractionManager::HandleRetinaScanners(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (g_vrRetinaScanners.count(hEntity) > 0)
	{
		Vector retinaScannerPosition = (hEntity->pev->absmax + hEntity->pev->absmin) / 2.f;
		bool isLookingAtRetinaScanner = UTIL_IsFacing(pPlayer->pev->origin, pPlayer->pev->angles.ToViewAngles(), retinaScannerPosition)
			&& ((pPlayer->EyePosition() - retinaScannerPosition).Length() < 32.f)
			&& (pPlayer->EyePosition().z >= hEntity->pev->absmin.z)
			&& (pPlayer->EyePosition().z <= hEntity->pev->absmax.z);

		if (isLookingAtRetinaScanner)
		{
			if (g_vrHRetinaScanner == hEntity)
			{
				if ((gpGlobals->time - g_vrRetinaScannerLookTime) >= VR_RETINASCANNER_ACTIVATE_LOOK_TIME)
				{
					if (!g_vrRetinaScannerUsed)
					{
						g_vrRetinaScanners[hEntity]->Use(pPlayer, pPlayer, USE_SET, 1);
						g_vrRetinaScannerUsed = true;
					}
				}
			}
			else
			{
				g_vrHRetinaScanner = hEntity;
				g_vrRetinaScannerLookTime = gpGlobals->time;
				g_vrRetinaScannerUsed = false;
			}
		}
		else
		{
			if (g_vrHRetinaScanner == hEntity)
			{
				g_vrHRetinaScanner = nullptr;
				g_vrRetinaScannerLookTime = 0.f;
				g_vrRetinaScannerUsed = false;
			}
		}
		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleButtons(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isDragging, bool didDragChange)
{
	// Don't touch activate retina scanners
	if (g_vrRetinaScannerButtons.count(hEntity) > 0)
		return true;

	if (FClassnameIs(hEntity->pev, "func_button"))
	{
		if (isTouching)
		{
			if (didTouchChange || FBitSet(hEntity->ObjectCaps(), FCAP_CONTINUOUS_USE))
			{
				if (FBitSet(hEntity->pev->spawnflags, SF_BUTTON_TOUCH_ONLY)) // touchable button
				{
					hEntity->Touch(pPlayer);
				}
				else
				{
					hEntity->Use(pPlayer, pPlayer, USE_SET, 1);
				}
			}
		}
		return true;
	}
	else if (FClassnameIs(hEntity->pev, "func_rot_button") || FClassnameIs(hEntity->pev, "momentary_rot_button"))
	{
		VRRotatableEnt* pRotatableEnt = hEntity->MyRotatableEntPtr();
		if (pRotatableEnt)
		{
			if (isDragging)
			{
				if (didDragChange)
				{
					pRotatableEnt->ClearDraggingCancelled();
				}
				if (!pRotatableEnt->IsDraggingCancelled())
				{
					Vector pos;
					if (controller.GetAttachment(VR_MUZZLE_ATTACHMENT, pos))
					{
						pRotatableEnt->VRRotate(pPlayer, pos, didDragChange);
					}
					else
					{
						pRotatableEnt->VRRotate(pPlayer, controller.GetPosition(), didDragChange);
					}
				}
			}
			else if (didDragChange)
			{
				pRotatableEnt->VRStopRotate();
			}
		}
		else
		{
			if (isDragging && didDragChange)
			{
				ALERT(at_console, "Error: Found rotating button not of type VRRotatableEnt: %s %s\n", STRING(hEntity->pev->classname), STRING(hEntity->pev->targetname));
			}
			if (FClassnameIs(hEntity->pev, "func_rot_button"))
			{
				if (isDragging)
				{
					if (didDragChange || FBitSet(hEntity->ObjectCaps(), FCAP_CONTINUOUS_USE))
					{
						if (FBitSet(hEntity->pev->spawnflags, SF_BUTTON_TOUCH_ONLY)) // touchable button
						{
							hEntity->Touch(pPlayer);
						}
						else
						{
							hEntity->Use(pPlayer, pPlayer, USE_SET, 1);
						}
					}
				}
				return true;
			}
			else if (FClassnameIs(hEntity->pev, "momentary_rot_button"))
			{
				if (isDragging)
				{
					hEntity->Use(pPlayer, pPlayer, USE_SET, 1);
				}
				return true;
			}
		}
		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleDoors(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (FClassnameIs(hEntity->pev, "func_door") || FClassnameIs(hEntity->pev, "func_door_rotating"))
	{
		if (didTouchChange)
		{
			if (isTouching)
			{
				if (FBitSet(hEntity->pev->spawnflags, SF_DOOR_USE_ONLY))
				{
					hEntity->Use(pPlayer, pPlayer, USE_SET, 1);
				}
				else
				{
					hEntity->Touch(pPlayer);
				}
			}
		}
		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleRechargers(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (FClassnameIs(hEntity->pev, "func_healthcharger") || FClassnameIs(hEntity->pev, "func_recharge"))
	{
		if (isTouching)
		{
			hEntity->Use(pPlayer, pPlayer, USE_SET, 1);
		}
		return true;
	}
		
	return false;
}

bool VRControllerInteractionManager::HandleTriggers(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (hEntity->pev->solid != SOLID_TRIGGER)
		return false;

	if (std::string{ STRING(hEntity->pev->classname) }.find("trigger_") == 0)
	{
		// Only handle trigger_multiple and trigger_once, other triggers should only be activated by the actual player "body"
		if (FClassnameIs(hEntity->pev, "trigger_multiple") || FClassnameIs(hEntity->pev, "trigger_once"))
		{
			if (isTouching)
			{
				hEntity->Touch(pPlayer);
			}
		}
		// Except trigger_hurt: Cause 10% damage when touching with hand
		else if (FClassnameIs(hEntity->pev, "trigger_hurt"))
		{
			if (isTouching)
			{
				float dmgBackup = hEntity->pev->dmg;
				hEntity->pev->dmg *= 0.1f;
				hEntity->Touch(pPlayer);
				hEntity->pev->dmg = dmgBackup;
			}
		}
	}
	else if (FClassnameIs(hEntity->pev, "item_sodacan"))
	{
		// TODO: pick up soda cans!
		if (isTouching)
		{
			hEntity->Touch(pPlayer);
		}
	}
	else if (std::string{ STRING(hEntity->pev->classname) }.find("item_") == 0
		|| std::string{ STRING(hEntity->pev->classname) }.find("weapon_") == 0
		|| std::string{ STRING(hEntity->pev->classname) }.find("ammo_") == 0
		|| FClassnameIs(hEntity->pev, "weaponbox"))
	{
		if (isTouching)
		{
			hEntity->Touch(pPlayer);
		}
	}
	else if (FClassnameIs(hEntity->pev, "xen_plantlight"))
	{
		if (isTouching)
		{
			hEntity->Touch(pPlayer);
		}
	}

	return true;
}

bool VRControllerInteractionManager::HandleBreakables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage)
{
	if (!FClassnameIs(hEntity->pev, "func_breakable"))
		return false;

	CBreakable *pBreakable = dynamic_cast<CBreakable*>((CBaseEntity*)hEntity);

	// Make sure breakables that are supposed to immediately break on melee attacks, do break.
	if (pPlayer->HasWeapons()
		&& pBreakable != nullptr
		&& pBreakable->IsBreakable()
		&& pBreakable->pev->takedamage != DAMAGE_NO
		&& !FBitSet(pBreakable->pev->spawnflags, SF_BREAK_TRIGGER_ONLY)
		&& FBitSet(pBreakable->pev->spawnflags, SF_BREAK_CROWBAR)
		&& isHitting && didHitChange)
	{
		pBreakable->TakeDamage(pPlayer->pev, pPlayer->pev, pBreakable->pev->health, DMG_CLUB);
	}

	return true;
}

bool VRControllerInteractionManager::HandlePushables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, const Interaction& interaction)
{
	if (!FClassnameIs(hEntity->pev, "func_pushable"))
		return false;

	CPushable *pPushable = dynamic_cast<CPushable*>((CBaseEntity*)hEntity);
	if (pPushable != nullptr)
	{
		if (interaction.touching.isSet)
		{
			// backup origins and move entities temporarily out of world, so box won't collide with itself when moving
			Vector playerOrigin = pPlayer->pev->origin;
			pPlayer->pev->origin = Vector{ 999,999,999 };
			UTIL_SetOrigin(pPlayer->pev, pPlayer->pev->origin);

			Vector pushableOrigin = pPushable->pev->origin;
			pPushable->pev->origin = Vector{ 999,999,999 };
			UTIL_SetOrigin(pPushable->pev, pPushable->pev->origin);

			Vector targetPos;
			bool isDragging = false;

			Vector controllerDragStartPos;
			Vector entityDragStartOrigin;
			if (interaction.dragging.isSet && controller.GetDraggedEntityStartPositions(hEntity, controllerDragStartPos, entityDragStartOrigin))
			{
				pPushable->pev->gravity = 0;
				targetPos = entityDragStartOrigin + controller.GetPosition() - controllerDragStartPos;
				targetPos.z = pushableOrigin.z;	// prevent lifting up, is buggy and allows for ugly cheating and exploits
				isDragging = true;
			}
			else
			{
				Vector relativePos = interaction.intersectPoint - pushableOrigin;

				// Decide axis to push on:
				Vector pushDelta;

				float normalizedX = relativePos.x;
				float normalizedY = relativePos.y;
				if (normalizedX < 0.f) normalizedX /= fabs(pPushable->pev->mins.x);
				if (normalizedX > 0.f) normalizedX /= fabs(pPushable->pev->maxs.x);
				if (normalizedY < 0.f) normalizedY /= fabs(pPushable->pev->mins.y);
				if (normalizedY > 0.f) normalizedY /= fabs(pPushable->pev->maxs.y);

				bool useAxis[3];
				useAxis[0] = fabs(normalizedX) >= 0.5f || fabs(normalizedY) <= 0.5f;
				useAxis[1] = fabs(normalizedY) >= 0.5f || fabs(normalizedX) <= 0.5f;

				// Only push up or down if floating in water
				useAxis[2] = (pPushable->pev->waterlevel > 0 && !FBitSet(pPushable->pev->flags, FL_ONGROUND));

				for (int i = 0; i < 3; i++)
				{
					if (useAxis[i])
					{
						if (relativePos[i] < 0.f)
						{
							pushDelta[i] = relativePos[i] - pPushable->pev->mins[i];
							if (pushDelta[i] < 0.f)
								pushDelta[i] = 1.f;
						}
						else
						{
							pushDelta[i] = relativePos[i] - pPushable->pev->maxs[i];
							if (pushDelta[i] > 0.f)
								pushDelta[i] = -1.f;
						}
					}
				}

				targetPos = pushableOrigin + pushDelta;
			}

			if (targetPos != pushableOrigin)
			{
				Vector moveDir = (targetPos - pushableOrigin);

				float wishspeed;
				if (isDragging)
				{
					wishspeed = moveDir.Length() / gpGlobals->frametime;		// when dragging, wishspeed is simply set to get there immediately this frame
				}
				else
				{
					constexpr const float Minspeed = 20.f;							// never go below this, or boxes won't move at all
					float targetspeed = moveDir.Length() * 10.f;					// 0.1 seconds to reach target
					float controllerspeed = controller.GetVelocity().Length();		// use controller speed, so smacking boxes with force pushes them faster

					wishspeed = (std::max)(controllerspeed, (std::max)(targetspeed, Minspeed));	// take the highest speed value
				}

				float speed = (std::min)(wishspeed, pPushable->MaxSpeed());		// cap at pushable's maxspeed

				pPushable->EmitPushSound(speed);
				pPushable->pev->velocity = moveDir.Normalize() * speed;
			}

			pPushable->pev->origin = pushableOrigin;
			UTIL_SetOrigin(pPushable->pev, pPushable->pev->origin);

			pPlayer->pev->origin = playerOrigin;
			UTIL_SetOrigin(pPlayer->pev, pPlayer->pev->origin);
		}
	}

	return true;
}

bool VRControllerInteractionManager::HandleGrabbables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, const Interaction& interaction)
{
	if (hEntity->IsDraggable())
	{
		if (interaction.dragging.isSet)
		{
			if (interaction.dragging.didChange)
			{
				hEntity->m_isBeingDragged.insert(controller.GetID());
				hEntity->SetThink(&CBaseEntity::DragStartThink);
			}
			hEntity->pev->origin = controller.GetGunPosition();
			hEntity->pev->angles = controller.GetAngles();
			hEntity->pev->velocity = controller.GetVelocity();
		}
		else
		{
			if (interaction.dragging.didChange)
			{
				hEntity->m_isBeingDragged.erase(controller.GetID());
				hEntity->SetThink(&CBaseEntity::DragStopThink);
			}
		}
		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleAlliedMonsters(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage)
{
	// Special handling of barneys and scientists that don't hate us (yet)
	CBaseMonster *pMonster = hEntity->MyMonsterPointer();
	if (pMonster != nullptr && (FClassnameIs(pMonster->pev, "monster_barney") || FClassnameIs(pMonster->pev, "monster_scientist")) && !pMonster->HasMemory(bits_MEMORY_PROVOKED))
	{
		if (isTouching)
		{
			// Face player when touched
			pMonster->MakeIdealYaw(pPlayer->pev->origin);
		}

		// Make scientists panic and barneys angry if pointing guns at them
		GetAngryIfAtGunpoint(pPlayer, pMonster, controller);

		// Don't do any following/unfollowing if we just hurt the ally somehow
		if (flHitDamage > 0.f)
		{
			pMonster->vr_flStopSignalTime = 0;
			pMonster->vr_flShoulderTouchTime = 0;
		}

		// Make scientists and barneys move away if holding weapons in their face, or we hit them
		if (isTouching && (flHitDamage > 0.f || IsWeapon(controller.GetWeaponId())))
		{
			pMonster->vr_flStopSignalTime = 0;
			pMonster->vr_flShoulderTouchTime = 0;
			pMonster->SetConditions(bits_COND_CLIENT_PUSH);
			pMonster->MakeIdealYaw(controller.GetPosition());
			pMonster->Touch(pPlayer);
		}

		// Follow/Unfollow commands
		if (flHitDamage == 0.f && !pMonster->HasMemory(bits_MEMORY_PROVOKED))
		{
			DoFollowUnfollowCommands(pPlayer, pMonster, controller, isTouching);
		}

		return true;
	}

	return false;
}

void VRControllerInteractionManager::GetAngryIfAtGunpoint(CBasePlayer *pPlayer, CBaseMonster *pMonster, const VRController& controller)
{
	Vector weaponForward;
	UTIL_MakeVectorsPrivate(pPlayer->GetWeaponViewAngles(), weaponForward, nullptr, nullptr);

	if (!pMonster->m_hEnemy
		&& controller.IsValid()
		&& IsWeaponWithRange(controller.GetWeaponId())
		&& (controller.GetPosition() - pMonster->pev->origin).Length() < VR_MAX_ANGRY_GUNPOINT_DIST
		&& UTIL_IsFacing(controller.GetPosition(), pPlayer->GetWeaponViewAngles(), pMonster->pev->origin)
		&& UTIL_IsFacing(pMonster->pev->origin, pMonster->pev->angles.ToViewAngles(), controller.GetPosition())
		&& pMonster->HasClearSight(controller.GetPosition())
		&& UTIL_CheckTraceIntersectsEntity(controller.GetPosition(), controller.GetPosition() + (weaponForward * VR_MAX_ANGRY_GUNPOINT_DIST), pMonster))
	{
		pMonster->vr_flStopSignalTime = 0;
		pMonster->vr_flShoulderTouchTime = 0;
		if (pMonster->vr_flGunPointTime == 0)
		{
			pMonster->vr_flGunPointTime = gpGlobals->time + VR_INITIAL_ANGRY_GUNPOINT_TIME;
		}
		else if (gpGlobals->time > pMonster->vr_flGunPointTime)
		{
			if (pMonster->HasMemory(bits_MEMORY_GUNPOINT) && pMonster->HasMemory(bits_MEMORY_SUSPICIOUS))
			{
				if (FClassnameIs(pMonster->pev, "monster_barney"))
				{
					pMonster->PlaySentence("BA_GNPNT_PSSD", 4, VOL_NORM, ATTN_NORM);
					pMonster->m_MonsterState = MONSTERSTATE_COMBAT;	// Make barneys draw their gun and point at player
				}
				else
				{
					pMonster->PlaySentence("SC_GNPNT_PSSD", 4, VOL_NORM, ATTN_NORM);
				}
				pMonster->StopFollowing(TRUE);

				// TODO: Add a difficulty setting to make allies go provoked from having a gun pointed at them
				// if (setting...)
				// {
				// pMonster->Remember(bits_MEMORY_PROVOKED);
				// }
			}
			else
			{
				if (pMonster->HasMemory(bits_MEMORY_GUNPOINT))
				{
					if (FClassnameIs(pMonster->pev, "monster_barney"))
					{
						pMonster->PlaySentence("BA_GNPNT_SND", 4, VOL_NORM, ATTN_NORM);
					}
					else
					{
						pMonster->PlaySentence("SC_GNPNT_SND", 4, VOL_NORM, ATTN_NORM);
					}
					pMonster->Remember(bits_MEMORY_SUSPICIOUS);
				}
				else
				{
					if (FClassnameIs(pMonster->pev, "monster_barney"))
					{
						pMonster->PlaySentence("BA_GNPNT_FRST", 4, VOL_NORM, ATTN_NORM);
					}
					else
					{
						pMonster->PlaySentence("SC_GNPNT_FRST", 4, VOL_NORM, ATTN_NORM);
					}
					pMonster->Remember(bits_MEMORY_GUNPOINT);
				}
			}
			pMonster->vr_flGunPointTime = gpGlobals->time + RANDOM_FLOAT(VR_MIN_ANGRY_GUNPOINT_TIME, VR_MAX_ANGRY_GUNPOINT_TIME);
		}
	}
	else
	{
		if (!pMonster->m_hEnemy && pMonster->HasMemory(bits_MEMORY_GUNPOINT) && FClassnameIs(pMonster->pev, "monster_barney"))
		{
			// TODO: Make barney put away his gun
			pMonster->m_MonsterState = MONSTERSTATE_ALERT;
		}
		pMonster->vr_flGunPointTime = 0;
		pMonster->Forget(bits_MEMORY_GUNPOINT);
	}
}

void VRControllerInteractionManager::DoFollowUnfollowCommands(CBasePlayer *pPlayer, CBaseMonster *pMonster, const VRController& controller, bool isTouching)
{
	// Check if we're touching the shoulder for 1 second (follow me!)
	if (isTouching)
	{
		pMonster->vr_flStopSignalTime = 0;
		if (!IsWeapon(controller.GetWeaponId()) && (controller.IsDragging() || CheckShoulderTouch(pMonster, controller.GetPosition())))
		{
			pMonster->ClearConditions(bits_COND_CLIENT_PUSH);
			if (dynamic_cast<CTalkMonster*>(pMonster)->CanFollow()) // Ignore if can't follow
			{
				if (pMonster->vr_flShoulderTouchTime == 0)
				{
					pMonster->vr_flShoulderTouchTime = gpGlobals->time;
				}
				else if ((gpGlobals->time - pMonster->vr_flShoulderTouchTime) > VR_SHOULDER_TOUCH_TIME)
				{
					dynamic_cast<CTalkMonster*>(pMonster)->FollowerUse(pPlayer, pPlayer, USE_ON, 1.f);
					pMonster->vr_flShoulderTouchTime = 0;
				}
			}
			else
			{
				pMonster->vr_flShoulderTouchTime = 0;
			}
		}
		// Otherwise just look at player
		else
		{
			pMonster->vr_flStopSignalTime = 0;
			pMonster->vr_flShoulderTouchTime = 0;
			pMonster->MakeIdealYaw(pPlayer->pev->origin);
		}
	}
	// Tell allies to stop following by holding hand in front of their face
	else if (pMonster->IsFollowing() && !IsWeapon(controller.GetWeaponId()) &&CheckStopSignal(pMonster, controller.GetPosition(), controller.GetAngles()))
	{
		pMonster->vr_flShoulderTouchTime = 0;
		if (pMonster->vr_flStopSignalTime == 0)
		{
			pMonster->vr_flStopSignalTime = gpGlobals->time;
		}
		else if ((gpGlobals->time - pMonster->vr_flStopSignalTime) > VR_STOP_SIGNAL_TIME)
		{
			dynamic_cast<CTalkMonster*>(pMonster)->StopFollowing(TRUE);
			pMonster->vr_flStopSignalTime = 0;
		}
	}
	else
	{
		pMonster->vr_flStopSignalTime = 0;
		pMonster->vr_flShoulderTouchTime = 0;
	}
}
