
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

	if (!vr_hTeleSprite)
	{
		vr_hTeleSprite = CSprite::SpriteCreate("sprites/XSpark1.spr", telePos, FALSE);
		vr_hTeleSprite->Spawn();
		vr_hTeleSprite->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
		vr_hTeleSprite->pev->owner = pPlayer->edict();
		vr_hTeleSprite->TurnOn();
	}
	vr_hTeleSprite->pev->effects &= ~EF_NODRAW;

	ClearTeleportParabola();
	vr_teleportResult = TeleportResult::INVALID;
	vr_vecTeleDestination = telePos;
	vr_needsToDuckAfterTeleport = false;
	vr_teleportedXenMoundOrigins.clear();
}

void VRControllerTeleporter::StopTele(CBasePlayer* pPlayer)
{
	// reset
	pPlayer->vr_didJustTeleportThroughChangeLevel = false;

	if (vr_teleportResult != TeleportResult::INVALID)
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

		for (auto& xenMoundOrigin : vr_teleportedXenMoundOrigins)
		{
			if (gGlobalXenMounds.Has(xenMoundOrigin))
			{
				gGlobalXenMounds.Trigger(pPlayer, xenMoundOrigin);
			}
		}

		if (vr_teleportResult == TeleportResult::TRIGGER_TELEPORT)
		{
			if (vr_hTriggerTeleport)
			{
				vr_hTriggerTeleport->Touch(pPlayer);
				vr_hTriggerTeleport = nullptr;
			}
		}
		else
		{
			Vector prevOrigin = pPlayer->pev->origin;
			Vector prevAngles = pPlayer->pev->angles;

			pPlayer->pev->origin = vr_vecTeleDestination;
			pPlayer->pev->origin.z -= pPlayer->pev->mins.z;
			pPlayer->pev->origin.z += 1.f;
			UTIL_SetOrigin(pPlayer->pev, pPlayer->pev->origin);
			pPlayer->pev->absmin = pPlayer->pev->origin + pPlayer->pev->mins;
			pPlayer->pev->absmax = pPlayer->pev->origin + pPlayer->pev->maxs;

			pPlayer->VRJustTeleported(prevOrigin, prevAngles);
		}

		TouchTriggersInTeleportPath(pPlayer);

		// Create "sound" for AI
		// (not actually a sound that plays back, this notifies monsters so players can't use the teleporter to sneak)
		float distanceTeleported = (vr_vecTeleDestination - pPlayer->pev->origin).Length();
		CSoundEnt::InsertSound(bits_SOUND_PLAYER, pPlayer->pev->origin, int(distanceTeleported), 0.2f);

		// used to disable gravity in water when using VR teleporter
		extern bool g_vrTeleportInWater;
		g_vrTeleportInWater = vr_teleportResult == TeleportResult::WATER;
	}

	if (vr_hTeleSprite)
	{
		UTIL_Remove(vr_hTeleSprite);
		vr_hTeleSprite = nullptr;
	}

	ClearTeleportParabola();
	vr_teleportResult = TeleportResult::INVALID;
	vr_vecTeleDestination = Vector{};
	vr_needsToDuckAfterTeleport = false;
	vr_teleportedXenMoundOrigins.clear();
}

