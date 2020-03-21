
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <algorithm>

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
constexpr const int VR_MIN_ANGRY_GUNPOINT_TIME = 2;
constexpr const int VR_MAX_ANGRY_GUNPOINT_TIME = 5;

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

constexpr const float VR_MAX_DRAG_START_TIME = 0.5f;


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

bool WasJustThrownByPlayer(CBasePlayer* pPlayer, CBaseEntity *pEntity)
{
	if (pEntity->pev->owner != pPlayer->edict())
		return false;

	if (!FClassnameIs(pEntity->pev, "monster_satchel")
		&& !FClassnameIs(pEntity->pev, "grenade")
		&& !FClassnameIs(pEntity->pev, "monster_snark")
		&& !FClassnameIs(pEntity->pev, "rpg_rocket"))
		return false;

	return (gpGlobals->time - pEntity->m_spawnTime) < 2.f;
}

bool IsUsableDoor(edict_t* pent)
{
	return (FClassnameIs(&pent->v, "func_door") || FClassnameIs(&pent->v, "func_door_rotating"))
		&& FBitSet(&pent->v.spawnflags, SF_DOOR_USE_ONLY);
}

bool IsNonInteractingEntity(edict_t* pent)
{
	// TODO/NOTE: If this mod gets ever patched up for multiplayer,
	// and you want players to be able to crowbar-fight,
	// vr_controllermodel needs to be handled differently!
	return (pent->v.solid == SOLID_NOT && !IsUsableDoor(pent))
		|| FClassnameIs(&pent->v, "func_wall")
		|| FClassnameIs(&pent->v, "func_illusionary")
		|| FClassnameIs(&pent->v, "vr_controllermodel"); 
}

bool IsReachable(CBasePlayer* pPlayer, edict_t* pent, const StudioHitBox& hitbox, bool strict)
{
	TraceResult tr{ 0 };
	UTIL_TraceLine(pPlayer->pev->origin, hitbox.abscenter, ignore_monsters, nullptr, &tr);
	if (tr.pHit == pent)
		return true;

	if (tr.flFraction < 1.f)
		return false;

	if (!strict)
		return true;

	return !VRPhysicsHelper::Instance().CheckIfLineIsBlocked(pPlayer->pev->origin, hitbox.abscenter);
}

bool IsTriggerOrButton(CBaseEntity *pEntity)
{
	if (!pEntity)
		return false;

	// usables (doors, buttons, levers etc)
	if (pEntity->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE))
		return true;

	// triggers
	if (pEntity->pev->solid == SOLID_TRIGGER && pEntity->pev->model && STRING(pEntity->pev->model)[0] == '*')
		return true;

	return false;
}

bool CheckIfEntityAndHitboxesTouch(CBasePlayer* pPlayer, CBaseEntity* pEntity, const std::vector<StudioHitBox>& hitboxes, VRPhysicsHelperModelBBoxIntersectResult* intersectResult)
{
	// Check each hitbox of current weapon
	for (auto hitbox : hitboxes)
	{
		// Prevent interaction with stuff through walls
		// (simple check for non-important stuff like ammo, strict check using physics engine for important things like triggers, NPCs and buttons)
		if (!IsReachable(pPlayer, pEntity->edict(), hitbox, IsTriggerOrButton(pEntity)))
			continue;

		if (VRPhysicsHelper::Instance().ModelIntersectsBBox(pEntity, hitbox.origin, hitbox.mins, hitbox.maxs, hitbox.angles, intersectResult)
			// TODO: Use VRPhysicsHelper to check if point is inside entity with proper raytracing
			|| UTIL_IsPointInEntity(pEntity, hitbox.abscenter))
		{
			if (!intersectResult->hasresult)
			{
				intersectResult->hitpoint = hitbox.abscenter;
				intersectResult->hitgroup = 0;
				intersectResult->hasresult = true;
			}
			return true;
		}
	}
	return false;
}

