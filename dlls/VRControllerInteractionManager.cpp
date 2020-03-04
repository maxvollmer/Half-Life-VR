
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
#include "pm_defs.h"

#include "VRControllerInteractionManager.h"
#include "VRPhysicsHelper.h"
#include "CWorldsSmallestCup.h"
#include "VRModelHelper.h"
#include "VRMovementHandler.h"
#include "VRControllerModel.h"

#include "VRRotatableEnt.h"


#ifdef RENDER_DEBUG_BBOXES
#include "VRDebugBBoxDrawer.h"
static VRDebugBBoxDrawer g_VRDebugBBoxDrawer;
#endif


#define SF_BUTTON_TOUCH_ONLY 256  // button only fires as a result of USE key.
#define SF_DOOR_PASSABLE     8
#define SF_DOOR_USE_ONLY     256  // door must be opened by player's use button.

#define bits_COND_CLIENT_PUSH (bits_COND_SPECIAL1)

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
std::unordered_map<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> g_vrRetinaScanners;
std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> g_vrRetinaScannerButtons;

EHANDLE<CBaseEntity> g_vrHRetinaScanner;
float g_vrRetinaScannerLookTime = 0.f;
bool g_vrRetinaScannerUsed = false;


bool CheckShoulderTouch(CBaseMonster* pMonster, const Vector& pos)
{
	Vector posInMonsterLocalSpace = pos - pMonster->pev->origin;
	if (posInMonsterLocalSpace.z >= (VR_SHOULDER_TOUCH_MIN_Z * pMonster->pev->scale) && posInMonsterLocalSpace.z <= (VR_SHOULDER_TOUCH_MAX_Z * pMonster->pev->scale))
	{
		float angle = -pMonster->pev->angles.y * M_PI / 180.f;
		//float localX = cos(angle)*posInMonsterLocalSpace.x - sin(angle)*posInMonsterLocalSpace.y;
		float localY = sin(angle) * posInMonsterLocalSpace.x + cos(angle) * posInMonsterLocalSpace.y;
		if (fabs(localY) >= (VR_SHOULDER_TOUCH_MIN_Y * pMonster->pev->scale) && fabs(localY) <= (VR_SHOULDER_TOUCH_MAX_Y * pMonster->pev->scale))
		{
			return true;
		}
	}
	return false;
}

bool CheckStopSignal(CBaseMonster* pMonster, const Vector& pos, const Vector& angles)
{
	Vector posInMonsterLocalSpace = pos - pMonster->pev->origin;
	if (posInMonsterLocalSpace.z >= (VR_STOP_SIGNAL_MIN_Z * pMonster->pev->scale) && posInMonsterLocalSpace.z <= (VR_STOP_SIGNAL_MAX_Z * pMonster->pev->scale))
	{
		float angle = -pMonster->pev->angles.y * M_PI / 180.f;
		float localX = cos(angle) * posInMonsterLocalSpace.x - sin(angle) * posInMonsterLocalSpace.y;
		float localY = sin(angle) * posInMonsterLocalSpace.x + cos(angle) * posInMonsterLocalSpace.y;
		if (localX >= (VR_STOP_SIGNAL_MIN_X * pMonster->pev->scale) && localX <= (VR_STOP_SIGNAL_MAX_X * pMonster->pev->scale) && localY >= (VR_STOP_SIGNAL_MIN_Y * pMonster->pev->scale) && localY <= (VR_STOP_SIGNAL_MAX_Y * pMonster->pev->scale))
		{
			// Stop signal is given when hand is pointing up and back of hand is facing away from ally
			Vector forward;
			Vector up;
			Vector monsterForward;
			UTIL_MakeAimVectorsPrivate(angles, forward, up, nullptr);
			UTIL_MakeAimVectorsPrivate(pMonster->pev->angles, monsterForward, nullptr, nullptr);
			bool isHandFacingUp = DotProduct(forward, Vector(0, 0, 1)) > 0.9f;
			bool isBackOfHandFacingAway = true;  // DotProduct(up, monsterForward) > 0.9f;	// TODO: Buggy
			return isHandFacingUp && isBackOfHandFacingAway;
		}
	}
	return false;
}

bool WasJustThrownByPlayer(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity)
{
	if (hEntity->pev->owner != pPlayer->edict())
		return false;

	if (!FClassnameIs(hEntity->pev, "monster_satchel") && !FClassnameIs(hEntity->pev, "grenade") && !FClassnameIs(hEntity->pev, "monster_snark"))
		return false;

	return (gpGlobals->time - hEntity->m_spawnTime) < 2.f;
}

bool IsUsableDoor(EHANDLE<CBaseEntity> hEntity)
{
	return (FClassnameIs(hEntity->pev, "func_door") || FClassnameIs(hEntity->pev, "func_door_rotating")) && FBitSet(hEntity->pev->spawnflags, SF_DOOR_USE_ONLY);
}

bool IsNonInteractingEntity(EHANDLE<CBaseEntity> hEntity)
{
	return (hEntity->pev->solid == SOLID_NOT && !IsUsableDoor(hEntity)) || FClassnameIs(hEntity->pev, "func_wall") || FClassnameIs(hEntity->pev, "func_illusionary") || FClassnameIs(hEntity->pev, "vr_controllermodel");  // TODO/NOTE: If this mod gets ever patched up for multiplayer, and you want players to be able to crowbar-fight, this should probably be changed
}

