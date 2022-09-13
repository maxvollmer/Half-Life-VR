#pragma once

#include <unordered_set>
#include <string>

class CBasePlayer;

class VRGroundEntityHandler
{
public:
	VRGroundEntityHandler::VRGroundEntityHandler(CBasePlayer* pPlayer) :
		m_pPlayer{ pPlayer }
	{
	}

	void HandleMovingWithSolidGroundEntities();

private:
	static constexpr const int VR_MIN_VALID_LIFT_OR_TRAIN_WIDTH = 64;  // Special handling of entities that are less than 64x64 in horizontal size

	void DetectAndSetGroundEntity();
	bool CheckIfPotentialGroundEntityForPlayer(edict_t* pent);
	bool IsExcludedAsGroundEntity(edict_t* pent);
	edict_t* ChoseBetterGroundEntityForPlayer(edict_t* pent1, edict_t* pent2);
	void SendGroundEntityToClient();
	void MoveWithGroundEntity();
	Vector CalculateNewOrigin();
	bool CheckIfNewOriginIsValid(const Vector& newOrigin);

	EHANDLE<CBasePlayer> m_pPlayer;
	EHANDLE<CBaseEntity> m_hGroundEntity;

	Vector m_lastGroundEntityOrigin;
	Vector m_lastGroundEntityAngles;
};