bool VRControllerInteractionManager::CheckIfEntityAndControllerTouch(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, VRPhysicsHelperModelBBoxIntersectResult* intersectResult)
{
	if (!controller.IsValid())
	{
#ifdef RENDER_DEBUG_BBOXES
		g_VRDebugBBoxDrawer.Clear(pEntity);
#endif
		return false;
	}

	if (controller.GetRadius() <= 0.f)
	{
#ifdef RENDER_DEBUG_BBOXES
		g_VRDebugBBoxDrawer.Clear(pEntity);
#endif
		return false;
	}

	if (IsNonInteractingEntity(pEntity->edict()) && !IsDraggableEntity(pEntity))
	{
#ifdef RENDER_DEBUG_BBOXES
		g_VRDebugBBoxDrawer.Clear(pEntity);
#endif
		return false;
	}

	// Don't hit stuff we just threw ourselfes
	if (WasJustThrownByPlayer(pPlayer, pEntity))
	{
#ifdef RENDER_DEBUG_BBOXES
		g_VRDebugBBoxDrawer.Clear(pEntity);
#endif
		return false;
	}

	// Rule out entities too far away
	if (pEntity->edict() != INDEXENT(0))
	{
		float entityRadius = pEntity->pev->size.Length() * 0.5f;
		if (entityRadius == 0.f)
		{
			// assume this is a gib and use sensible radius
			entityRadius = 16.f;
		}
		Vector entityCenter = pEntity->Center();
		float distance = (controller.GetPosition() - entityCenter).Length();
		if (distance > (controller.GetRadius() + entityRadius))
		{
#ifdef RENDER_DEBUG_BBOXES
			g_VRDebugBBoxDrawer.Clear(pEntity);
#endif
			return false;
		}
	}

#ifdef RENDER_DEBUG_BBOXES
	// draw bboxes of entities in close proximity of controller
	g_VRDebugBBoxDrawer.SetColor(0, 0, 255);
	g_VRDebugBBoxDrawer.DrawBBoxes(pEntity);
#endif

	// first check controller hitboxes
	if (CheckIfEntityAndHitboxesTouch(pPlayer, pEntity, controller.GetHitBoxes(), intersectResult))
	{
#ifdef RENDER_DEBUG_BBOXES
		g_VRDebugBBoxDrawer.DrawPoint(pEntity, intersectResult->hitpoint);
#endif
		return true;
	}

	// then check hitboxes of dragged entity (if any)
	if (controller.HasDraggedEntity())
	{
		CBaseEntity *pDraggedEntity = controller.GetDraggedEntity();
		if (IsDraggableEntityThatCanTouchStuff(pDraggedEntity))
		{
			void* pmodel = GET_MODEL_PTR(pDraggedEntity->edict());
			if (pmodel)
			{
				int numhitboxes = GetNumHitboxes(pmodel);
				if (numhitboxes > 0)
				{
					std::vector<StudioHitBox> draggedentityhitboxes;
					draggedentityhitboxes.resize(numhitboxes);
					if (GetHitboxesAndAttachments(pDraggedEntity->pev, pmodel, pDraggedEntity->pev->sequence, pDraggedEntity->pev->frame, draggedentityhitboxes.data(), nullptr))
					{
						if (CheckIfEntityAndHitboxesTouch(pPlayer, pDraggedEntity, draggedentityhitboxes, intersectResult))
						{
							return true;
						}
					}
				}
			}
		}
	}

#ifdef RENDER_DEBUG_BBOXES
	g_VRDebugBBoxDrawer.ClearPoint(pEntity);
#endif
	return false;
}

bool VRControllerInteractionManager::IsDraggableEntity(CBaseEntity* pEntity)
{
	if (!pEntity)
		return false;

	return FClassnameIs(pEntity->pev, "vr_easteregg")
		|| FClassnameIs(pEntity->pev, "func_rot_button")
		|| FClassnameIs(pEntity->pev, "momentary_rot_button")
		|| (FClassnameIs(pEntity->pev, "func_door_rotating") && FBitSet(pEntity->pev->spawnflags, SF_DOOR_USE_ONLY))
		|| FClassnameIs(pEntity->pev, "func_pushable")
		|| (CVAR_GET_FLOAT("vr_ladder_immersive_movement_enabled") != 0.f && FClassnameIs(pEntity->pev, "func_ladder"))
		|| pEntity->IsDraggable();
}

bool VRControllerInteractionManager::IsDraggableEntityThatCanTouchStuff(CBaseEntity* pEntity)
{
	if (!pEntity)
		return false;

	if (FClassnameIs(pEntity->pev, "func_rot_button")
		|| FClassnameIs(pEntity->pev, "momentary_rot_button")
		|| FClassnameIs(pEntity->pev, "func_door_rotating")
		|| FClassnameIs(pEntity->pev, "func_pushable")
		|| FClassnameIs(pEntity->pev, "func_ladder"))
		return false;

	if (pEntity->IsBSPModel() || !pEntity->pev->model || STRING(pEntity->pev->model)[0] == '*')
		return false;

	return FClassnameIs(pEntity->pev, "vr_easteregg") || pEntity->IsDraggable();
}