bool IsReachable(CBasePlayer* pPlayer, const StudioHitBox& hitbox, bool strict)
{
	TraceResult tr{ 0 };
	UTIL_TraceLine(pPlayer->pev->origin, hitbox.origin, ignore_monsters, nullptr, &tr);
	if (tr.flFraction < 1.f)
		return false;

	if (!strict)
		return true;

	return !VRPhysicsHelper::Instance().CheckIfLineIsBlocked(pPlayer->pev->origin, hitbox.origin);
}

bool IsTriggerOrButton(EHANDLE<CBaseEntity> hEntity)
{
	if (!hEntity)
		return false;

	// usables (doors, buttons, levers etc)
	if (hEntity->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE))
		return true;

	// triggers
	if (hEntity->pev->solid == SOLID_TRIGGER && hEntity->pev->model && STRING(hEntity->pev->model)[0] == '*')
		return true;

	return false;
}

bool CheckIfEntityAndHitboxesTouch(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const std::vector<StudioHitBox>& hitboxes, VRPhysicsHelperModelBBoxIntersectResult* intersectResult)
{
	// Check each hitbox of current weapon
	for (auto hitbox : hitboxes)
	{
		// Prevent interaction with stuff through walls
		// (simple check for non-important stuff like ammo, strict check using physics engine for important things like triggers, NPCs and buttons)
		if (!IsReachable(pPlayer, hitbox, IsTriggerOrButton(hEntity)))
			continue;

		if (VRPhysicsHelper::Instance().ModelIntersectsBBox(hEntity, hitbox.origin, hitbox.mins, hitbox.maxs, hitbox.angles, intersectResult)
			// TODO: Use VRPhysicsHelper to check if point is inside entity with proper raytracing
			|| UTIL_IsPointInEntity(hEntity, hitbox.origin))
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

bool VRControllerInteractionManager::CheckIfEntityAndControllerTouch(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, VRPhysicsHelperModelBBoxIntersectResult* intersectResult)
{
	if (!controller.IsValid())
		return false;

	if (controller.GetRadius() <= 0.f)
		return false;

	if (IsNonInteractingEntity(hEntity) && !IsDraggableEntity(hEntity))
		return false;

	// Don't hit stuff we just threw ourselfes
	if (WasJustThrownByPlayer(pPlayer, hEntity))
		return false;

	// Rule out entities too far away
	CBaseEntity* pWorld = CBaseEntity::InstanceOrWorld(INDEXENT(0));
	if (hEntity != pWorld)
	{
		float entityRadius = (std::max)(hEntity->pev->mins.Length(), hEntity->pev->maxs.Length());
		if (entityRadius == 0.f)
		{
			// assume this is a gib and use sensible radius
			entityRadius = 16.f;
		}
		Vector entityCenter = hEntity->pev->origin + (hEntity->pev->maxs + hEntity->pev->mins) * 0.5f;
		float distance = (controller.GetPosition() - entityCenter).Length();
		if (distance > (controller.GetRadius() + entityRadius))
		{
			return false;
		}
	}

#ifdef RENDER_DEBUG_BBOXES
	// draw bboxes of entities in close proximity of controller
	g_VRDebugBBoxDrawer.DrawBBoxes(hEntity);
#endif

	// first check controller hitboxes
	if (CheckIfEntityAndHitboxesTouch(pPlayer, hEntity, controller.GetHitBoxes(), intersectResult))
	{
		return true;
	}

	// then check hitboxes of dragged entity (if any)
	if (controller.HasDraggedEntity())
	{
		EHANDLE<CBaseEntity> hDraggedEntity = controller.GetDraggedEntity();
		if (IsDraggableEntityThatCanTouchStuff(hDraggedEntity))
		{
			void* pmodel = GET_MODEL_PTR(hDraggedEntity->edict());
			if (pmodel)
			{
				int numhitboxes = GetNumHitboxes(pmodel);
				if (numhitboxes > 0)
				{
					std::vector<StudioHitBox> draggedentityhitboxes;
					draggedentityhitboxes.resize(numhitboxes);
					if (GetHitboxesAndAttachments(hDraggedEntity->pev, pmodel, hDraggedEntity->pev->sequence, hDraggedEntity->pev->frame, draggedentityhitboxes.data(), nullptr))
					{
						if (CheckIfEntityAndHitboxesTouch(pPlayer, hEntity, draggedentityhitboxes, intersectResult))
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool VRControllerInteractionManager::IsDraggableEntity(EHANDLE<CBaseEntity> hEntity)
{
	if (!hEntity)
		return false;

	return FClassnameIs(hEntity->pev, "vr_easteregg")
		|| FClassnameIs(hEntity->pev, "func_rot_button")
		|| FClassnameIs(hEntity->pev, "momentary_rot_button")
		|| (FClassnameIs(hEntity->pev, "func_door_rotating") && FBitSet(hEntity->pev->spawnflags, SF_DOOR_USE_ONLY))
		|| FClassnameIs(hEntity->pev, "func_pushable")
		|| (CVAR_GET_FLOAT("vr_ladder_immersive_movement_enabled") != 0.f && FClassnameIs(hEntity->pev, "func_ladder"))
		|| hEntity->IsDraggable();
}

bool VRControllerInteractionManager::IsDraggableEntityThatCanTouchStuff(EHANDLE<CBaseEntity> hEntity)
{
	if (!hEntity)
		return false;

	if (FClassnameIs(hEntity->pev, "func_rot_button")
		|| FClassnameIs(hEntity->pev, "momentary_rot_button")
		|| FClassnameIs(hEntity->pev, "func_door_rotating")
		|| FClassnameIs(hEntity->pev, "func_pushable")
		|| FClassnameIs(hEntity->pev, "func_ladder"))
		return false;

	if (hEntity->IsBSPModel() || !hEntity->pev->model || STRING(hEntity->pev->model)[0] == '*')
		return false;

	return FClassnameIs(hEntity->pev, "vr_easteregg") || hEntity->IsDraggable();
}

constexpr const int VR_DRAG_DISTANCE_TOLERANCE = 64;

bool DistanceTooBigForDragging(EHANDLE<CBaseEntity> hEntity, const VRController& controller)
{
	Vector entityCenter = hEntity->pev->origin + (hEntity->pev->maxs + hEntity->pev->mins) * 0.5f;
	float distance = (controller.GetPosition() - entityCenter).Length();
	float entityRadius = (std::max)(hEntity->pev->mins.Length(), hEntity->pev->maxs.Length());
	return distance > (controller.GetRadius() + entityRadius + VR_DRAG_DISTANCE_TOLERANCE);
}

void VRControllerInteractionManager::CheckAndPressButtons(CBasePlayer* pPlayer, VRController& controller)
{
	CBaseEntity* pEntity = nullptr;
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

		EHANDLE<CBaseEntity> hEntity = pEntity;

		VRPhysicsHelperModelBBoxIntersectResult intersectResult;
		bool isTouching;
		bool didTouchChange;
		bool isDragging;
		bool didDragChange;
		bool isHitting;
		bool didHitChange;

		// If we are dragging something draggable, we override all the booleans to avoid "losing" the entity due to fast movements
		if (controller.IsDragging() && controller.IsDraggedEntity(hEntity) && IsDraggableEntity(hEntity) && !DistanceTooBigForDragging(hEntity, controller))
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

			isDragging = isTouching && controller.IsDragging();
			if (isDragging)
			{
				didDragChange = !controller.IsDraggedEntity(hEntity);
			}
			else
			{
				didDragChange = controller.RemoveDraggedEntity(hEntity);
			}

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

		float flHitDamage = 0.f;
		if (pPlayer->HasWeapons() && isHitting && didHitChange)
		{
			flHitDamage = DoDamage(pPlayer, hEntity, controller, intersectResult);
		}

		// vibrate if touching something solid (but not if we are dragging it!)
		if (isTouching && hEntity->m_vrDragger != pPlayer && hEntity->m_vrDragController != controller.GetID() && !controller.IsDraggedEntity(hEntity))
		{
			if ((hEntity->pev->solid != SOLID_NOT && hEntity->pev->solid != SOLID_TRIGGER)
				|| (hEntity->pev->solid == SOLID_TRIGGER && (hEntity->pev->movetype == MOVETYPE_TOSS || hEntity->pev->movetype == MOVETYPE_BOUNCE))
				|| hEntity->IsDraggable())
			{
				if (isHitting)
				{
					if (hEntity->pev->solid == SOLID_BSP)
					{
						controller.AddTouch(VRController::TouchType::HARD_TOUCH, 0.1f);
					}
					else
					{
						controller.AddTouch(VRController::TouchType::NORMAL_TOUCH, 0.1f);
					}
				}
				else
				{
					if (hEntity->pev->solid == SOLID_BSP)
					{
						controller.AddTouch(VRController::TouchType::NORMAL_TOUCH, 0.1f);
					}
					else
					{
						controller.AddTouch(VRController::TouchType::LIGHT_TOUCH, 0.1f);
					}
				}
			}
		}

		Interaction::InteractionInfo touching{ isTouching, didTouchChange };
		Interaction::InteractionInfo dragging{ isDragging, didDragChange };
		Interaction::InteractionInfo hitting{ isHitting, didHitChange };
		Interaction interaction{ touching, dragging, hitting, flHitDamage, intersectResult.hasresult ? intersectResult.hitpoint : controller.GetPosition() };

		if (HandleEasterEgg(pPlayer, hEntity, controller, interaction))
			;  // easter egg first, obviously the most important
		else if (HandleRetinaScanners(pPlayer, hEntity, controller, isTouching, didTouchChange))
			;
		else if (HandleButtonsAndDoors(pPlayer, hEntity, controller, interaction))
			;
		else if (HandleRechargers(pPlayer, hEntity, controller, isTouching, didTouchChange))
			;
		else if (HandleTriggers(pPlayer, hEntity, controller, isTouching, didTouchChange))
			;
		else if (HandleBreakables(pPlayer, hEntity, controller, isTouching, didTouchChange, isHitting, didHitChange, flHitDamage))
			;
		else if (HandlePushables(pPlayer, hEntity, controller, interaction))
			;
		else if (HandleGrabbables(pPlayer, hEntity, controller, interaction))
			;
		else if (HandleLadders(pPlayer, hEntity, controller, interaction))
			;
		else if (HandleAlliedMonsters(pPlayer, hEntity, controller, isTouching, didTouchChange, isHitting, didHitChange, flHitDamage))
			;
		else if (HandleTossables(pPlayer, hEntity, controller, interaction))
			;
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

float VRControllerInteractionManager::DoDamage(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const VRPhysicsHelperModelBBoxIntersectResult& intersectResult)
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

		EHANDLE<CBasePlayerWeapon> hWeapon = pPlayer->m_pActiveItem;
		if (hWeapon)
		{
			// If you smack something with an explosive, it might just explode...
			// 25% chance that charged satchels go off when you smack something with the remote
			if (controller.GetWeaponId() == WEAPON_SATCHEL && pPlayer->m_pActiveItem->m_chargeReady && RANDOM_LONG(0, 4) == 0)
			{
				hWeapon->PrimaryAttack();
			}
			// 5% chance that an explosive explodes when hitting something
			else if (IsExplosiveWeapon(controller.GetWeaponId()) && RANDOM_LONG(0, 20) == 0 && pPlayer->m_rgAmmo[pPlayer->m_pActiveItem->PrimaryAmmoIndex()] > 0)
			{
				if (controller.GetWeaponId() == WEAPON_SNARK)
				{
					EHANDLE<CSqueak> hSqueak = hWeapon;
					if (hSqueak)
						hSqueak->m_fJustThrown = 1;
					pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
					CBaseEntity* pSqueak = CBaseEntity::Create<CBaseEntity>("monster_snark", controller.GetPosition(), controller.GetAngles(), pPlayer->edict());
					pSqueak->pev->health = -1;
					pSqueak->Killed(pPlayer->pev, DMG_CRUSH, GIB_ALWAYS);
					pSqueak = nullptr;
				}
				else
				{
					ExplosionCreate(controller.GetPosition(), controller.GetAngles(), pPlayer->edict(), pPlayer->m_pActiveItem->pev->dmg, TRUE);
				}

				pPlayer->m_rgAmmo[pPlayer->m_pActiveItem->PrimaryAmmoIndex()]--;
				hWeapon->m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;

				if (pPlayer->m_rgAmmo[pPlayer->m_pActiveItem->PrimaryAmmoIndex()] > 0)
				{
					hWeapon->Deploy();
				}
				else
				{
					hWeapon->RetireWeapon();
				}
			}
		}
	}

	return damage;
}

bool VRControllerInteractionManager::HandleEasterEgg(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction)
{
	if (FClassnameIs(hEntity->pev, "vr_easteregg"))
	{
		EHANDLE<CWorldsSmallestCup> pWorldsSmallestCup = hEntity;
		if (pWorldsSmallestCup)
		{
			if (interaction.dragging.isSet)
			{
				if (interaction.dragging.didChange)
				{
					if (controller.SetDraggedEntity(hEntity))
					{
						hEntity->m_vrDragger = pPlayer;
						hEntity->m_vrDragController = controller.GetID();
					}
				}
				// If we are the same player and controller that last started dragging the entity, update drag
				if (controller.IsDraggedEntity(hEntity) && hEntity->m_vrDragger == pPlayer && hEntity->m_vrDragController == controller.GetID())
				{
					pWorldsSmallestCup->pev->origin = controller.GetGunPosition();
					pWorldsSmallestCup->pev->angles = controller.GetAngles();
					pWorldsSmallestCup->pev->velocity = controller.GetVelocity();
				}
			}
			else
			{
				if (interaction.dragging.didChange)
				{
					// If we are the same player and controller that last started dragging the entity, stop dragging
					if (hEntity->m_vrDragger == pPlayer && hEntity->m_vrDragController == controller.GetID())
					{
						hEntity->m_vrDragger = nullptr;
						hEntity->m_vrDragController = VRControllerID::INVALID;
					}
				}
			}
		}
		return true;
	}
	return false;
}

bool VRControllerInteractionManager::HandleRetinaScanners(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (g_vrRetinaScanners.count(hEntity) > 0)
	{
		Vector retinaScannerPosition = (hEntity->pev->absmax + hEntity->pev->absmin) / 2.f;
		bool isLookingAtRetinaScanner = UTIL_IsFacing(pPlayer->pev->origin, pPlayer->pev->angles.ToViewAngles(), retinaScannerPosition) && ((pPlayer->EyePosition() - retinaScannerPosition).Length() < 32.f) && (pPlayer->EyePosition().z >= hEntity->pev->absmin.z) && (pPlayer->EyePosition().z <= hEntity->pev->absmax.z);

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

bool VRControllerInteractionManager::HandleButtonsAndDoors(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction)
{
	// Don't touch activate retina scanners
	if (g_vrRetinaScannerButtons.count(hEntity) > 0)
		return true;

	if (FClassnameIs(hEntity->pev, "func_door") || (FClassnameIs(hEntity->pev, "func_door_rotating") && !FBitSet(hEntity->pev->spawnflags, SF_DOOR_USE_ONLY)))
	{
		if (interaction.touching.didChange)
		{
			if (interaction.touching.isSet)
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
	else if (FClassnameIs(hEntity->pev, "func_button"))
	{
		if (interaction.touching.isSet)
		{
			if (interaction.touching.didChange || FBitSet(hEntity->ObjectCaps(), FCAP_CONTINUOUS_USE))
			{
				if (FBitSet(hEntity->pev->spawnflags, SF_BUTTON_TOUCH_ONLY))  // touchable button
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
	else if (FClassnameIs(hEntity->pev, "func_rot_button") || FClassnameIs(hEntity->pev, "momentary_rot_button") || (FClassnameIs(hEntity->pev, "func_door_rotating") && FBitSet(hEntity->pev->spawnflags, SF_DOOR_USE_ONLY)))
	{
		VRRotatableEnt* pRotatableEnt = hEntity->MyRotatableEntPtr();
		if (pRotatableEnt)
		{
			if (interaction.dragging.isSet)
			{
				if (interaction.dragging.didChange)
				{
					if (controller.SetDraggedEntity(hEntity))
					{
						pRotatableEnt->ClearDraggingCancelled();
					}
				}
				if (controller.IsDraggedEntity(hEntity))
				{
					if (!pRotatableEnt->IsDraggingCancelled())
					{
						Vector pos;
						if (controller.GetAttachment(VR_MUZZLE_ATTACHMENT, pos))
						{
							pRotatableEnt->VRRotate(pPlayer, pos, interaction.dragging.didChange);
						}
						else
						{
							pRotatableEnt->VRRotate(pPlayer, controller.GetPosition(), interaction.dragging.didChange);
						}
					}
				}
			}
			else if (interaction.dragging.didChange)
			{
				pRotatableEnt->VRStopRotate();
			}
		}
		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleRechargers(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
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

bool VRControllerInteractionManager::HandleTriggers(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
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
	else if (std::string{ STRING(hEntity->pev->classname) }.find("item_") == 0 || std::string{ STRING(hEntity->pev->classname) }.find("weapon_") == 0 || std::string{ STRING(hEntity->pev->classname) }.find("ammo_") == 0 || FClassnameIs(hEntity->pev, "weaponbox"))
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

bool VRControllerInteractionManager::HandleBreakables(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage)
{
	if (!FClassnameIs(hEntity->pev, "func_breakable"))
		return false;

	EHANDLE<CBreakable> pBreakable = hEntity;

	// Make sure breakables that are supposed to immediately break on melee attacks, do break.
	if (pPlayer->HasWeapons() && pBreakable != nullptr && pBreakable->IsBreakable() && pBreakable->pev->takedamage != DAMAGE_NO && !FBitSet(pBreakable->pev->spawnflags, SF_BREAK_TRIGGER_ONLY) && FBitSet(pBreakable->pev->spawnflags, SF_BREAK_CROWBAR) && isHitting && didHitChange)
	{
		pBreakable->TakeDamage(pPlayer->pev, pPlayer->pev, pBreakable->pev->health, DMG_CLUB);
	}

	return true;
}

bool VRControllerInteractionManager::HandlePushables(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction)
{
	if (!FClassnameIs(hEntity->pev, "func_pushable"))
		return false;

	EHANDLE<CPushable> pPushable = hEntity;
	if (pPushable != nullptr)
	{
		if (interaction.touching.isSet)
		{
			bool onlyZ = false;

			// Is player standing on this pushable?
			if (FBitSet(pPlayer->pev->flags, FL_ONGROUND) && pPlayer->pev->groundentity && VARS(pPlayer->pev->groundentity) == hEntity->pev)
			{
				// Allow vertical push if floating
				if (hEntity->pev->waterlevel > 0)
				{
					onlyZ = true;
				}
				else
				{
					return true;
				}
			}

			// backup origins and move entities temporarily out of world, so box won't collide with itself when moving
			Vector playerOrigin = pPlayer->pev->origin;
			pPlayer->pev->origin = Vector{ 9999, 9999, 9999 };
			UTIL_SetOrigin(pPlayer->pev, pPlayer->pev->origin);

			Vector pushableOrigin = pPushable->pev->origin;
			pPushable->pev->origin = Vector{ 9999, 9999, 9999 };
			UTIL_SetOrigin(pPushable->pev, pPushable->pev->origin);

			Vector targetPos;
			bool isDragging = false;

			Vector controllerDragStartPos;
			Vector entityDragStartOrigin;
			Vector dummy;

			if (!onlyZ && interaction.dragging.isSet && controller.SetDraggedEntity(hEntity) && controller.GetDraggedEntityPositions(hEntity, dummy, controllerDragStartPos, entityDragStartOrigin, dummy, dummy, dummy, dummy, dummy))
			{
				pPushable->pev->gravity = 0;
				targetPos = entityDragStartOrigin + controller.GetPosition() - controllerDragStartPos;
				targetPos.z = pushableOrigin.z;  // prevent lifting up, is buggy and allows for ugly cheating and exploits
				isDragging = true;
			}
			else
			{
				Vector pushableCenter = pushableOrigin + (pPushable->pev->mins + pPushable->pev->maxs) * 0.5f;
				Vector relativePos = interaction.intersectPoint - pushableCenter;

				Vector size = pPushable->pev->maxs - pPushable->pev->mins;
				Vector extends = size * 0.5f;

				float normalizedX = relativePos.x / extends.x;
				float normalizedY = relativePos.y / extends.y;

				// Decide axis to push on:
				bool useAxis[3];
				useAxis[0] = !onlyZ && (fabs(normalizedX) >= 0.5f || fabs(normalizedY) <= 0.5f);
				useAxis[1] = !onlyZ && (fabs(normalizedY) >= 0.5f || fabs(normalizedX) <= 0.5f);

				// Only push up or down if floating in water
				useAxis[2] = onlyZ || (pPushable->pev->waterlevel > 0);

				Vector pushDelta;
				for (int i = 0; i < 3; i++)
				{
					if (useAxis[i])
					{
						if (relativePos[i] < 0.f)
						{
							pushDelta[i] = relativePos[i] + extends[i];
							if (pushDelta[i] < 0.f)
								pushDelta[i] = 1.f;
						}
						else
						{
							pushDelta[i] = relativePos[i] - extends[i];
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

				if (onlyZ)
				{
					if (moveDir.z < EPSILON)
						return true;

					moveDir.x = 0.f;
					moveDir.y = 0.f;
				}

				float wishspeed = 0.f;
				if (isDragging)
				{
					wishspeed = moveDir.Length() / gpGlobals->frametime;  // when dragging, wishspeed is simply set to get there immediately this frame
				}
				else
				{
					constexpr const float Minspeed = 20.f;                               // never go below this, or boxes won't move at all
					float targetspeed = moveDir.Length() * 10.f;            // 0.1 seconds to reach target
					float controllerspeed = controller.GetVelocity().Length();  // use controller speed, so smacking boxes with force pushes them faster

					wishspeed = (std::max)(controllerspeed, (std::max)(targetspeed, Minspeed));  // take the highest speed value
				}

				float speed = (std::min)(wishspeed, pPushable->MaxSpeed());  // cap at pushable's maxspeed

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

bool VRControllerInteractionManager::HandleGrabbables(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction)
{
	if (hEntity->IsDraggable())
	{
		if (interaction.dragging.isSet)
		{
			if (interaction.dragging.didChange)
			{
				if (controller.SetDraggedEntity(hEntity))
				{
					hEntity->m_vrDragger = pPlayer;
					hEntity->m_vrDragController = controller.GetID();
					hEntity->SetThink(&CBaseEntity::DragStartThink);
					hEntity->pev->nextthink = gpGlobals->time;
				}
			}
			else
			{
				// If we are the same player and controller that last started dragging the entity, update drag
				if (controller.IsDraggedEntity(hEntity) && hEntity->m_vrDragger == pPlayer && hEntity->m_vrDragController == controller.GetID())
				{
					hEntity->SetThink(&CBaseEntity::DragThink);
					hEntity->pev->nextthink = gpGlobals->time;
				}
			}
		}
		else
		{
			if (interaction.dragging.didChange)
			{
				// If we are the same player and controller that last started dragging the entity, stop dragging
				if (hEntity->m_vrDragger == pPlayer && hEntity->m_vrDragController == controller.GetID())
				{
					hEntity->m_vrDragger = nullptr;
					hEntity->m_vrDragController = VRControllerID::INVALID;
					hEntity->SetThink(&CBaseEntity::DragStopThink);
					hEntity->pev->nextthink = gpGlobals->time;
				}
			}
		}

		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleLadders(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction)
{
	if (FClassnameIs(hEntity->pev, "func_ladder") && CVAR_GET_FLOAT("vr_ladder_immersive_movement_enabled") != 0.f)
	{
		if (interaction.dragging.didChange)
		{
			if (interaction.dragging.isSet)
			{
				if (controller.SetDraggedEntity(hEntity))
				{
					pPlayer->SetLadderGrabbingController(controller.GetID(), hEntity);
				}
			}
			else
			{
				pPlayer->ClearLadderGrabbingController(controller.GetID());
				if (CVAR_GET_FLOAT("vr_ladder_immersive_movement_swinging_enabled") != 0.f)
				{
					// we let go off the ladder, let's take the controller velocity to push us a bit (allows for swinging)
					if (pPlayer->GetGrabbedLadderEntIndex() == 0)
					{
						pPlayer->pev->velocity = -controller.GetVelocity();
					}
				}
			}
		}
		else
		{
			if (interaction.dragging.isSet && controller.IsDraggedEntity(hEntity) && pPlayer->IsLadderGrabbingController(controller.GetID(), hEntity))
			{
				Vector controllerDragStartOffset;
				Vector playerDragStartOrigin;
				Vector dummy;
				if (controller.GetDraggedEntityPositions(hEntity,
					controllerDragStartOffset,
					dummy,
					dummy,
					playerDragStartOrigin,
					dummy,
					dummy,
					dummy,
					dummy))
				{
					// get new position from controller movement
					Vector targetPos = playerDragStartOrigin + controllerDragStartOffset - controller.GetOffset();

					// prevent moving into or away from ladder (nausea!)
					if (fabs(hEntity->pev->size.x) > fabs(hEntity->pev->size.y))
						targetPos.y = pPlayer->pev->origin.y;
					else
						targetPos.x = pPlayer->pev->origin.x;

					// move!
					pPlayer->pev->origin = VRMovementHandler::DoMovement(pPlayer->pev->origin, targetPos);
					UTIL_SetOrigin(pPlayer->pev, pPlayer->pev->origin);
				}
			}
		}
		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleTossables(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction)
{
	if (interaction.touching.isSet
		&& (hEntity->pev->movetype == MOVETYPE_TOSS || hEntity->pev->movetype == MOVETYPE_BOUNCE)
		&& hEntity->pev->solid == SOLID_BBOX)
	{
		if (interaction.touching.didChange)
		{
			hEntity->Touch(pPlayer);
		}
		Vector intersection = hEntity->pev->origin - interaction.intersectPoint;
		Vector pushAwayVelocity = controller.GetVelocity() + (intersection * 0.5f);
		if (pushAwayVelocity.Length() > 100)
		{
			pushAwayVelocity = pushAwayVelocity.Normalize() * 100;
		}
		hEntity->pev->velocity = pushAwayVelocity;
		return true;
	}
	return false;
}

bool VRControllerInteractionManager::HandleAlliedMonsters(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage)
{
	// Special handling of barneys and scientists that don't hate us (yet)
	EHANDLE<CBaseMonster> hMonster = hEntity;
	if (hMonster && (FClassnameIs(hMonster->pev, "monster_barney") || FClassnameIs(hMonster->pev, "monster_scientist")) && !hMonster->HasMemory(bits_MEMORY_PROVOKED))
	{
		if (isTouching)
		{
			// Face player when touched
			hMonster->MakeIdealYaw(pPlayer->pev->origin);
		}

		// Make scientists panic and barneys angry if pointing guns at them
		GetAngryIfAtGunpoint(pPlayer, hMonster, controller);

		// Don't do any following/unfollowing if we just hurt the ally somehow
		if (flHitDamage > 0.f)
		{
			hMonster->vr_flStopSignalTime = 0;
			hMonster->vr_flShoulderTouchTime = 0;
		}

		// Make scientists and barneys move away if holding weapons in their face, or we hit them
		if (isTouching && (flHitDamage > 0.f || IsWeapon(controller.GetWeaponId())))
		{
			hMonster->vr_flStopSignalTime = 0;
			hMonster->vr_flShoulderTouchTime = 0;
			hMonster->SetConditions(bits_COND_CLIENT_PUSH);
			hMonster->MakeIdealYaw(controller.GetPosition());
			hMonster->Touch(pPlayer);
		}

		// Follow/Unfollow commands
		if (flHitDamage == 0.f && !hMonster->HasMemory(bits_MEMORY_PROVOKED))
		{
			DoFollowUnfollowCommands(pPlayer, hMonster, controller, isTouching);
		}

		return true;
	}

	return false;
}

void VRControllerInteractionManager::GetAngryIfAtGunpoint(CBasePlayer* pPlayer, CBaseMonster* pMonster, const VRController& controller)
{
	Vector weaponForward;
	UTIL_MakeVectorsPrivate(pPlayer->GetWeaponViewAngles(), weaponForward, nullptr, nullptr);

	if (!pMonster->m_hEnemy && controller.IsValid() && IsWeaponWithRange(controller.GetWeaponId()) && (controller.GetPosition() - pMonster->pev->origin).Length() < VR_MAX_ANGRY_GUNPOINT_DIST && UTIL_IsFacing(controller.GetPosition(), pPlayer->GetWeaponViewAngles(), pMonster->pev->origin) && UTIL_IsFacing(pMonster->pev->origin, pMonster->pev->angles.ToViewAngles(), controller.GetPosition()) && pMonster->HasClearSight(controller.GetPosition()) && UTIL_CheckTraceIntersectsEntity(controller.GetPosition(), controller.GetPosition() + (weaponForward * VR_MAX_ANGRY_GUNPOINT_DIST), pMonster))
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
					pMonster->m_MonsterState = MONSTERSTATE_COMBAT;  // Make barneys draw their gun and point at player
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

void VRControllerInteractionManager::DoFollowUnfollowCommands(CBasePlayer* pPlayer, CBaseMonster* pMonster, const VRController& controller, bool isTouching)
{
	// Check if we're touching the shoulder for 1 second (follow me!)
	if (isTouching)
	{
		pMonster->vr_flStopSignalTime = 0;
		if (!IsWeapon(controller.GetWeaponId()) && ((controller.IsDragging() && !controller.HasDraggedEntity()) || CheckShoulderTouch(pMonster, controller.GetPosition())))
		{
			pMonster->ClearConditions(bits_COND_CLIENT_PUSH);
			if (dynamic_cast<CTalkMonster*>(pMonster)->CanFollow())  // Ignore if can't follow
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
	else if (pMonster->IsFollowing() && !IsWeapon(controller.GetWeaponId()) && CheckStopSignal(pMonster, controller.GetPosition(), controller.GetAngles()))
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


bool IsPullingDownFastEnoughForLedge(const Vector& velocity)
{
	const float minLedgePullSpeed = CVAR_GET_FLOAT("vr_ledge_pull_speed");
	return velocity.z < 0.f && velocity.Length() > minLedgePullSpeed;
}

bool AreInSamePlane(const Vector& pos1, const Vector& pos2)
{
	return fabs(pos1.z - pos2.z) < 12.f;
}

bool IsInLedge(CBasePlayer* pPlayer, const Vector& pos, float* ledgeheight)
{
	TraceResult tr;
	UTIL_TraceLine(pos + Vector{ 0.f, 0.f, 32.f }, pos, ignore_monsters, pPlayer->edict(), &tr);
	if (tr.flFraction < 1.f && DotProduct(tr.vecPlaneNormal, Vector{ 0.f, 0.f, 1.f }) > 0.8f)
	{
		*ledgeheight = tr.vecEndPos.z;
		return true;
	}
	return false;
}


Vector CalculateLedgeTargetPosition(CBasePlayer* pPlayer, const Vector& controllerPosition1, float ledgeheight1, const Vector& controllerPosition2, float ledgeheight2, bool* isValidPosition)
{
	Vector middlePoint = (Vector{ controllerPosition1.x, controllerPosition1.y, ledgeheight1 } +Vector{ controllerPosition2.x, controllerPosition2.y, ledgeheight2 }) * 0.5f;
	middlePoint.z -= VEC_DUCK_HULL_MIN.z;
	middlePoint.z += 1.f;

	extern playermove_t* pmove;
	int originalhull = pmove->usehull;
	pmove->usehull = 1;

	// find closest free space that player can be pulled up to
	Vector closestoffset;
	bool foundfreepoint = false;
	for (int x = -16; x <= 16; x++)
	{
		for (int y = -16; y <= 16; y++)
		{
			for (int z = 0; z <= 16; z++)
			{
				Vector offset = Vector{ float(x), float(y), float(z) };
				if (pmove->PM_TestPlayerPosition(middlePoint + offset, nullptr) < 0)
				{
					if (!foundfreepoint || offset.LengthSquared() < closestoffset.LengthSquared())
					{
						closestoffset = offset;
					}
					foundfreepoint = true;
				}
			}
		}
	}

	pmove->usehull = originalhull;

	*isValidPosition = foundfreepoint;
	return middlePoint + closestoffset;
}



void VRControllerInteractionManager::DoMultiControllerActions(CBasePlayer* pPlayer, const VRController& controller1, const VRController& controller2)
{
	float vr_ledge_pull_mode = CVAR_GET_FLOAT("vr_ledge_pull_mode");
	if (vr_ledge_pull_mode != 1.f && vr_ledge_pull_mode != 2.f)
		return;

	// avoid pull cascades
	if (pPlayer->m_vrWasPullingOnLedge && controller1.IsDragging() && controller2.IsDragging())
		return;

	pPlayer->m_vrWasPullingOnLedge = false;

	float ledgeheight1 = 0.f;
	float ledgeheight2 = 0.f;

	// pulling
	if (pPlayer->m_vrIsPullingOnLedge)
	{
		pPlayer->m_vrLedgePullSpeed = (std::max)(pPlayer->m_vrLedgePullSpeed, (std::max)(controller1.GetVelocity().Length(), controller2.GetVelocity().Length()));

		float timeEllapsed = gpGlobals->time - pPlayer->m_vrLedgePullStartTime;
		if (timeEllapsed > 0.f)
		{
			float totalDistance = (pPlayer->m_vrLedgeTargetPosition - pPlayer->m_vrLedgePullStartPosition).Length();

			float percentageComplete = totalDistance / (pPlayer->m_vrLedgePullSpeed * timeEllapsed);

			if (percentageComplete >= 1.f)
			{
				// finished pulling
				pPlayer->pev->origin = pPlayer->m_vrLedgeTargetPosition;
				pPlayer->m_vrIsPullingOnLedge = false;
				pPlayer->m_vrWasPullingOnLedge = true;
			}
			else if (percentageComplete <= 0.f)
			{
				pPlayer->pev->origin = pPlayer->m_vrLedgePullStartPosition;
			}
			else
			{
				pPlayer->pev->origin = pPlayer->m_vrLedgeTargetPosition * percentageComplete + pPlayer->m_vrLedgePullStartPosition * (1.f - percentageComplete);
			}

			UTIL_SetOrigin(pPlayer->pev, pPlayer->pev->origin);
		}
	}
	// starting to pull
	else if (controller1.IsDragging() && controller2.IsDragging() && IsPullingDownFastEnoughForLedge(controller1.GetVelocity()) && IsPullingDownFastEnoughForLedge(controller2.GetVelocity()) && AreInSamePlane(controller1.GetPosition(), controller2.GetPosition()) && IsInLedge(pPlayer, controller1.GetPosition(), &ledgeheight1) && IsInLedge(pPlayer, controller2.GetPosition(), &ledgeheight2))
	{
		bool isValidPosition = false;
		Vector ledgeTargetPosition = CalculateLedgeTargetPosition(pPlayer, controller1.GetPosition(), ledgeheight1, controller2.GetPosition(), ledgeheight2, &isValidPosition);

		if (isValidPosition)
		{
			if (!FBitSet(pPlayer->pev->flags, FL_DUCKING))
			{
				pPlayer->pev->flags |= FL_DUCKING;
				UTIL_SetSize(pPlayer->pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
				pPlayer->pev->origin.z += (VEC_HULL_MIN.z - VEC_DUCK_HULL_MIN.z);
			}

			if (vr_ledge_pull_mode == 2.f)
			{
				// Instant
				pPlayer->pev->origin = ledgeTargetPosition;
				pPlayer->m_vrWasPullingOnLedge = true;
			}
			else
			{
				// Move
				pPlayer->StartPullingLedge(ledgeTargetPosition, (std::max)(controller1.GetVelocity().Length(), controller2.GetVelocity().Length()));
			}

			UTIL_SetOrigin(pPlayer->pev, pPlayer->pev->origin);
		}
	}
}
