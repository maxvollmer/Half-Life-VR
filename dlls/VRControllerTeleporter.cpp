
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"

#include "VRControllerTeleporter.h"
#include "VRPhysicsHelper.h"


void VRControllerTeleporter::StartTele(CBasePlayer *pPlayer, const Vector& telePos)
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
}

void VRControllerTeleporter::StopTele(CBasePlayer *pPlayer)
{
	// reset
	pPlayer->vr_didJustTeleportThroughChangeLevel = false;

	if (vr_fValidTeleDestination)
	{
		if (vr_fTelePointsAtXenMound && vr_parabolaBeams[0] != nullptr && gGlobalXenMounds.Has(vr_parabolaBeams[0]->pev->origin))
		{
			gGlobalXenMounds.Trigger(pPlayer, vr_parabolaBeams[0]->pev->origin);
		}
		pPlayer->pev->origin = vr_vecTeleDestination;
		pPlayer->pev->origin.z -= pPlayer->pev->mins.z;
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
}

void VRControllerTeleporter::UpdateTele(CBasePlayer *pPlayer, const Vector& telePos, const Vector& teleDir)
{
	// reset
	pPlayer->vr_didJustTeleportThroughChangeLevel = false;

	if (!vr_pTeleBeam || !vr_pTeleSprite)
	{
		vr_fValidTeleDestination = false;
		vr_fTelePointsAtXenMound = false;
		vr_fTelePointsInWater = false;
		StopTele(pPlayer);
		return;
	}

	TraceResult tr;
	VRPhysicsHelper::Instance().TraceLine(telePos, telePos + teleDir * GetCurrentTeleLength(pPlayer), pPlayer->edict(), &tr);

	Vector beamEndPos = tr.vecEndPos;
	Vector teleportDestination = tr.vecEndPos;

	vr_fValidTeleDestination = CanTeleportHere(pPlayer, tr, telePos, beamEndPos, teleportDestination);

	vr_pTeleBeam->SetStartPos(telePos);
	vr_pTeleBeam->SetEndPos(beamEndPos);

	if (vr_fValidTeleDestination && vr_fTelePointsAtXenMound)
	{
		EnableXenMoundParabolaAndUpdateTeleDestination(pPlayer, telePos, beamEndPos, teleportDestination);
	}
	else
	{
		DisableXenMoundParabola();
	}

	vr_vecTeleDestination = teleportDestination;
	UTIL_SetOrigin(vr_pTeleSprite->pev, teleportDestination);

	bool isTeleporterBlocked =
		!UTIL_CheckClearSight(pPlayer->EyePosition(), telePos, ignore_monsters, dont_ignore_glass, pPlayer->edict())
		|| VRPhysicsHelper::Instance().CheckIfLineIsBlocked(pPlayer->EyePosition(), telePos);

	if (isTeleporterBlocked)
	{
		vr_fValidTeleDestination = false;
	}

	if (vr_fValidTeleDestination)
	{
		vr_pTeleSprite->pev->rendercolor = Vector(0, 255, 0);
		vr_pTeleBeam->pev->rendercolor = Vector(0, 255, 0);

		// Move destination down when head would be in ceiling (e.g. teleporting inside a duct)
		TraceResult tr;
		UTIL_TraceLine(vr_vecTeleDestination, vr_vecTeleDestination + Vector(0, 0, pPlayer->pev->size.z), ignore_monsters, pPlayer->edict(), &tr);
		vr_vecTeleDestination.z -= pPlayer->pev->size.z * (1.0f - tr.flFraction);
	}
	else
	{
		vr_pTeleSprite->pev->rendercolor = Vector(255, 0, 0);
		vr_pTeleBeam->pev->rendercolor = Vector(255, 0, 0);
	}
}


void VRControllerTeleporter::TouchTriggersInTeleportPath(CBasePlayer *pPlayer)
{
	CBaseEntity *pEntity = nullptr;
	while (UTIL_FindEntityByFilter(&pEntity, [](CBaseEntity *pEnt) {return pEnt->pev->solid == SOLID_TRIGGER; }))
	{
		bool fTouched = vr_pTeleBeam && UTIL_TraceBBox(vr_pTeleBeam->pev->origin, vr_pTeleBeam->pev->angles, pEntity->pev->absmin, pEntity->pev->absmax);

		if (!fTouched && vr_pTeleBeam && vr_pTeleSprite && (vr_pTeleSprite->pev->origin - vr_pTeleBeam->pev->angles).Length() > 0.1f)
		{
			fTouched = UTIL_TraceBBox(vr_pTeleBeam->pev->angles, vr_pTeleSprite->pev->origin, pEntity->pev->absmin, pEntity->pev->absmax);
		}

		for (int i = 0; !fTouched && i < VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT; i++)
		{
			if (vr_parabolaBeams[i] != nullptr)
			{
				fTouched = UTIL_TraceBBox(vr_parabolaBeams[i]->pev->origin, vr_parabolaBeams[i]->pev->angles, pEntity->pev->absmin, pEntity->pev->absmax);
			}
		}

		if (fTouched)
		{
			if (FClassnameIs(pEntity->pev, "trigger_changelevel"))
			{
				pPlayer->vr_didJustTeleportThroughChangeLevel = true;
			}
			pEntity->Touch(pPlayer);
		}
	}
}

float VRControllerTeleporter::GetCurrentTeleLength(CBasePlayer *pPlayer)
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

bool VRControllerTeleporter::CanTeleportHere(CBasePlayer *pPlayer, const TraceResult& tr, const Vector& beamStartPos, Vector& beamEndPos, Vector& teleportDestination)
{
	vr_fTelePointsAtXenMound = false;	// reset every frame
	vr_fTelePointsInWater = false;		// reset every frame
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
			if (beamStartPos.z < tr.vecEndPos.z)
			{
				// climb up
				teleportDestination.z = tr.pHit->v.absmax.z;

				// Make sure we don't teleport the player into a ceiling
				float flPlayerHeight = FBitSet(pPlayer->pev->flags, (FL_DUCKING | FL_VR_DUCKING)) ? (VEC_DUCK_HULL_MAX.z - VEC_DUCK_HULL_MIN.z) : (VEC_HULL_MAX.z - VEC_HULL_MIN.z);
				TraceResult tr2;
				VRPhysicsHelper::Instance().TraceLine(teleportDestination, teleportDestination + Vector{ 0, 0, flPlayerHeight }, tr.pHit, &tr2);
				if (tr2.flFraction < 1.f)
				{
					teleportDestination.z -= flPlayerHeight * (1.f - tr2.flFraction);
				}
			}
			else
			{
				// climb down
				teleportDestination.z = tr.pHit->v.absmin.z;

				Vector delta = beamEndPos - beamStartPos;

				if (tr.pHit->v.size.x > tr.pHit->v.size.y)
				{
					if (delta.y < 0)
					{
						teleportDestination.y += 32.0f;
					}
					else
					{
						teleportDestination.y -= 32.0f;
					}
				}
				else
				{
					if (delta.x < 0)
					{
						teleportDestination.x += 32.0f;
					}
					else
					{
						teleportDestination.x -= 32.0f;
					}
				}
			}
			return true;// !UTIL_BBoxIntersectsBSPModel(teleportDestination + Vector(0, 0, -VEC_DUCK_HULL_MIN.z), VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
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
				return true;// !UTIL_BBoxIntersectsBSPModel(teleportDestination + Vector(0, 0, -VEC_DUCK_HULL_MIN.z), VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
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

bool VRControllerTeleporter::TryTeleportInUpwardsTriggerPush(CBasePlayer *pPlayer, const Vector& beamStartPos, Vector& beamEndPos, Vector& teleportDestination)
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

void VRControllerTeleporter::EnableXenMoundParabolaAndUpdateTeleDestination(CBasePlayer *pPlayer, const Vector& beamStartPos, const Vector& beamEndPos, Vector & teleportDestination)
{
	// Set this to false, in case we won't hit anything in the parabola loop (parabola goes into empty space), thus the teleport destination couldn't be determined and is invalid
	vr_fValidTeleDestination = false;

	// Get direction of beam and clamp at -0.1f z direction
	Vector beamDir = (beamEndPos - beamStartPos).Normalize();
	beamDir.z = min(beamDir.z, -0.1f);
	beamDir = beamDir.Normalize();

	// Get direction of parabola at beam end pos
	Vector parabolaDir = beamDir;
	parabolaDir.z = -parabolaDir.z * 4; // Invert and make steeper
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
		CBeam *pCurrentSegmentBeam = nullptr;
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
			z1 = parabolaConstants.x*linearPos1.y*linearPos1.y + parabolaConstants.y*linearPos1.y + parabolaConstants.z;
			z2 = parabolaConstants.x*linearPos2.y*linearPos2.y + parabolaConstants.y*linearPos2.y + parabolaConstants.z;
		}
		else
		{
			z1 = parabolaConstants.x*linearPos1.x*linearPos1.x + parabolaConstants.y*linearPos1.x + parabolaConstants.z;
			z2 = parabolaConstants.x*linearPos2.x*linearPos2.x + parabolaConstants.y*linearPos2.x + parabolaConstants.z;
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
			vr_fValidTeleDestination = CanTeleportHere(pPlayer, tr, beamSegmentPos1, beamSegmentPos2, teleportDestination);

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