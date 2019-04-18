
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


#define SF_BUTTON_TOUCH_ONLY	256	// button only fires as a result of USE key.
#define SF_DOOR_USE_ONLY		256	// door must be opened by player's use button.

#define	bits_COND_CLIENT_PUSH (bits_COND_SPECIAL1)

#define VR_MAX_ANGRY_GUNPOINT_DIST 256
#define VR_INITIAL_ANGRY_GUNPOINT_TIME 2
#define VR_MIN_ANGRY_GUNPOINT_TIME 3
#define VR_MAX_ANGRY_GUNPOINT_TIME 7

#define VR_SHOULDER_TOUCH_TIME 1.5f
#define VR_STOP_SIGNAL_TIME 1

#define VR_SHOULDER_TOUCH_MIN_Z 56
#define VR_SHOULDER_TOUCH_MAX_Z 62
#define VR_SHOULDER_TOUCH_MIN_Y 4
#define VR_SHOULDER_TOUCH_MAX_Y 12

#define VR_STOP_SIGNAL_MIN_Z 56
#define VR_STOP_SIGNAL_MAX_Z 72
#define VR_STOP_SIGNAL_MIN_X 8
#define VR_STOP_SIGNAL_MAX_X 36
#define VR_STOP_SIGNAL_MIN_Y -12
#define VR_STOP_SIGNAL_MAX_Y 12

#define VR_RETINASCANNER_ACTIVATE_LOOK_TIME 1.5f

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

// TODO TEMP
/*
std::unordered_map<int, std::vector<CBeam*>> debugbboxes;
void DrawDebugBBox(int which, bool valid, Vector pos, Vector angles, Vector mins, Vector maxs)
{
	if (!valid)
	{
		for (CBeam* pBeam : debugbboxes[which])
		{
			UTIL_Remove(pBeam);
		}
		debugbboxes[which].clear();
		return;
	}

	while (debugbboxes[which].size() < 8)
	{
		CBeam *pBeam = CBeam::BeamCreate("sprites/laserbeam.spr", 5);
		pBeam->PointsInit(mins, maxs);
		pBeam->SetBrightness(128);
		pBeam->SetWidth(1);
		pBeam->SetColor(255, 0, 0);
		pBeam->SetFlags(0x20);
		// pBeam->SetEndAttachment(1);
		pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
		debugbboxes[which].push_back(pBeam);
	}

	Vector minsmaxs[2] = { mins, maxs };



	debugbboxes[which][0]->PointsInit(
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[0].y, minsmaxs[0].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[0].y, minsmaxs[0].z }, angles)
	);

	debugbboxes[which][1]->PointsInit(
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[0].y, minsmaxs[0].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[1].y, minsmaxs[0].z }, angles));

	debugbboxes[which][2]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[0].y, minsmaxs[0].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[0].y, minsmaxs[1].z }, angles));


	debugbboxes[which][3]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[1].y, minsmaxs[0].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[0].y, minsmaxs[0].z }, angles));

	debugbboxes[which][3]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[1].y, minsmaxs[0].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[1].y, minsmaxs[0].z }, angles));

	debugbboxes[which][3]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[1].y, minsmaxs[0].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[1].y, minsmaxs[1].z }, angles));


	debugbboxes[which][0]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[1].y, minsmaxs[1].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[1].y, minsmaxs[0].z }, angles));

	debugbboxes[which][0]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[1].y, minsmaxs[1].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[0].y, minsmaxs[1].z }, angles));

	debugbboxes[which][0]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[1].y, minsmaxs[1].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[1].y, minsmaxs[1].z }, angles));


	debugbboxes[which][0]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[0].y, minsmaxs[1].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[0].y, minsmaxs[0].z }, angles));

	debugbboxes[which][0]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[0].y, minsmaxs[1].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[0].x, minsmaxs[0].y, minsmaxs[1].z }, angles));

	debugbboxes[which][0]->PointsInit(pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[0].y, minsmaxs[1].z }, angles),
		pos + VRPhysicsHelper::Instance().RotateVectorInline(Vector{ minsmaxs[1].x, minsmaxs[1].y, minsmaxs[1].z }, angles));
}
*/