void VRControllerTeleporter::UpdateTele(CBasePlayer* pPlayer, const Vector& telePos, const Vector& wishTeleDir)
{
	// reset
	pPlayer->vr_didJustTeleportThroughChangeLevel = false;

	if (!vr_hTeleSprite)
	{
		vr_teleportResult = TeleportResult::INVALID;
		vr_needsToDuckAfterTeleport = false;
		StopTele(pPlayer);
		return;
	}

	if (wishTeleDir.z > 0.f && wishTeleDir.Length2D() < EPSILON)
	{
		vr_teleportResult = TeleportResult::INVALID;
		vr_needsToDuckAfterTeleport = false;
		StopTele(pPlayer);
		return;
	}

	Vector teleDir = wishTeleDir;
	if (teleDir.z > 0.5f)
	{
		teleDir.z = 0.5f;
		float length2d = teleDir.Length2D();
		teleDir.x = teleDir.x / length2d;
		teleDir.y = teleDir.y / length2d;
	}
	teleDir = teleDir.Normalize();

	vr_teleportParabolaSegmentCounter = 0;

	Vector teleportDestination;
	bool needsToDuck = false;
	vr_teleportResult = TeleportParabola(pPlayer, telePos, teleDir, VR_TELEPORT_PARABOLA_LENGTH, teleportDestination, needsToDuck);

	if (vr_teleportResult == TeleportResult::XEN_MOUND)
	{
		ParabolaSegment& lastSegment = vr_teleParabola.segments[vr_teleParabola.lastSegment];
		EnableXenMoundParabolaAndUpdateTeleDestination(pPlayer,
			lastSegment.startPoint,
			lastSegment.endPoint,
			teleportDestination, needsToDuck);
	}

	if (vr_teleportResult == TeleportResult::TRIGGER_TELEPORT && vr_hTriggerTeleport)
	{
		vr_hTeleSprite->pev->effects |= EF_NODRAW;
		ColorTeleporter(Vector(255, 255, 0));
		return;
	}
	vr_hTeleSprite->pev->effects &= ~EF_NODRAW;

	vr_vecTeleDestination = teleportDestination + Vector{ 0.f, 0.f, 1.f };
	UTIL_SetOrigin(vr_hTeleSprite->pev, teleportDestination);

	if (vr_teleportResult != TeleportResult::INVALID)
	{
		bool isTeleporterBlocked =
			GetDownwardsParabolaLength() > VR_TELEPORT_MAX_DOWNWARDS_TELE_LENGTH
			|| !UTIL_CheckClearSight(pPlayer->EyePosition(), telePos, ignore_monsters, dont_ignore_glass, pPlayer->edict()) || VRPhysicsHelper::Instance().CheckIfLineIsBlocked(pPlayer->EyePosition(), telePos);

		if (isTeleporterBlocked)
		{
			vr_teleportResult = TeleportResult::INVALID;
		}
	}

	if (vr_teleportResult != TeleportResult::INVALID)
	{
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
					vr_teleportResult = TeleportResult::INVALID;
				}
			}
		}
	}

	ColorTeleporter((vr_teleportResult != TeleportResult::INVALID) ? Vector(0, 255, 0) : Vector(255, 0, 0));
}


EHANDLE<CBaseEntity> CheckTriggerTeleport(CBasePlayer* pPlayer, const Vector& from, const Vector& to)
{
	for (int index = 1; index < gpGlobals->maxEntities; index++)
	{
		edict_t* pent = INDEXENT(index);
		if (FNullEnt(pent))
			continue;

		if (pent->v.solid != SOLID_TRIGGER)
			continue;

		if (FBitSet(pent->v.spawnflags, SF_TRIGGER_NOCLIENTS))
			continue;

		if (!FClassnameIs(&pent->v, "trigger_teleport"))
			continue;

		if (!UTIL_TraceBBox(from, to, pent->v.absmin, pent->v.absmax))
			continue;

		edict_t* pentTarget = FIND_ENTITY_BY_TARGETNAME(nullptr, STRING(pent->v.target));
		if (FNullEnt(pentTarget))
			continue;

		EHANDLE<CBaseEntity> hEntity = CBaseEntity::SafeInstance<CBaseEntity>(pent);
		if (!hEntity)
			continue;

		return hEntity;
	}

	return nullptr;
}

