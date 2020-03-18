
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "pm_defs.h"
#include "com_model.h"
#include "soundent.h"

#include "VRControllerTeleporter.h"
#include "VRPhysicsHelper.h"


void VRControllerTeleporter::StartTele(CBasePlayer* pPlayer, const Vector& telePos)
{
	// reset
	pPlayer->vr_didJustTeleportThroughChangeLevel = false;

	if (!vr_pTeleSprite)
	{
		vr_pTeleSprite = CSprite::SpriteCreate("sprites/XSpark1.spr", telePos, FALSE);
		vr_pTeleSprite->Spawn();
		vr_pTeleSprite->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
		vr_pTeleSprite->pev->owner = pPlayer->edict();
		vr_pTeleSprite->TurnOn();
		vr_pTeleSprite->pev->effects &= ~EF_NODRAW;
	}

	if (!vr_pTeleBeam)
	{
		vr_pTeleBeam = CBeam::BeamCreate("sprites/xbeam1.spr", 20);
		vr_pTeleBeam->Spawn();
		vr_pTeleBeam->PointsInit(telePos, telePos);
		vr_pTeleBeam->pev->owner = pPlayer->edict();
		vr_pTeleBeam->pev->effects &= ~EF_NODRAW;
	}

	vr_fValidTeleDestination = false;
	vr_fTelePointsAtXenMound = false;
	vr_fTelePointsInWater = false;
	vr_needsToDuckAfterTeleport = false;
}

