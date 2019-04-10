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
	static constexpr const int VR_MIN_VALID_LIFT_OR_TRAIN_WIDTH = 64;	// Special handling of entities that are less than 64x64 in horizontal size

	void DetectAndSetGroundEntity();
	bool CheckIfPotentialGroundEntityForPlayer(CBaseEntity *pEntity);
	CBaseEntity* ChoseBetterGroundEntityForPlayer(CBaseEntity *pEntity1, CBaseEntity *pEntity2);
	void SendGroundEntityToClient();
	void MoveWithGroundEntity();
	Vector CalculateNewOrigin();
	bool CheckIfNewOriginIsValid(const Vector& newOrigin);

	CBasePlayer*		m_pPlayer{ nullptr };
	EHANDLE				m_hGroundEntity;
	bool				m_isTouchingGroundEntityFloor{ false };

	/*
	Vector m_initialPlayerOffsetToGroundEntity;
	Vector m_lastPlayerOrigin;
	Vector m_initialGroundEntityAngles;
	Vector m_initialGroundEntityOrigin;
	*/

	Vector m_lastGroundEntityOrigin;
	Vector m_lastGroundEntityAngles;
	//Vector m_lastGroundEntityOffset;
};
