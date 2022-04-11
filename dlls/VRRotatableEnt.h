#pragma once

class CBaseEntity;

class VRRotatableEnt
{
public:
	void VRRotate(CBaseEntity* pPlayer, const Vector& pos, bool fStart);
	void VRStopRotate();
	bool IsDraggingCancelled() { return m_draggingCancelled; }
	void ClearDraggingCancelled() { m_draggingCancelled = false; }

protected:
	virtual bool CanDoVRDragRotation(CBaseEntity* pPlayer, Vector& angleStart, Vector& angleEnd) = 0;
	virtual void StartVRDragRotation() = 0;
	virtual bool SetVRDragRotation(CBaseEntity* pPlayer, const Vector& angles, float delta) = 0;
	virtual void StopVRDragRotation() = 0;
	virtual float GetVRDragRotationMoveDistance() = 0;
	virtual CBaseEntity* MyEntityPointer() = 0;

private:
	Vector2D CalculateDeltaPos2D(const Vector& pos);
	float CalculateAngleDelta(const Vector& currentRotatePos);
	Vector GetUpdatedAngles(float wishDeltaAngle);
	void UpdateAngles(CBaseEntity* pPlayer, const Vector& wishAngles, const Vector& angleStart, const Vector& angleEnd);

	Vector m_vrRotateStartPos{ 0.f, 0.f, 0.f };
	Vector m_vrRotateStartAngles{ 0.f, 0.f, 0.f };
	float m_vrLastWishAngleTime{ 0.f };
	float m_vrWishAngleDelta{ 0.f };
	bool m_draggingCancelled{ false };
};
