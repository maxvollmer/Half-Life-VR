#pragma once

#include "VRCommons.h"
#include "vector.h"
#include "eiface.h"

constexpr const int XEN_MOUND_MAX_TRIGGER_DISTANCE = 128;
constexpr const int VR_TELEPORT_MAX_DOWNWARDS_TELE_LENGTH = 512;
constexpr const int VR_TELEPORT_PARABOLA_LENGTH = 80;
constexpr const int VR_TELEPORT_XEN_MOUND_PARABOLA_LENGTH = 1024;
constexpr const int VR_TELEPORT_PARABOLA_BEAM_SEGMENT_COUNT = 11; // Always keep this an uneven number

class VRController;
class CSprite;
class CBeam;

class VRControllerTeleporter
{
public:
	void StartTele(CBasePlayer* pPlayer, const Vector& telePos);
	void StopTele(CBasePlayer* pPlayer);
	void UpdateTele(CBasePlayer* pPlayer, const Vector& telePos, const Vector& teleDir);

private:
	enum class TeleportResult
	{
		INVALID,
		VALID,
		WATER,
		XEN_MOUND,
		TRIGGER_TELEPORT
	};

	struct ParabolaSegment
	{
		EHANDLE<CBeam> beam;
		Vector startPoint;
		Vector endPoint;
		TraceResult tr;
	};

	struct Parabola
	{
		std::vector<ParabolaSegment> segments;
		Vector endPoint;
		int lastSegment{ 0 };
	};

	TeleportResult TeleportParabola(CBasePlayer* pPlayer, const Vector& telePos, const Vector& teleDir, float length, Vector& teleportDestination, bool& needsToDuck);

	float GetCurrentTeleLength(CBasePlayer* pPlayer);

	void TouchTriggersInTeleportPath(CBasePlayer* pPlayer);  // Touches all entities with SOLID_TRIGGER when a player teleports through them
	bool TryTeleportInUpwardsTriggerPush(CBasePlayer* pPlayer, const Vector& beamStartPos, Vector& beamEndPos, Vector& teleportDestination);
	void EnableXenMoundParabolaAndUpdateTeleDestination(CBasePlayer* pPlayer, const Vector& beamStartPos, const Vector& beamEndPos, Vector& teleportDestination, bool& needsToDuck);

	void ExtendTeleportParabola(CBasePlayer* pPlayer, const Vector& parabolaStartPos, const Vector& parabolaDir, float length);
	void ClearTeleportParabola();
	float GetDownwardsParabolaLength();

	void ColorTeleporter(const Vector& color);

	TeleportResult CanTeleportHere(CBasePlayer* pPlayer, const TraceResult& tr, const Vector& beamStartPos, Vector& beamEndPos, Vector& teleportSpritePos, bool& needsToDuck);

	EHANDLE<CSprite> vr_hTeleSprite;
	Parabola vr_teleParabola;
	std::vector<Vector> vr_teleportedXenMoundOrigins;

	EHANDLE<CBaseEntity> vr_hTriggerTeleport;

	TeleportResult vr_teleportResult = TeleportResult::INVALID;
	Vector vr_vecTeleDestination;
	bool vr_needsToDuckAfterTeleport = false;
	int vr_teleportParabolaSegmentCounter = 0;
};