void VRControllerTeleporter::StopTele(CBasePlayer* pPlayer)
{
	// reset
	pPlayer->vr_didJustTeleportThroughChangeLevel = false;

	if (vr_fValidTeleDestination)
	{
		if (vr_needsToDuckAfterTeleport)
		{
			// we need to duck now!
			extern playermove_t* pmove;
			if (pmove != nullptr)
			{
				pmove->usehull = 1;
				pmove->flags |= FL_DUCKING;
				pmove->bInDuck = true;
				pmove->oldbuttons |= IN_DUCK;
				pmove->cmd.buttons |= IN_DUCK;
			}
			pPlayer->pev->flags |= FL_DUCKING;
			UTIL_SetSize(pPlayer->pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
		}

		if (vr_fTelePointsAtXenMound && vr_parabolaBeams[0] != nullptr && gGlobalXenMounds.Has(vr_parabolaBeams[0]->pev->origin))
		{
			gGlobalXenMounds.Trigger(pPlayer, vr_parabolaBeams[0]->pev->origin);
		}

		// Create sound
		float distanceTeleported = (vr_vecTeleDestination - pPlayer->pev->origin).Length();
		CSoundEnt::InsertSound(bits_SOUND_PLAYER, pPlayer->pev->origin, int(distanceTeleported), 0.2f);

		pPlayer->pev->origin = vr_vecTeleDestination;
		pPlayer->pev->origin.z -= pPlayer->pev->mins.z;
		pPlayer->pev->origin.z += 1.f;
		UTIL_SetOrigin(pPlayer->pev, pPlayer->pev->origin);
		pPlayer->pev->absmin = pPlayer->pev->origin + pPlayer->pev->mins;
		pPlayer->pev->absmax = pPlayer->pev->origin + pPlayer->pev->maxs;
		TouchTriggersInTeleportPath(pPlayer);


		// used to disable gravity in water when using VR teleporter
		extern bool g_vrTeleportInWater;
		g_vrTeleportInWater = vr_fTelePointsInWater;
	}
	if (vr_pTeleSprite)
	{
		UTIL_Remove(vr_pTeleSprite);
		vr_pTeleSprite = nullptr;
	}
	if (vr_pTeleBeam)
	{
		UTIL_Remove(vr_pTeleBeam);
		vr_pTeleBeam = nullptr;
	}
	DisableXenMoundParabola();
	vr_fValidTeleDestination = false;
	vr_fTelePointsAtXenMound = false;
	vr_fTelePointsInWater = false;
	vr_needsToDuckAfterTeleport = false;
}

void VRControllerTeleporter::UpdateTele(CBasePlayer* pPlayer, const Vector& telePos, const Vector& teleDir)
{
	// reset
	pPlayer->vr_didJustTeleportThroughChangeLevel = false;

	if (!vr_pTeleBeam || !vr_pTeleSprite)
	{
		vr_fValidTeleDestination = false;
		vr_fTelePointsAtXenMound = false;
		vr_fTelePointsInWater = false;
		vr_needsToDuckAfterTeleport = false;
		StopTele(pPlayer);
		return;
	}

	TraceResult tr;
	VRPhysicsHelper::Instance().TraceLine(telePos, telePos + teleDir * GetCurrentTeleLength(pPlayer), pPlayer->edict(), &tr);

	Vector beamEndPos = tr.vecEndPos;
	Vector teleportDestination = tr.vecEndPos;
	bool needsToDuck = false;

	vr_fValidTeleDestination = CanTeleportHere(pPlayer, tr, telePos, beamEndPos, teleportDestination, needsToDuck);

	vr_pTeleBeam->SetStartPos(telePos);
	vr_pTeleBeam->SetEndPos(beamEndPos);

	if (vr_fValidTeleDestination && vr_fTelePointsAtXenMound)
	{
		EnableXenMoundParabolaAndUpdateTeleDestination(pPlayer, telePos, beamEndPos, teleportDestination, needsToDuck);
	}
	else
	{
		DisableXenMoundParabola();
	}

	vr_vecTeleDestination = teleportDestination + Vector{ 0.f, 0.f, 1.f };
	UTIL_SetOrigin(vr_pTeleSprite->pev, teleportDestination);

	bool isTeleporterBlocked =
		!UTIL_CheckClearSight(pPlayer->EyePosition(), telePos, ignore_monsters, dont_ignore_glass, pPlayer->edict()) || VRPhysicsHelper::Instance().CheckIfLineIsBlocked(pPlayer->EyePosition(), telePos);

	if (isTeleporterBlocked)
	{
		vr_fValidTeleDestination = false;
		vr_needsToDuckAfterTeleport = false;
	}

	if (vr_fValidTeleDestination)
	{
		vr_pTeleSprite->pev->rendercolor = Vector(0, 255, 0);
		vr_pTeleBeam->pev->rendercolor = Vector(0, 255, 0);

		// Check if we need to duck for this destination (e.g. teleporting inside a duct)
		vr_needsToDuckAfterTeleport = needsToDuck;
		if (!vr_needsToDuckAfterTeleport)
		{
			TraceResult trStanding;
			Vector vecTeleDestinationHeadPos = vr_vecTeleDestination;
			vecTeleDestinationHeadPos.z += VEC_HULL_MAX.z - VEC_HULL_MIN.z;
			UTIL_TraceLine(vr_vecTeleDestination, vecTeleDestinationHeadPos, ignore_monsters, pPlayer->edict(), &trStanding);
			if (trStanding.flFraction < 1.f)
			{
				vr_needsToDuckAfterTeleport = true;
				// Check if head would be in ceiling even when ducking and move destination down
				TraceResult trDucking;
				Vector vecTeleDestinationDuckedHeadPos = vr_vecTeleDestination;
				vecTeleDestinationDuckedHeadPos.z += VEC_DUCK_HULL_MAX.z - VEC_DUCK_HULL_MIN.z;
				UTIL_TraceLine(vr_vecTeleDestination, vecTeleDestinationDuckedHeadPos, ignore_monsters, pPlayer->edict(), &trDucking);
				if (trDucking.flFraction < 1.f)
				{
					// Can't even duck here - This is probably an invalid destination!
					vr_fValidTeleDestination = false;
				}
			}
		}
	}

	if (!vr_fValidTeleDestination)
	{
		vr_pTeleSprite->pev->rendercolor = Vector(255, 0, 0);
		vr_pTeleBeam->pev->rendercolor = Vector(255, 0, 0);
	}
}


void VRControllerTeleporter::TouchTriggersInTeleportPath(CBasePlayer* pPlayer)
{
	for (int index = 1; index < gpGlobals->maxEntities; index++)
	{
		edict_t* pent = INDEXENT(index);
		if (FNullEnt(pent))
			continue;

		if (pent->v.solid  != SOLID_TRIGGER)
			continue;

		EHANDLE<CBaseEntity> hEntity = CBaseEntity::SafeInstance<CBaseEntity>(pent);
		if (!hEntity)
			continue;

		// Don't touch xen jump triggers with the teleporter
		if (hEntity->IsXenJumpTrigger())
			continue;

		bool fTouched = vr_pTeleBeam && UTIL_TraceBBox(vr_pTeleBeam->pev->origin, vr_pTeleBeam->pev->angles, hEntity->pev->absmin, hEntity->pev->absmax);

		if (!fTouched && vr_pTeleBeam && vr_pTeleSprite && (vr_pTeleSprite->pev->origin - vr_pTeleBeam->pev->angles).Length() > 0.1f)
		{
			fTouched = UTIL_TraceBBox(vr_pTeleBeam->pev->angles, vr_pTeleSprite->pev->origin, hEntity->pev->absmin, hEntity->pev->absmax);
		}

		for (int i = 0; !fTouched && i < VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT; i++)
		{
			if (vr_parabolaBeams[i] != nullptr)
			{
				fTouched = UTIL_TraceBBox(vr_parabolaBeams[i]->pev->origin, vr_parabolaBeams[i]->pev->angles, hEntity->pev->absmin, hEntity->pev->absmax);
			}
		}

		if (fTouched)
		{
			if (FClassnameIs(hEntity->pev, "trigger_changelevel"))
			{
				pPlayer->vr_didJustTeleportThroughChangeLevel = true;
			}
			hEntity->Touch(pPlayer);
		}
	}
}

float VRControllerTeleporter::GetCurrentTeleLength(CBasePlayer* pPlayer)
{
	if (pPlayer->pev->waterlevel > 1)
	{
		return 100;
	}
	else
	{
		return 250;
	}
}

constexpr const int VR_MIN_LADDER_TOUCH_DEPTH = -16;
constexpr const int VR_MAX_LADDER_TOUCH_DEPTH = 4;

bool VRControllerTeleporter::CanTeleportHere(CBasePlayer* pPlayer, const TraceResult& tr, const Vector& beamStartPos, Vector& beamEndPos, Vector& teleportDestination, bool& needsToDuck)
{
	vr_fTelePointsAtXenMound = false;  // reset every frame
	vr_fTelePointsInWater = false;  // reset every frame
	if (!tr.fAllSolid)
	{
		// Detect water
		if (UTIL_PointContents(tr.vecEndPos) == CONTENTS_WATER)
		{
			// Reduce distance a bit to avoid walls
			Vector delta = beamEndPos - beamStartPos;
			if (delta.Length() < 32.0f)
			{
				teleportDestination = beamEndPos = beamStartPos;
			}
			else
			{
				teleportDestination = beamEndPos = beamEndPos - (delta.Normalize() * 32.0f);
			}
			vr_fTelePointsInWater = true;
			return true;
		}
		else if (pPlayer->pev->waterlevel > 0 && UTIL_PointContents(beamStartPos) == CONTENTS_WATER)
		{
			teleportDestination = beamEndPos = UTIL_WaterLevelPos(beamStartPos, beamEndPos);
			vr_fTelePointsInWater = true;
			return true;
		}
		// Detect ladders
		else if (tr.pHit != nullptr && FClassnameIs(tr.pHit, "func_ladder"))
		{
			for (int ladderTouchDepth = VR_MAX_LADDER_TOUCH_DEPTH; ladderTouchDepth >= VR_MIN_LADDER_TOUCH_DEPTH; ladderTouchDepth--)
			{
				Vector ladderHit = VecBModelOrigin(&tr.pHit->v);
				ladderHit.z = tr.vecEndPos.z;

				if (tr.pHit->v.size.x < tr.pHit->v.size.y)
				{
					if (beamEndPos.x < ladderHit.x)
						ladderHit.x = tr.pHit->v.absmin.x + ladderTouchDepth + pPlayer->pev->mins.x;
					else
						ladderHit.x = tr.pHit->v.absmax.x - ladderTouchDepth + pPlayer->pev->maxs.x;
				}
				else
				{
					if (beamEndPos.y < ladderHit.y)
						ladderHit.y = tr.pHit->v.absmin.y + ladderTouchDepth + pPlayer->pev->mins.y;
					else
						ladderHit.y = tr.pHit->v.absmax.y - ladderTouchDepth + pPlayer->pev->maxs.y;
				}
				teleportDestination = beamEndPos = ladderHit;

				extern playermove_t* pmove;
				if (pmove && pmove->numphysent > 1 && pmove->physents[0].model->needload == 0)
				{
					while (ladderHit.z > tr.pHit->v.absmin.z)
					{
						int blockingEnt = pmove->PM_TestPlayerPosition(ladderHit, nullptr);
						bool isBlocked = blockingEnt >= 0;
						bool isBlockedDucking = isBlocked;
						if (isBlocked && pmove->usehull == 0)
						{
							// check ducking
							pmove->usehull = 1;
							isBlockedDucking = pmove->PM_TestPlayerPosition(ladderHit, nullptr) >= 0;
							pmove->usehull = 0;
						}
						if (isBlockedDucking)
						{
							if (ladderHit.z == tr.vecEndPos.z && (ladderHit.z > (tr.pHit->v.absmax.z - VEC_DUCK_HEIGHT)))
							{
								ladderHit.z = tr.pHit->v.absmax.z - VEC_DUCK_HEIGHT;
							}
							else
							{
								ladderHit.z -= 8.f;
							}
						}
						else
						{
							if (isBlocked)
							{
								needsToDuck = true;
							}
							teleportDestination = beamEndPos = ladderHit;
							return true;
						}
					}
				}
			}

			return false;
		}
		else if (tr.flFraction < 1.0f)
		{
			Vector dir = (beamEndPos - beamStartPos).Normalize();
			const char* texturename = TRACE_TEXTURE(tr.pHit != nullptr ? tr.pHit : INDEXENT(0), beamStartPos, beamEndPos + dir * 32.f);

			// Detect Xen jump pads
			if (FStrEq(texturename, "c2a5mound") && gGlobalXenMounds.Has(beamEndPos))
			{
				vr_fTelePointsAtXenMound = true;
				return true;
			}
			// Normal floor
			else if (DotProduct(Vector(0, 0, 1), tr.vecPlaneNormal) > 0.2f)
			{
				teleportDestination = beamEndPos;
				return true;  // !UTIL_BBoxIntersectsBSPModel(teleportDestination + Vector(0, 0, -VEC_DUCK_HULL_MIN.z), VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
			}
			// else wall, ceiling or other surface we can't teleport on
		}
		// else beam ended in air, check if we are in an upwards trigger_push
		else if (TryTeleportInUpwardsTriggerPush(pPlayer, beamStartPos, beamEndPos, teleportDestination))
		{
			return true;
		}
	}
	return false;
}

bool VRControllerTeleporter::TryTeleportInUpwardsTriggerPush(CBasePlayer* pPlayer, const Vector& beamStartPos, Vector& beamEndPos, Vector& teleportDestination)
{
	// Don't teleport downwards in upwards trigger_push
	if (beamEndPos.z <= beamStartPos.z)
		return false;

	CBaseEntity* pTriggerPush = pPlayer->GetCurrentUpwardsTriggerPush();

	if (pTriggerPush)
	{
		if (UTIL_PointInsideBBox(beamEndPos, pTriggerPush->pev->absmin, pTriggerPush->pev->absmax))
		{
			teleportDestination = beamEndPos;
			return true;
		}
		else
		{
			Vector result;
			if (UTIL_GetLineIntersectionWithBBox(beamEndPos, beamStartPos, pTriggerPush->pev->absmin, pTriggerPush->pev->absmax, result))
			{
				teleportDestination = beamEndPos = result;
				return true;
			}
		}
	}

	return false;
}

void VRControllerTeleporter::EnableXenMoundParabolaAndUpdateTeleDestination(CBasePlayer* pPlayer, const Vector& beamStartPos, const Vector& beamEndPos, Vector& teleportDestination, bool& needsToDuck)
{
	// Set this to false, in case we won't hit anything in the parabola loop (parabola goes into empty space), thus the teleport destination couldn't be determined and is invalid
	vr_fValidTeleDestination = false;
	needsToDuck = false;

	// Get direction of beam and clamp at -0.1f z direction
	Vector beamDir = (beamEndPos - beamStartPos).Normalize();
	beamDir.z = min(beamDir.z, -0.1f);
	beamDir = beamDir.Normalize();

	// Get direction of parabola at beam end pos
	Vector parabolaDir = beamDir;
	parabolaDir.z = -parabolaDir.z * 4;  // Invert and make steeper
	parabolaDir = parabolaDir.Normalize();

	// Get vertex of parabola (approximation, but good enough for our purpose)
	Vector parabolaVertex = beamEndPos + (parabolaDir * XEN_MOUND_PARABOLA_LENGTH * 0.5f);

	// Get third parabola point
	Vector thirdParabolaPoint = parabolaVertex + (beamDir * XEN_MOUND_PARABOLA_LENGTH * 0.5f);

	// Vector for holding A B and C of parabola function: y = A*x*x + B*x + C
	Vector parabolaConstants;

	// Decide wether to use x,z or y,z for parabola calculations
	bool fUseYForParabolaX = fabs(parabolaDir.y) > fabs(parabolaDir.x);
	if (fUseYForParabolaX)
	{
		UTIL_ParabolaFromPoints(Vector2D(beamEndPos.y, beamEndPos.z), Vector2D(parabolaVertex.y, parabolaVertex.z), Vector2D(thirdParabolaPoint.y, thirdParabolaPoint.z), parabolaConstants);
	}
	else
	{
		UTIL_ParabolaFromPoints(Vector2D(beamEndPos.x, beamEndPos.z), Vector2D(parabolaVertex.x, parabolaVertex.z), Vector2D(thirdParabolaPoint.x, thirdParabolaPoint.z), parabolaConstants);
	}

	// Get length of parabola flattened into horizontal plane (linear projection)
	float parabolaFlattenedLength = (thirdParabolaPoint - beamEndPos).Length();
	float parabolaFlattenedSegmentLength = parabolaFlattenedLength / VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT;

	// Get parabola linear projection direction vector
	Vector parabolaFlattenedLinearDirection = (thirdParabolaPoint - beamEndPos).Normalize();

	// Get positions of parabola beam segments along linear parabola projection with parabola function
	for (int i = 0; i < VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT; i++)
	{
		// If necessary create the beam, and enable it
		CBeam* pCurrentSegmentBeam = nullptr;
		if (vr_parabolaBeams[i] == nullptr)
		{
			pCurrentSegmentBeam = CBeam::BeamCreate("sprites/xbeam1.spr", 20);
			pCurrentSegmentBeam->Spawn();
			pCurrentSegmentBeam->PointsInit(pPlayer->pev->origin, pPlayer->pev->origin);
			pCurrentSegmentBeam->pev->owner = pPlayer->edict();
			vr_parabolaBeams[i] = pCurrentSegmentBeam;
		}
		else
		{
			pCurrentSegmentBeam = vr_parabolaBeams[i];
		}
		pCurrentSegmentBeam->pev->effects &= ~EF_NODRAW;

		// Get positions on linear projection
		Vector linearPos1 = beamEndPos + (parabolaFlattenedLinearDirection * i * parabolaFlattenedSegmentLength);
		Vector linearPos2 = beamEndPos + (parabolaFlattenedLinearDirection * (i + 1) * parabolaFlattenedSegmentLength);

		// Determine z position of parabola in 3d space (y in 2d space determined by parabola function)
		float z1, z2;
		if (fUseYForParabolaX)
		{
			z1 = parabolaConstants.x * linearPos1.y * linearPos1.y + parabolaConstants.y * linearPos1.y + parabolaConstants.z;
			z2 = parabolaConstants.x * linearPos2.y * linearPos2.y + parabolaConstants.y * linearPos2.y + parabolaConstants.z;
		}
		else
		{
			z1 = parabolaConstants.x * linearPos1.x * linearPos1.x + parabolaConstants.y * linearPos1.x + parabolaConstants.z;
			z2 = parabolaConstants.x * linearPos2.x * linearPos2.x + parabolaConstants.y * linearPos2.x + parabolaConstants.z;
		}

		// Merge values into valid 3d space positions for the beam segment
		Vector beamSegmentPos1(linearPos1.x, linearPos1.y, z1);
		Vector beamSegmentPos2(linearPos2.x, linearPos2.y, z2);

		if (i == VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT - 1)
		{
			// If this is the last beam segment, extend it to "infinity"
			Vector dir = (beamSegmentPos2 - beamSegmentPos1).Normalize();
			beamSegmentPos2 = beamSegmentPos1 + (dir * 8192.f);
		}

		// Now do a trace to see if we hit something!
		TraceResult tr;
		VRPhysicsHelper::Instance().TraceLine(beamSegmentPos1, beamSegmentPos2, pPlayer->edict(), &tr);

		if (tr.flFraction < 1.0f)
		{
			// We hit something, update beam segment end position
			teleportDestination = beamSegmentPos2 = tr.vecEndPos;

			// Determine if this is a valid position
			vr_fValidTeleDestination = CanTeleportHere(pPlayer, tr, beamSegmentPos1, beamSegmentPos2, teleportDestination, needsToDuck);

			// Hide all further beams (and increment i, so this loop exits)
			for (i++; i < VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT; i++)
			{
				if (vr_parabolaBeams[i] != nullptr)
				{
					vr_parabolaBeams[i]->pev->effects |= EF_NODRAW;
				}
			}
		}
		else
		{
			// Move teleport destination to end of segment, so sprite will be drawn correctly even if we exit the loop without finding a correct spot
			teleportDestination = beamSegmentPos2;
		}

		// Set beam segment positions
		pCurrentSegmentBeam->SetStartPos(beamSegmentPos1);
		pCurrentSegmentBeam->SetEndPos(beamSegmentPos2);
	}

	// Set color of parabola beam
	for (int i = 0; i < VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT; i++)
	{
		if (vr_parabolaBeams[i] != nullptr)
		{
			if (vr_fValidTeleDestination)
			{
				vr_parabolaBeams[i]->pev->rendercolor = Vector(0, 255, 0);
			}
			else
			{
				vr_parabolaBeams[i]->pev->rendercolor = Vector(255, 0, 0);
			}
		}
	}
}

void VRControllerTeleporter::DisableXenMoundParabola()
{
	for (int i = 0; i < VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT; i++)
	{
		if (vr_parabolaBeams[i] != nullptr)
		{
			UTIL_Remove(vr_parabolaBeams[i]);
			vr_parabolaBeams[i] = nullptr;
		}
	}
}