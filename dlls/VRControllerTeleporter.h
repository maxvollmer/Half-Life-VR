#pragma once

#include "VRCommons.h"
#include "vector.h"
#include "eiface.h"

class VRController;
class CSprite;
class CBeam;

class VRControllerTeleporter
{
public:
	void StartTele(CBasePlayer *pPlayer, const VRController& controller);
	void StopTele(CBasePlayer *pPlayer, const VRController& controller);
	void UpdateTele(CBasePlayer *pPlayer, const VRController& controller);

private:
	void TouchTriggersInTeleportPath(CBasePlayer *pPlayer); // Touches all entities with SOLID_TRIGGER when a player teleports through them
	bool CanTeleportHere(CBasePlayer *pPlayer, const TraceResult& tr, const Vector& beamStartPos, Vector& beamEndPos, Vector& teleportSpritePos);
	void EnableXenMoundParabolaAndUpdateTeleDestination(CBasePlayer *pPlayer, const Vector& beamStartPos, const Vector& beamEndPos, Vector & teleportDestination);
	void DisableXenMoundParabola();
	float GetCurrentTeleLength(CBasePlayer *pPlayer);

	CSprite* vr_pTeleSprite = nullptr;
	CBeam* vr_pTeleBeam = nullptr;
	CBeam* vr_parabolaBeams[VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT] = { nullptr };
	bool vr_fValidTeleDestination = false;
	bool vr_fTelePointsAtXenMound = false;
	bool vr_fTelePointsInWater = false;
	Vector vr_vecTeleDestination;
};