void VRControllerTeleporter::TouchTriggersInTeleportPath(CBasePlayer* pPlayer)
{
	for (int index = 1; index < gpGlobals->maxEntities; index++)
	{
		edict_t* pent = INDEXENT(index);
		if (FNullEnt(pent))
			continue;

		if (pent->v.solid != SOLID_TRIGGER)
			continue;

		EHANDLE<CBaseEntity> hEntity = CBaseEntity::SafeInstance<CBaseEntity>(pent);
		if (!hEntity)
			continue;

		// Don't touch xen jump triggers with the teleporter
		if (hEntity->IsXenJumpTrigger())
			continue;

		// We handle trigger_teleport separately
		if (FClassnameIs(hEntity->pev, "trigger_teleport"))
			continue;

		for (ParabolaSegment& segment : vr_teleParabola.segments)
		{
			if (segment.beam && !FBitSet(segment.beam->pev->effects, EF_NODRAW))
			{
				if (UTIL_TraceBBox(segment.startPoint, segment.endPoint, hEntity->pev->absmin, hEntity->pev->absmax))
				{
					if (FClassnameIs(hEntity->pev, "trigger_changelevel"))
					{
						pPlayer->vr_didJustTeleportThroughChangeLevel = true;
					}
					hEntity->Touch(pPlayer);
					break;
				}
			}
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

VRControllerTeleporter::TeleportResult VRControllerTeleporter::CanTeleportHere(CBasePlayer* pPlayer, const TraceResult& tr, const Vector& beamStartPos, Vector& beamEndPos, Vector& teleportDestination, bool& needsToDuck)
{
	if (vr_hTriggerTeleport)
		return TeleportResult::TRIGGER_TELEPORT;

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
			return TeleportResult::WATER;
		}
		else if (pPlayer->pev->waterlevel > 0 && UTIL_PointContents(beamStartPos) == CONTENTS_WATER)
		{
			teleportDestination = beamEndPos = UTIL_WaterLevelPos(beamStartPos, beamEndPos);
			return TeleportResult::WATER;
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
							return TeleportResult::VALID;
						}
					}
				}
			}

			return TeleportResult::INVALID;
		}
		else if (tr.flFraction < 1.0f)
		{
			Vector dir = (beamEndPos - beamStartPos).Normalize();
			const char* texturename = TRACE_TEXTURE(tr.pHit != nullptr ? tr.pHit : INDEXENT(0), beamStartPos, beamEndPos + dir * 32.f);

			// Detect Xen jump pads
			if (FStrEq(texturename, "c2a5mound") && gGlobalXenMounds.Has(beamEndPos))
			{
				return TeleportResult::XEN_MOUND;
			}
			// Normal floor
			else if (DotProduct(Vector(0, 0, 1), tr.vecPlaneNormal) > 0.2f)
			{
				teleportDestination = beamEndPos;
				return TeleportResult::VALID;  // !UTIL_BBoxIntersectsBSPModel(teleportDestination + Vector(0, 0, -VEC_DUCK_HULL_MIN.z), VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
			}
			// else wall, ceiling or other surface we can't teleport on
		}
		// else beam ended in air, check if we are in an upwards trigger_push
		else if (TryTeleportInUpwardsTriggerPush(pPlayer, beamStartPos, beamEndPos, teleportDestination))
		{
			return TeleportResult::VALID;
		}
	}
	return TeleportResult::INVALID;
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

void VRControllerTeleporter::ClearTeleportParabola()
{
	for (auto& segment : vr_teleParabola.segments)
	{
		UTIL_Remove(segment.beam);
	}
	vr_teleParabola = Parabola{};
}

float VRControllerTeleporter::GetDownwardsParabolaLength()
{
	float length = 0.f;
	for (auto& segment : vr_teleParabola.segments)
	{
		if (segment.beam && !FBitSet(segment.beam->pev->effects, EF_NODRAW))
		{
			if (segment.startPoint.z > segment.endPoint.z)
			{
				length += (segment.startPoint - segment.endPoint).Length();
			}
		}
	}
	return length;
}