constexpr const int VR_DRAG_DISTANCE_TOLERANCE = 64;

bool DistanceTooBigForDragging(CBaseEntity* pEntity, const VRController& controller)
{
	Vector entityCenter = pEntity->Center();
	float entityRadius = pEntity->pev->size.Length() * 0.5f;
	float distance = (controller.GetPosition() - entityCenter).Length();
	return distance > (controller.GetRadius() + entityRadius + VR_DRAG_DISTANCE_TOLERANCE);
}

void VRControllerInteractionManager::CheckAndPressButtons(CBasePlayer* pPlayer, VRController& controller)
{
	// Skip invalid controllers.
	// (Except if it has any entities, as we need to tell them that they were let go.
	// Next frame all entities will have been cleared.)
	if (!controller.IsValid() && !controller.HasAnyEntites())
	{
		return;
	}

	edict_t* pentcontrollermodel = controller.GetModel()->edict();
	edict_t* pentplayer = pPlayer->edict();

	bool isDraggingAnEntity = false;

	for (int index = 1; index < gpGlobals->maxEntities; index++)
	{
		edict_t* pent = INDEXENT(index);
		if (FNullEnt(pent))
			continue;

		if (pent == pentplayer)
		{
			continue;
		}

		if (pent == pentcontrollermodel)
		{
			continue;
		}

		// skip point entities (no model and/or no size)
		if (FStringNull(pent->v.model) && pent->v.size.x == 0.f && pent->v.size.y == 0.f && pent->v.size.z == 0.f)
		{
			continue;
		}

		bool isTouching;
		bool didTouchChange;
		bool isDragging;
		bool didDragChange;
		bool isHitting;
		bool didHitChange;

		CBaseEntity *pEntity = CBaseEntity::UnsafeInstance(pent);
		if (!pEntity)
		{
			continue;
		}

		VRPhysicsHelperModelBBoxIntersectResult intersectResult;
		isTouching = CheckIfEntityAndControllerTouch(pPlayer, pEntity, controller, &intersectResult);

#ifdef RENDER_DEBUG_BBOXES
		if (isTouching)
		{
			pEntity->pev->rendermode = kRenderTransTexture;
			pEntity->pev->renderamt = 128;
			pEntity->pev->renderfx = kRenderFxGlowShell;
			pEntity->pev->rendercolor.x = 0;
			pEntity->pev->rendercolor.y = 255;
			pEntity->pev->rendercolor.z = 0;
		}
#endif

		// If we are dragging something draggable, we override all the booleans to avoid "losing" the entity due to fast movements
		if (controller.IsDragging() && controller.IsDraggedEntity(pEntity) && (isTouching || !DistanceTooBigForDragging(pEntity, controller)))
		{
			isTouching = true;
			didTouchChange = false;
			isDragging = true;
			didDragChange = false;
			isHitting = false;
			didHitChange = controller.RemoveHitEntity(pEntity);
		}
		else
		{
			didTouchChange = isTouching ? controller.AddTouchedEntity(pEntity) : controller.RemoveTouchedEntity(pEntity);

			if (!isTouching
				|| !controller.IsDragging()
				|| (controller.HasDraggedEntity() && !controller.IsDraggedEntity(pEntity)))
			{
				isDragging = false;
				didDragChange = controller.RemoveDraggedEntity(pEntity);
			}
			else if (!controller.HasDraggedEntity() && std::fabs(controller.GetDragStartTime() - gpGlobals->time) <= VR_MAX_DRAG_START_TIME)
			{
				isDragging = true;
				didDragChange = true;
			}
			else
			{
				// controller.IsDraggedEntity(hEntity) is probably false always,
				// as that should be covered by the if-statement above that overrides all booleans,
				// but just to be sure
				isDragging = controller.IsDraggedEntity(pEntity);
				didDragChange = false;
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
				Vector relativeVelocity = pPlayer->pev->velocity + controllerSwingVelocity - pent->v.velocity;
				isHitting = controllerSwingVelocity.Length() > GetMeleeSwingSpeed() && relativeVelocity.Length() > GetMeleeSwingSpeed();
			}
			else
			{
				isHitting = false;
			}
			didHitChange = isHitting ? controller.AddHitEntity(pEntity) : controller.RemoveHitEntity(pEntity);
		}

		float flHitDamage = 0.f;
		if (isHitting && didHitChange && pPlayer->HasWeapons())
		{
			flHitDamage = DoDamage(pPlayer, pEntity, controller, intersectResult);
		}

		Interaction::InteractionInfo touching{ isTouching, didTouchChange };
		Interaction::InteractionInfo dragging{ isDragging, didDragChange };
		Interaction::InteractionInfo hitting{ isHitting, didHitChange };
		Interaction interaction{ touching, dragging, hitting, flHitDamage, intersectResult.hasresult ? intersectResult.hitpoint : controller.GetPosition() };

		if (!interaction.IsInteresting())
		{
			continue;
		}

		// vibrate if touching something solid (but not if we are dragging it!)
		if (isTouching && pEntity->m_vrDragger != pPlayer && pEntity->m_vrDragController != controller.GetID() && !controller.IsDraggedEntity(pEntity))
		{
			if ((pEntity->pev->solid != SOLID_NOT && pEntity->pev->solid != SOLID_TRIGGER)
				|| (pEntity->pev->solid == SOLID_TRIGGER && (pEntity->pev->movetype == MOVETYPE_TOSS || pEntity->pev->movetype == MOVETYPE_BOUNCE))
				|| pEntity->IsDraggable())
			{
				if (isHitting)
				{
					if (pEntity->pev->solid == SOLID_BSP)
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
					if (pEntity->pev->solid == SOLID_BSP)
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

		if (HandleGrabbables(pPlayer, pEntity, controller, interaction))
			;
		else if (HandleButtonsAndDoors(pPlayer, pEntity, controller, interaction))
			;
		else if (HandleRechargers(pPlayer, pEntity, controller, interaction))
			;
		else if (HandleTriggers(pPlayer, pEntity, controller, interaction))
			;
		else if (HandleBreakables(pPlayer, pEntity, controller, interaction))
			;
		else if (HandlePushables(pPlayer, pEntity, controller, interaction))
			;
		else if (HandleLadders(pPlayer, pEntity, controller, interaction))
			;
		else if (HandleAlliedMonsters(pPlayer, pEntity, controller, interaction))
			;
		else if (HandleTossables(pPlayer, pEntity, controller, interaction))
			;
		else if (isTouching && didTouchChange)
		{
			// Touch any entity not handled yet and set player velocity temporarily to controller velocity, as some entities (e.g. cockroaches) use the velocity in their touch logic
			Vector backupVel = pPlayer->pev->velocity;
			pPlayer->pev->velocity = controller.GetVelocity();
			pEntity->Touch(pPlayer);
			pPlayer->pev->velocity = backupVel;
		}

		if (isDragging && controller.IsDraggedEntity(pEntity))
		{
			isDraggingAnEntity = true;
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

float VRControllerInteractionManager::DoDamage(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const VRPhysicsHelperModelBBoxIntersectResult& intersectResult)
{
	if (!pEntity || pEntity->pev->takedamage == DAMAGE_NO || pEntity->pev->solid == SOLID_NOT || pEntity->pev->solid == SOLID_TRIGGER)
		return 0.f;

	Vector controllerSwingVelocity = controller.GetVelocity();
	if (intersectResult.hasresult)
	{
		Vector relativeHitPos = intersectResult.hitpoint - controller.GetPosition();
		Vector previousRotatedRelativeHitPos = VRPhysicsHelper::Instance().RotateVectorInline(relativeHitPos, controller.GetPreviousAngles());
		Vector rotatedRelativeHitPos = VRPhysicsHelper::Instance().RotateVectorInline(relativeHitPos, controller.GetAngles());
		controllerSwingVelocity = controllerSwingVelocity + rotatedRelativeHitPos - previousRotatedRelativeHitPos;
	}

	float speed = (pPlayer->pev->velocity + controllerSwingVelocity - pEntity->pev->velocity).Length();
	float damage = UTIL_CalculateMeleeDamage(controller.GetWeaponId(), speed);
	if (damage > 0.f)
	{
		Vector impactPosition = intersectResult.hasresult ? intersectResult.hitpoint : controller.GetPosition();

		pPlayer->PlayMeleeSmackSound(pEntity, controller.GetWeaponId(), impactPosition, controllerSwingVelocity);

		int backupBlood = pEntity->m_bloodColor;
		if (IsSoftWeapon(controller.GetWeaponId()))
		{
			// Slapping with soft things (hands, snarks or hornetgun) just causes damage, but no blood or decals
			pEntity->m_bloodColor = DONT_BLEED;
		}
		TraceResult fakeTrace = { 0 };
		fakeTrace.pHit = pEntity->edict();
		fakeTrace.vecEndPos = impactPosition;
		fakeTrace.flFraction = 0.5f;
		if (intersectResult.hasresult)
		{
			fakeTrace.iHitgroup = intersectResult.hitgroup;
		}
		ClearMultiDamage();
		pEntity->TraceAttack(pPlayer->pev, damage, controllerSwingVelocity.Normalize(), &fakeTrace, UTIL_DamageTypeFromWeapon(controller.GetWeaponId()));
		ApplyMultiDamage(pPlayer->pev, pPlayer->pev);
		pEntity->m_bloodColor = backupBlood;

		// Entities that support being "baseballed" overwrite this method
		pEntity->BaseBalled(pPlayer, controllerSwingVelocity);

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

bool VRControllerInteractionManager::HandleButtonsAndDoors(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	// Don't touch activate retina scanners
	extern std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> g_vrRetinaScannerButtons;
	if (g_vrRetinaScannerButtons.count(pEntity) > 0)
		return true;

	if (FClassnameIs(pEntity->pev, "func_door") || (FClassnameIs(pEntity->pev, "func_door_rotating") && !FBitSet(pEntity->pev->spawnflags, SF_DOOR_USE_ONLY)))
	{
		if (interaction.touching.isSet && interaction.touching.didChange)
		{
			if (FBitSet(pEntity->pev->spawnflags, SF_DOOR_USE_ONLY))
			{
				pEntity->Use(pPlayer, pPlayer, USE_SET, 1);
			}
			else
			{
				pEntity->Touch(pPlayer);
			}
		}
		return true;
	}
	else if (FClassnameIs(pEntity->pev, "func_button"))
	{
		if (interaction.touching.isSet)
		{
			if (interaction.touching.didChange || FBitSet(pEntity->ObjectCaps(), FCAP_CONTINUOUS_USE))
			{
				if (FBitSet(pEntity->pev->spawnflags, SF_BUTTON_TOUCH_ONLY))  // touchable button
				{
					pEntity->Touch(pPlayer);
				}
				else
				{
					pEntity->Use(pPlayer, pPlayer, USE_SET, 1);
				}
			}
		}
		return true;
	}
	else if (FClassnameIs(pEntity->pev, "func_rot_button") || FClassnameIs(pEntity->pev, "momentary_rot_button") || (FClassnameIs(pEntity->pev, "func_door_rotating") && FBitSet(pEntity->pev->spawnflags, SF_DOOR_USE_ONLY)))
	{
		VRRotatableEnt* pRotatableEnt = pEntity->MyRotatableEntPtr();
		if (pRotatableEnt)
		{
			if (interaction.dragging.isSet)
			{
				if (interaction.dragging.didChange)
				{
					if (controller.SetDraggedEntity(pEntity))
					{
						pRotatableEnt->ClearDraggingCancelled();
					}
				}
				if (controller.IsDraggedEntity(pEntity))
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

bool VRControllerInteractionManager::HandleRechargers(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	if (FClassnameIs(pEntity->pev, "func_healthcharger") || FClassnameIs(pEntity->pev, "func_recharge"))
	{
		if (interaction.touching.isSet)
		{
			pEntity->Use(pPlayer, pPlayer, USE_SET, 1);
		}
		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleTriggers(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	if (pEntity->pev->solid == SOLID_TRIGGER && strncmp(STRING(pEntity->pev->classname), "trigger_", 8) == 0)
	{
		if (interaction.touching.isSet)
		{
			// Only touch trigger_multiple,trigger_once and trigger_hurt.
			// Other triggers should only be activated by the actual player "body".
			if (FClassnameIs(pEntity->pev, "trigger_multiple") || FClassnameIs(pEntity->pev, "trigger_once"))
			{
				pEntity->Touch(pPlayer);
			}
			else if (FClassnameIs(pEntity->pev, "trigger_hurt"))
			{
				// Cause 10% damage when touching with hand
				float dmgBackup = pEntity->pev->dmg;
				pEntity->pev->dmg *= 0.1f;
				pEntity->Touch(pPlayer);
				pEntity->pev->dmg = dmgBackup;
			}
		}
		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleBreakables(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	if (!FClassnameIs(pEntity->pev, "func_breakable"))
		return false;

	// Make sure breakables that are supposed to immediately break on melee attacks, do break.
	if (interaction.hitting.isSet && interaction.hitting.didChange)
	{
		CBreakable* pBreakable = dynamic_cast<CBreakable*>(pEntity);
		if (pBreakable
			&& pBreakable->pev->takedamage != DAMAGE_NO
			&& !FBitSet(pBreakable->pev->spawnflags, SF_BREAK_TRIGGER_ONLY)
			&& FBitSet(pBreakable->pev->spawnflags, SF_BREAK_CROWBAR)
			&& pBreakable->IsBreakable()
			&& pPlayer->HasWeapons())
		{
			pBreakable->TakeDamage(pPlayer->pev, pPlayer->pev, pBreakable->pev->health, DMG_CLUB);
		}
	}

	return true;
}

bool VRControllerInteractionManager::HandlePushables(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	if (!FClassnameIs(pEntity->pev, "func_pushable"))
		return false;

	CPushable* pPushable = dynamic_cast<CPushable*>(pEntity);
	if (pPushable != nullptr)
	{
		if (interaction.touching.isSet)
		{
			bool onlyZ = false;

			// Is player standing on this pushable?
			if (FBitSet(pPlayer->pev->flags, FL_ONGROUND) && pPlayer->pev->groundentity && VARS(pPlayer->pev->groundentity) == pPushable->pev)
			{
				// Allow vertical push if floating
				if (pPushable->pev->waterlevel > 0)
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

			if (!onlyZ && interaction.dragging.isSet && controller.SetDraggedEntity(pEntity) && controller.GetDraggedEntityPositions(pEntity, dummy, controllerDragStartPos, entityDragStartOrigin, dummy, dummy, dummy, dummy, dummy))
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

bool VRControllerInteractionManager::HandleGrabbables(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	if (pEntity->IsDraggable())
	{
		if (interaction.dragging.isSet)
		{
			if (interaction.dragging.didChange)
			{
				if (controller.SetDraggedEntity(pEntity))
				{
					// did another controller lose this dragged entity?
					if (pEntity->m_vrDragger)
					{
						EHANDLE<CBasePlayer> hOtherPlayer = pEntity->m_vrDragger;
						if (hOtherPlayer)
						{
							hOtherPlayer->GetController(pEntity->m_vrDragController).RemoveDraggedEntity(pEntity);
						}
					}
					pEntity->m_vrDragger = pPlayer;
					pEntity->m_vrDragController = controller.GetID();
					pEntity->SetThink(&CBaseEntity::DragStartThink);
					pEntity->pev->nextthink = gpGlobals->time;
				}
			}
			else
			{
				// If we are the same player and controller that last started dragging the entity, update drag
				if (controller.IsDraggedEntity(pEntity) && pEntity->m_vrDragger == pPlayer && pEntity->m_vrDragController == controller.GetID())
				{
					pEntity->SetThink(&CBaseEntity::DragThink);
					pEntity->pev->nextthink = gpGlobals->time;
				}
			}
		}
		else
		{
			if (interaction.dragging.didChange)
			{
				// If we are the same player and controller that last started dragging the entity, stop dragging
				if (pEntity->m_vrDragger == pPlayer && pEntity->m_vrDragController == controller.GetID())
				{
					pEntity->m_vrDragger = nullptr;
					pEntity->m_vrDragController = VRControllerID::INVALID;
					pEntity->SetThink(&CBaseEntity::DragStopThink);
					pEntity->pev->nextthink = gpGlobals->time;
				}
			}
		}

		return true;
	}

	return false;
}

bool VRControllerInteractionManager::HandleLadders(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	if (FClassnameIs(pEntity->pev, "func_ladder") && CVAR_GET_FLOAT("vr_ladder_immersive_movement_enabled") != 0.f)
	{
		if (interaction.dragging.didChange)
		{
			if (interaction.dragging.isSet)
			{
				if (controller.SetDraggedEntity(pEntity))
				{
					pPlayer->SetLadderGrabbingController(controller.GetID(), pEntity);
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
			if (interaction.dragging.isSet && controller.IsDraggedEntity(pEntity) && pPlayer->IsLadderGrabbingController(controller.GetID(), pEntity))
			{
				Vector controllerDragStartOffset;
				Vector playerDragStartOrigin;
				Vector dummy;
				if (controller.GetDraggedEntityPositions(pEntity,
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
					if (fabs(pEntity->pev->size.x) > fabs(pEntity->pev->size.y))
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

bool VRControllerInteractionManager::HandleTossables(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	if (interaction.touching.isSet
		&& (pEntity->pev->movetype == MOVETYPE_TOSS || pEntity->pev->movetype == MOVETYPE_BOUNCE)
		&& pEntity->pev->solid == SOLID_BBOX)
	{
		if (interaction.touching.didChange)
		{
			pEntity->Touch(pPlayer);
		}
		Vector intersection = pEntity->pev->origin - interaction.intersectPoint;
		Vector pushAwayVelocity = controller.GetVelocity() + (intersection * 0.5f);
		if (pushAwayVelocity.Length() > 100)
		{
			pushAwayVelocity = pushAwayVelocity.Normalize() * 100;
		}
		pEntity->pev->velocity = pushAwayVelocity;
		return true;
	}
	return false;
}

bool VRControllerInteractionManager::HandleAlliedMonsters(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction)
{
	// Special handling of barneys and scientists that don't hate us (yet)
	if ((FClassnameIs(pEntity->pev, "monster_barney") || FClassnameIs(pEntity->pev, "monster_scientist")))
	{
		CTalkMonster* pMonster = dynamic_cast<CTalkMonster*>(pEntity);
		if (pMonster && !pMonster->HasMemory(bits_MEMORY_PROVOKED))
		{
			if (interaction.touching.isSet)
			{
				// Face player when touched
				pMonster->MakeIdealYaw(pPlayer->pev->origin);
			}

			// Make scientists panic and barneys angry if pointing guns at them
			GetAngryIfAtGunpoint(pPlayer, pMonster, controller);

			// Don't do any following/unfollowing if we just hurt the ally somehow
			if (interaction.hitDamage > 0.f)
			{
				pMonster->vr_flStopSignalTime = 0;
				pMonster->vr_flShoulderTouchTime = 0;
			}

			// Make scientists and barneys move away if holding weapons in their face, or we hit them
			if (interaction.touching.isSet && (interaction.hitDamage > 0.f || IsWeapon(controller.GetWeaponId())))
			{
				pMonster->vr_flStopSignalTime = 0;
				pMonster->vr_flShoulderTouchTime = 0;
				pMonster->SetConditions(bits_COND_CLIENT_PUSH);
				pMonster->MakeIdealYaw(controller.GetPosition());
				pMonster->Touch(pPlayer);
			}

			// Follow/Unfollow commands
			if (interaction.hitDamage == 0.f && !pMonster->HasMemory(bits_MEMORY_PROVOKED))
			{
				DoFollowUnfollowCommands(pPlayer, pMonster, controller, interaction.touching.isSet);
			}
		}
		return true;
	}

	return false;
}

void VRControllerInteractionManager::GetAngryIfAtGunpoint(CBasePlayer* pPlayer, CTalkMonster* pMonster, const VRController& controller)
{
	float vr_npc_gunpoint = CVAR_GET_FLOAT("vr_npc_gunpoint");
	if (vr_npc_gunpoint == 0.f)
		return;

	// if we already hate the player, no need to react to guns
	if (pMonster->HasMemory(bits_MEMORY_PROVOKED) || pMonster->HasMemory(bits_MEMORY_GUNPOINT_ATTACK))
		return;

	if (!pMonster->m_hEnemy
		&& controller.GetID() == VRControllerID::WEAPON
		&& controller.IsValid()
		&& IsWeaponWithRange(controller.GetWeaponId())
		&& controller.GetWeaponId() != WEAPON_HORNETGUN	// hornetgun is too weird to react to
		&& (pPlayer->GetGunPosition() - pMonster->EyePosition()).Length() < VR_MAX_ANGRY_GUNPOINT_DIST
		&& UTIL_IsFacing(pPlayer->GetGunPosition(), pPlayer->GetWeaponViewAngles(), pMonster->pev->origin)
		&& UTIL_IsFacing(pMonster->pev->origin, pMonster->pev->angles.ToViewAngles(), pPlayer->GetGunPosition())
		&& pMonster->HasClearSight(pPlayer->GetGunPosition())
		&& UTIL_CheckTraceIntersectsEntity(pPlayer->GetGunPosition(), pPlayer->GetGunPosition() + (pPlayer->GetAutoaimVector() * VR_MAX_ANGRY_GUNPOINT_DIST), pMonster))
	{
		pMonster->vr_flStopSignalTime = 0;
		pMonster->vr_flShoulderTouchTime = 0;
		if (pMonster->vr_flGunPointTime == 0)
		{
			pMonster->vr_flGunPointTime = gpGlobals->time + VR_INITIAL_ANGRY_GUNPOINT_TIME;
		}
		else if (gpGlobals->time > pMonster->vr_flGunPointTime && pMonster->FOkToSpeak())
		{
			pMonster->MakeIdealYaw(pPlayer->pev->origin);
			if (pMonster->HasMemory(bits_MEMORY_GUNPOINT_THREE) && vr_npc_gunpoint > 1.f)
			{
				if (FClassnameIs(pMonster->pev, "monster_barney"))
				{
					pMonster->PlaySentence("BA_GUN_ATT", 4, VOL_NORM, ATTN_NORM);
				}
				else
				{
					pMonster->PlaySentence("SC_GUN_RUN", 4, VOL_NORM, ATTN_NORM);
				}
				pMonster->StopFollowing(TRUE);
				pMonster->Remember(bits_MEMORY_PROVOKED);
				pMonster->Remember(bits_MEMORY_GUNPOINT_ATTACK);
			}
			else if (pMonster->HasMemory(bits_MEMORY_GUNPOINT_TWO))
			{
				if (FClassnameIs(pMonster->pev, "monster_barney"))
				{
					pMonster->PlaySentence("BA_GUN_THR", 4, VOL_NORM, ATTN_NORM);
					pMonster->m_MonsterState = MONSTERSTATE_COMBAT;  // Make barneys draw their gun and point at player
				}
				else
				{
					pMonster->PlaySentence("SC_GUN_THR", 4, VOL_NORM, ATTN_NORM);
				}
				pMonster->Remember(bits_MEMORY_SUSPICIOUS);
				pMonster->Remember(bits_MEMORY_GUNPOINT_THREE);
			}
			else if (pMonster->HasMemory(bits_MEMORY_GUNPOINT_ONE))
			{
				if (FClassnameIs(pMonster->pev, "monster_barney"))
				{
					pMonster->PlaySentence("BA_GUN_TWO", 4, VOL_NORM, ATTN_NORM);
				}
				else
				{
					pMonster->PlaySentence("SC_GUN_TWO", 4, VOL_NORM, ATTN_NORM);
				}
				pMonster->Remember(bits_MEMORY_GUNPOINT_TWO);
			}
			else
			{
				if (FClassnameIs(pMonster->pev, "monster_barney"))
				{
					pMonster->PlaySentence("BA_GUN_ONE", 4, VOL_NORM, ATTN_NORM);
				}
				else
				{
					pMonster->PlaySentence("SC_GUN_ONE", 4, VOL_NORM, ATTN_NORM);
				}
				pMonster->Remember(bits_MEMORY_GUNPOINT_ONE);
			}
			pMonster->vr_flGunPointTime = gpGlobals->time + RANDOM_FLOAT(VR_MIN_ANGRY_GUNPOINT_TIME, VR_MAX_ANGRY_GUNPOINT_TIME);
		}
	}
	else
	{
		if (!pMonster->m_hEnemy && pMonster->HasMemory(bits_MEMORY_GUNPOINT_THREE) && FClassnameIs(pMonster->pev, "monster_barney"))
		{
			pMonster->m_MonsterState = MONSTERSTATE_ALERT;
			pMonster->ChangeSchedule(pMonster->ScheduleFromName("Barney Disarm"));
		}
		pMonster->vr_flGunPointTime = 0;
		pMonster->Forget(bits_MEMORY_GUNPOINT_ONE);
		pMonster->Forget(bits_MEMORY_GUNPOINT_TWO);
		pMonster->Forget(bits_MEMORY_GUNPOINT_THREE);
	}
}

void VRControllerInteractionManager::DoFollowUnfollowCommands(CBasePlayer* pPlayer, CTalkMonster* pMonster, const VRController& controller, bool isTouching)
{
	// Check if we're touching the shoulder for 1 second (follow me!)
	if (isTouching)
	{
		pMonster->vr_flStopSignalTime = 0;
		if (!IsWeapon(controller.GetWeaponId()) && ((controller.IsDragging() && !controller.HasDraggedEntity()) || CheckShoulderTouch(pMonster, controller.GetPosition())))
		{
			pMonster->ClearConditions(bits_COND_CLIENT_PUSH);
			if (pMonster->CanFollow())  // Ignore if can't follow
			{
				if (pMonster->vr_flShoulderTouchTime == 0)
				{
					pMonster->vr_flShoulderTouchTime = gpGlobals->time;
				}
				else if ((gpGlobals->time - pMonster->vr_flShoulderTouchTime) > VR_SHOULDER_TOUCH_TIME)
				{
					pMonster->FollowerUse(pPlayer, pPlayer, USE_ON, 1.f);
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
			pMonster->StopFollowing(TRUE);
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

bool VRControllerInteractionManager::Interaction::IsInteresting()
{
	return touching.isSet
		|| touching.didChange
		|| dragging.isSet
		|| dragging.didChange
		|| hitting.isSet
		|| hitting.didChange
		|| hitDamage > 0.f;
}
