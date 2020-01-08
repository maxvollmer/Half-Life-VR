
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "com_model.h"

#include "VRMovementHandler.h"

constexpr const byte VR_MOVEMENT_FAKE_FRAMETIME = 10;

Vector VRMovementHandler::DoMovement(const Vector& from, const Vector& to, CBaseEntity* pMovingEntityForTouch /*= nullptr*/)
{
	extern playermove_t* pmove;
	extern void PM_PlayerMove(qboolean server);
	extern bool VRGlobalGetNoclipMode();

	// we can't move if pmove is nullptr (should never happen)
	if (!pmove)
		return from;

	// we can't move if map isn't loaded yet
	if (pmove->physents[0].model->needload != 0)
		return from;

	// we can't move if we are dead
	if (pmove->dead)
		return from;

	// unlikely, but not impossible
	if (from == to)
		return from;

	// in noclip mode we can always move
	if (VRGlobalGetNoclipMode())
		return to;

	// if "to" is free, we can move there
	if (pmove->PM_TestPlayerPosition(to, nullptr) == -1)
		return to;

	// if "from" is blocked, we can't move (let normal pm_shared unstuck as first)
	if (pmove->PM_TestPlayerPosition(from, nullptr) >= 0)
		return from;

	// if we end up here, noclip is off, from is free, and to is blocked - use pm_shared code to see if we can move (up stairs, along walls etc.)

	// make backup of pmove
	playermove_t pmovebackup;
	std::memcpy(&pmovebackup, pmove, sizeof(playermove_t));

	// make backup of accelerate settings and set them to instant
	float vrMoveInstantAccelerateBackup = CVAR_GET_FLOAT("vr_move_instant_accelerate");
	float vrMoveInstantDecelerateBackup = CVAR_GET_FLOAT("vr_move_instant_decelerate");
	CVAR_SET_FLOAT("vr_move_instant_accelerate", 1.f);
	CVAR_SET_FLOAT("vr_move_instant_decelerate", 1.f);

	// get move direction and distance (length)
	Vector moveDir = (to - from);
	moveDir.z = 0.f;  // clear any z (there shouldn't be any z, but still)
	float moveDist = moveDir.Length();
	moveDir = moveDir.Normalize();

	// increase maxspeed so movement won't be capped
	pmove->clientmaxspeed = 100000.f;
	pmove->maxspeed = 100000.f;

	// set origin to from
	pmove->origin = from;

	// Set viewangles so that forward movement is aligned with moveDir
	pmove->cmd.viewangles = UTIL_VecToAngles(moveDir);
	pmove->angles = pmove->cmd.viewangles;
	pmove->oldangles = pmove->cmd.viewangles;

	// clear punchangle to avoid any issues
	pmove->punchangle = Vector{};

	// Set frametime to something consistent
	pmove->cmd.msec = VR_MOVEMENT_FAKE_FRAMETIME;

	// these shouldn't be set anyways, but just to make sure - disable spectator
	pmove->spectator = 0;
	pmove->iuser1 = 0;

	// set movement input
	pmove->cmd.buttons &= IN_DUCK;                                                 // keep IN_DUCK, if set
	pmove->cmd.buttons |= IN_FORWARD;                                              // set IN_FORWARD
	pmove->cmd.buttons_ex &= (X_IN_VRDUCK | X_IN_LETLADDERGO);                     // keep X_IN_VRDUCK and X_IN_LETLADDERGO, if set
	pmove->cmd.forwardmove = moveDist * (1000 / (int)VR_MOVEMENT_FAKE_FRAMETIME);  // set velocity to match frametime and moveDist (so movement code calculates the exact moveDist)
	pmove->cmd.sidemove = 0.f;
	pmove->cmd.upmove = 0.f;

	// call movement code
	PM_PlayerMove(true);

	// get new position calculated by movement code
	Vector result = pmove->origin;

	// handle touched entities!
	if (pMovingEntityForTouch)
	{
		Vector backupVelocity = pMovingEntityForTouch->pev->velocity;
		pMovingEntityForTouch->pev->velocity = (to - from) * (1000 / (int)VR_MOVEMENT_FAKE_FRAMETIME);
		for (int i = 0; i < pmove->numtouch; i++)
		{
			EHANDLE<CBaseEntity> hTouched = CBaseEntity::SafeInstance<CBaseEntity>(g_engfuncs.pfnPEntityOfEntIndex(pmove->physents[pmove->touchindex[i].ent].info));
			if (hTouched)
			{
				hTouched->Touch(pMovingEntityForTouch);
			}
		}
		pMovingEntityForTouch->pev->velocity = backupVelocity;
	}

	// restore backup
	std::memcpy(pmove, &pmovebackup, sizeof(playermove_t));

	// restore accelerate settings
	CVAR_SET_FLOAT("vr_move_instant_accelerate", vrMoveInstantAccelerateBackup);
	CVAR_SET_FLOAT("vr_move_instant_decelerate", vrMoveInstantDecelerateBackup);

	// return result
	//ALERT(at_console, "DoMovement: from: %f %f %f; to: %f %f %f; result: %f %f %f\n", from.x, from.y, from.z, to.x, to.y, to.z, result.x, result.y, result.z);
	return result;
}