void VRControllerTeleporter::ExtendTeleportParabola(CBasePlayer* pPlayer, const Vector& parabolaStartPos, const Vector& parabolaDir, float length)
{
	// Get direction of parabola at beam end pos
	Vector parabolaEndDir = parabolaDir;
	parabolaEndDir.z = -std::fabsf(parabolaEndDir.z) - 0.25f;
	float length2d = parabolaEndDir.Length2D();
	if (length2d > 0.75f)
	{
		parabolaEndDir.x = parabolaEndDir.x / length2d * 0.75f;
		parabolaEndDir.y = parabolaEndDir.y / length2d * 0.75f;
	}
	parabolaEndDir = parabolaEndDir.Normalize();

	extern cvar_t* g_psv_gravity;
	float gravityAdjustedLength;
	if (g_psv_gravity->value >= 400.f && g_psv_gravity->value <= 1600.f)
		gravityAdjustedLength = length * 800.f / g_psv_gravity->value;
	else if (g_psv_gravity->value < 400.f)
		gravityAdjustedLength = length * 2.f;
	else // gravity > 1600
		gravityAdjustedLength = length * 0.5f;

	// Get vertex of parabola (approximation, but good enough for our purpose)
	Vector parabolaVertex = parabolaStartPos + (parabolaDir * gravityAdjustedLength * 0.5f);

	// Get third parabola point
	Vector thirdParabolaPoint = parabolaVertex + (parabolaEndDir * gravityAdjustedLength * 0.5f);

	// Vector for holding A B and C of parabola function: y = A*x*x + B*x + C
	Vector parabolaConstants;

	// Decide wether to use x,z or y,z for parabola calculations
	bool fUseYForParabolaX = fabs(parabolaDir.y) > fabs(parabolaDir.x);
	if (fUseYForParabolaX)
	{
		UTIL_ParabolaFromPoints(Vector2D(parabolaStartPos.y, parabolaStartPos.z), Vector2D(parabolaVertex.y, parabolaVertex.z), Vector2D(thirdParabolaPoint.y, thirdParabolaPoint.z), parabolaConstants);
	}
	else
	{
		UTIL_ParabolaFromPoints(Vector2D(parabolaStartPos.x, parabolaStartPos.z), Vector2D(parabolaVertex.x, parabolaVertex.z), Vector2D(thirdParabolaPoint.x, thirdParabolaPoint.z), parabolaConstants);
	}

	// Get length of parabola flattened into horizontal plane (linear projection)
	float parabolaFlattenedLength = (thirdParabolaPoint - parabolaStartPos).Length();
	float parabolaFlattenedSegmentLength = parabolaFlattenedLength / VR_TELEPORT_PARABOLA_BEAM_SEGMENT_COUNT;

	// Get parabola linear projection direction vector
	Vector parabolaFlattenedLinearDirection = (thirdParabolaPoint - parabolaStartPos).Normalize();

	// Ensure enough segments
	if (int(vr_teleParabola.segments.size()) < vr_teleportParabolaSegmentCounter + VR_TELEPORT_PARABOLA_BEAM_SEGMENT_COUNT)
	{
		vr_teleParabola.segments.resize(vr_teleportParabolaSegmentCounter + VR_TELEPORT_PARABOLA_BEAM_SEGMENT_COUNT);
	}

	// Get positions of parabola beam segments along linear parabola projection with parabola function
	for (int i = 0; i < VR_TELEPORT_PARABOLA_BEAM_SEGMENT_COUNT; i++)
	{
		vr_teleParabola.lastSegment = vr_teleportParabolaSegmentCounter + i;

		// If necessary create the beam, and enable it
		ParabolaSegment& currentSegment = vr_teleParabola.segments[vr_teleportParabolaSegmentCounter + i];
		if (!currentSegment.beam)
		{
			CBeam* pCurrentSegmentBeam = CBeam::BeamCreate("sprites/xbeam1.spr", 20);
			pCurrentSegmentBeam->Spawn();
			pCurrentSegmentBeam->PointsInit(pPlayer->pev->origin, pPlayer->pev->origin);
			pCurrentSegmentBeam->pev->owner = pPlayer->edict();
			currentSegment.beam = pCurrentSegmentBeam;
		}
		currentSegment.beam->pev->effects &= ~EF_NODRAW;

		// Get positions on linear projection
		Vector linearPos1 = parabolaStartPos + (parabolaFlattenedLinearDirection * i * parabolaFlattenedSegmentLength);
		Vector linearPos2 = parabolaStartPos + (parabolaFlattenedLinearDirection * (i + 1) * parabolaFlattenedSegmentLength);

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

		if (i == VR_TELEPORT_PARABOLA_BEAM_SEGMENT_COUNT - 1)
		{
			// If this is the last beam segment, extend it to "infinity"
			Vector dir = (beamSegmentPos2 - beamSegmentPos1).Normalize();
			beamSegmentPos2 = beamSegmentPos1 + (dir * 8192.f);
		}

		// Now do a trace to see if we hit something!
		TraceResult tr;
		VRPhysicsHelper::Instance().TraceLine(beamSegmentPos1, beamSegmentPos2, pPlayer->edict(), &tr);

		// Check if we teleport into a trigger_teleport
		vr_hTriggerTeleport = CheckTriggerTeleport(pPlayer, beamSegmentPos1, tr.vecEndPos);

		if (tr.flFraction < 1.0f || vr_hTriggerTeleport)
		{
			// We hit something, update beam segment end position
			beamSegmentPos2 = tr.vecEndPos;

			// Hide all further beams (and increment i, so this loop exits)
			for (i++; i < VR_TELEPORT_PARABOLA_BEAM_SEGMENT_COUNT; i++)
			{
				ParabolaSegment& segment = vr_teleParabola.segments[vr_teleportParabolaSegmentCounter + i];
				if (segment.beam)
				{
					segment.beam->pev->effects |= EF_NODRAW;
				}
			}
		}

		// Set beam segment positions
		currentSegment.beam->SetStartPos(beamSegmentPos1);
		currentSegment.beam->SetEndPos(beamSegmentPos2);
		currentSegment.startPoint = beamSegmentPos1;
		currentSegment.endPoint = beamSegmentPos2;
		currentSegment.tr = tr;
		vr_teleParabola.endPoint = beamSegmentPos2;
	}

	vr_teleportParabolaSegmentCounter += VR_TELEPORT_PARABOLA_BEAM_SEGMENT_COUNT;
}