bool CheckIfEntityAndControllerTouch(EHANDLE hEntity, const VRController& controller)
{
	if (!controller.IsValid() || !controller.IsBBoxValid())
		return false;

	if (hEntity->pev->solid == SOLID_NOT && !FClassnameIs(hEntity->pev, "vr_easteregg"))
		return false;

	bool isTouching = false;

	CBaseEntity *pWorld = CBaseEntity::Instance(INDEXENT(0));
	if (hEntity == pWorld)
	{
		isTouching = VRPhysicsHelper::Instance().ModelIntersectsWorld(controller.GetModel());
	}
	else
	{
		Vector entityCenter = (hEntity->pev->absmax + hEntity->pev->absmin) * 0.5f;
		float distance = (controller.GetPosition() - entityCenter).Length();
		if (distance > (controller.GetRadius() + (hEntity->pev->size.Length() * 0.5f)))
		{
			isTouching = false;
		}
		else
		{
			isTouching = VRPhysicsHelper::Instance().ModelsIntersect(controller.GetModel(), hEntity);
		}
	}

	return isTouching;
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

		// skip point entities (no model and/or no size)
		if (FStringNull(pEntity->pev->model) || pEntity->pev->size.LengthSquared() < EPSILON)
		{
			continue;
		}

		EHANDLE hEntity = pEntity;

		const bool isTouching = CheckIfEntityAndControllerTouch(hEntity, controller);
		const bool didTouchChange = isTouching ? controller.AddTouchedEntity(hEntity) : controller.RemoveTouchedEntity(hEntity);

		const bool isHitting = isTouching && controller.GetVelocity().Length() > GetMeleeSwingSpeed();
		const bool didHitChange = isHitting ? controller.AddHitEntity(hEntity) : controller.RemoveHitEntity(hEntity);

		float flHitDamage = 0.f;
		if (pPlayer->HasWeapons() && isHitting && didHitChange)
		{
			flHitDamage = DoDamage(pPlayer, hEntity, controller);
		}

		if (HandleEasterEgg(pPlayer, hEntity, controller, isTouching, didTouchChange));	// easter egg first, obviously the most important
		else if (HandleRetinaScanners(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleButtons(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleDoors(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleRechargers(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleTriggers(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleBreakables(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandlePushables(pPlayer, hEntity, controller, isTouching, didTouchChange));
		else if (HandleAlliedMonsters(pPlayer, hEntity, controller, isTouching, didTouchChange, isHitting, didHitChange, flHitDamage));
		else if (isTouching && didTouchChange)
		{
			hEntity->Touch(pPlayer);
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

float VRControllerInteractionManager::DoDamage(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller)
{
	if (hEntity->pev->solid == SOLID_NOT || hEntity->pev->solid == SOLID_TRIGGER)
		return 0.f;

	float speed = controller.GetVelocity().Length();
	float damage = UTIL_CalculateMeleeDamage(controller.GetWeaponId(), speed);
	if (damage > 0.f)
	{
		pPlayer->PlayMeleeSmackSound(hEntity, controller.GetWeaponId(), controller.GetPosition(), controller.GetVelocity());

		int backupBlood = hEntity->m_bloodColor;
		if (IsSoftWeapon(controller.GetWeaponId()))
		{
			// Slapping with soft things (hands, snarks or hornetgun) just causes damage, but no blood or decals
			hEntity->m_bloodColor = DONT_BLEED;
		}
		TraceResult fakeTrace = { 0 };
		fakeTrace.pHit = hEntity->edict();
		fakeTrace.vecEndPos = controller.GetPosition();
		fakeTrace.flFraction = 0.5f;
		ClearMultiDamage();
		hEntity->TraceAttack(pPlayer->pev, damage, controller.GetVelocity().Normalize(), &fakeTrace, UTIL_DamageTypeFromWeapon(controller.GetWeaponId()));
		ApplyMultiDamage(pPlayer->pev, pPlayer->pev);
		hEntity->m_bloodColor = backupBlood;

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
			pWorldsSmallestCup->pev->origin = pPlayer->GetGunPosition();
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

bool VRControllerInteractionManager::HandleButtons(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (FClassnameIs(hEntity->pev, "func_button") || FClassnameIs(hEntity->pev, "func_rot_button"))
	{
		// Don't touch activate retina scanners
		if (g_vrRetinaScannerButtons.count(hEntity) > 0)
			return true;

		// no activation of any buttons within 3 seconds of it spawning (fixes issues with the airlock levers near the tentacle)
		if ((gpGlobals->time - hEntity->m_spawnTime) < 3.f)
			return false;

		if (isTouching)
		{
			if (didTouchChange && FBitSet(hEntity->pev->spawnflags, SF_BUTTON_TOUCH_ONLY)) // touchable button
			{
				hEntity->Touch(pPlayer);
			}
			if (didTouchChange || FBitSet(hEntity->ObjectCaps(), FCAP_CONTINUOUS_USE))
			{
				hEntity->Use(pPlayer, pPlayer, USE_SET, 1);
			}
		}
		return true;
	}
	else if (FClassnameIs(hEntity->pev, "momentary_rot_button"))
	{
		if (isTouching)
		{
			hEntity->Use(pPlayer, pPlayer, USE_SET, 1);
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

	// Only handle trigger_multiple and trigger_once for now,
	// all other triggers should only be activated by the actual player "body"
	if (FClassnameIs(hEntity->pev, "trigger_multiple") || FClassnameIs(hEntity->pev, "trigger_once"))
	{
		if (isTouching && didTouchChange)
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

	return true;
}

bool VRControllerInteractionManager::HandleBreakables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (!FClassnameIs(hEntity->pev, "func_breakable"))
		return false;

	if (isTouching && didTouchChange)
	{
		CBreakable *pBreakable = dynamic_cast<CBreakable*>((CBaseEntity*)hEntity);
		// Only cause damage when we have weapons and is actually breakable
		if (pPlayer->HasWeapons() && pBreakable->IsBreakable())
		{
			if (pBreakable != nullptr && !FBitSet(pBreakable->pev->spawnflags, SF_BREAK_TRIGGER_ONLY))
			{
				if (FBitSet(pBreakable->pev->spawnflags, SF_BREAK_CROWBAR))
				{
					pBreakable->TakeDamage(pPlayer->pev, pPlayer->pev, pBreakable->pev->health, DMG_CLUB);
				}
				else if (FBitSet(pBreakable->pev->spawnflags, SF_BREAK_PRESSURE) || FBitSet(pBreakable->pev->spawnflags, SF_BREAK_TOUCH))
				{
					// Hack
					// Backup breakable spawnflags and our velocity
					int backupSpawnflags = pBreakable->pev->spawnflags;
					Vector backupVelocity = pPlayer->pev->velocity;
					// Override breakable spawnflags with break_touch and set our velocity to touch velocity
					pBreakable->pev->spawnflags = SF_BREAK_TOUCH;
					pPlayer->pev->velocity = controller.GetVelocity();
					// Make breakable handle the touch with our velocity
					pBreakable->BreakTouch(pPlayer);
					// Reset spawnflags and velocity
					pBreakable->pev->spawnflags = backupSpawnflags;
					pPlayer->pev->velocity = backupVelocity;
				}
			}
		}
	}
	return true;
}

bool VRControllerInteractionManager::HandlePushables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange)
{
	if (!FClassnameIs(hEntity->pev, "func_pushable"))
		return false;

	if (isTouching)
	{
		CPushable *pPushable = dynamic_cast<CPushable*>((CBaseEntity*)hEntity);
		if (pPushable != nullptr)
		{
			if (controller.IsDragging())
			{
				pPushable->pev->velocity = controller.GetVelocity();
			}
			else
			{
				// Get push velocity, but remove z (no up/down movement unless "grab" is on)
				Vector velocity = controller.GetVelocity();
				velocity.z = 0.f;

				// Don't pull a pushable unless "grab" is on
				// (we simply check if the angle between push velocity and pushable offset is less than 90°)
				Vector dir = (pPushable->pev->origin - controller.GetPosition()).Normalize();
				float dot = DotProduct(dir, velocity.Normalize()) > 0.f;
				if (dot > 0.f)
				{
					auto lambda = [](float pushVel, float pushableVel) -> float
					{
						if (pushVel*pushableVel >= 0.0f)
						{
							// same sign, use bigger absolute value
							// (this will push it faster if player hits faster than it already moves,
							//  or leaves speed as is, if player pushes slower in the direction it already moves)
							return (std::abs(pushVel) > std::abs(pushableVel)) ? pushVel : pushableVel;
						}
						else
						{
							// different sign, use pushVel
							// (this will stop a pushable from moving towards the player, and push it in opposite direction)
							return pushVel;
						}
					};

					pPushable->pev->velocity.x = lambda(velocity.x, pPushable->pev->velocity.x);
					pPushable->pev->velocity.y = lambda(velocity.y, pPushable->pev->velocity.y);
				}
			}

			if (pPushable->pev->velocity.Length() > pPushable->MaxSpeed())
			{
				float zSpeed = pPushable->pev->velocity.z;
				pPushable->pev->velocity = pPushable->pev->velocity.Normalize() * pPushable->MaxSpeed();
				if (zSpeed < 0.f && !controller.IsDragging())
				{
					// restore falling speed
					pPushable->pev->velocity.z = max(zSpeed, -g_psv_gravity->value);
				}
			}
		}
	}

	return true;
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