Vector CalculateXenMoundParabolaDir(const Vector& beamDir)
{
	// Get direction of parabola at beam end pos
	Vector parabolaDir = beamDir;
	parabolaDir.z = -parabolaDir.z * 4;  // Invert and make steeper
	parabolaDir = parabolaDir.Normalize();
	return parabolaDir;
}

VRControllerTeleporter::TeleportResult VRControllerTeleporter::TeleportParabola(CBasePlayer* pPlayer, const Vector& telePos, const Vector& teleDir, float length, Vector& teleportDestination, bool& needsToDuck)
{
	// Set this to false, in case we won't hit anything in the parabola loop (parabola goes into empty space), thus the teleport destination couldn't be determined and is invalid
	vr_teleportResult = TeleportResult::INVALID;
	needsToDuck = false;

	ExtendTeleportParabola(pPlayer, telePos, teleDir, length);

	teleportDestination = vr_teleParabola.endPoint;
	ParabolaSegment& lastSegment = vr_teleParabola.segments[vr_teleParabola.lastSegment];
	vr_teleportResult = CanTeleportHere(pPlayer, lastSegment.tr, lastSegment.startPoint, lastSegment.endPoint, teleportDestination, needsToDuck);
	return vr_teleportResult;
}

void VRControllerTeleporter::EnableXenMoundParabolaAndUpdateTeleDestination(CBasePlayer* pPlayer, const Vector& beamStartPos, const Vector& beamEndPos, Vector& teleportDestination, bool& needsToDuck)
{
	// Get direction of beam and clamp at -0.1f z direction
	Vector beamDir = (beamEndPos - beamStartPos).Normalize();
	beamDir.z = min(beamDir.z, -0.1f);
	beamDir = beamDir.Normalize();

	vr_teleportResult = TeleportParabola(pPlayer, beamEndPos, CalculateXenMoundParabolaDir(beamDir), VR_TELEPORT_XEN_MOUND_PARABOLA_LENGTH, teleportDestination, needsToDuck);

	// Don't hit xen mounds multiple times
	if (vr_teleportResult == TeleportResult::XEN_MOUND)
	{
		vr_teleportResult = TeleportResult::VALID;
	}

	vr_teleportedXenMoundOrigins.push_back(beamEndPos);
}

void VRControllerTeleporter::ColorTeleporter(const Vector& color)
{
	for (auto& segment : vr_teleParabola.segments)
	{
		if (segment.beam)
		{
			segment.beam->pev->rendercolor = color;
		}
	}
	vr_hTeleSprite->pev->rendercolor = color;
}
