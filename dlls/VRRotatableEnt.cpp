
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "doors.h"

#include "VRRotatableEnt.h"

constexpr const inline void NormalizeAngle(float& angle)
{
	while (angle > 360.f) angle -= 360.f;
	while (angle < 0.f) angle += 360.f;
}

constexpr const inline void NormalizeAngles(Vector& angles)
{
	NormalizeAngle(angles.x);
	NormalizeAngle(angles.y);
	NormalizeAngle(angles.z);
}

constexpr const inline void ClampDeltaAngle(float& angle)
{
	while (angle > 180.f) angle -= 360.f;
	while (angle < -180.f) angle += 360.f;
}

constexpr const inline float MoveIntoRange(float angle, float angleStart)
{
	while (angle < angleStart)
		angle += 360.f;
	return angle;
}

const inline Vector MoveIntoRange(const Vector& angles, const Vector& angleStart)
{
	return Vector{
		MoveIntoRange(angles.x, angleStart.x),
		MoveIntoRange(angles.y, angleStart.y),
		MoveIntoRange(angles.z, angleStart.z)
	};
}

constexpr const inline float ForcePositiveAngles(float& angleStart, float& angleEnd)
{
	float delta = 0.f;
	while (angleStart < 0.f || angleEnd < 0.f)
	{
		angleStart += 360.f;
		angleEnd += 360.f;
		delta += 360.f;
	}
	return delta;
}

const inline Vector ForcePositiveAngles(Vector& anglesStart, Vector& anglesEnd)
{
	return Vector{
		ForcePositiveAngles(anglesStart.x, anglesEnd.x),
		ForcePositiveAngles(anglesStart.y, anglesEnd.y),
		ForcePositiveAngles(anglesStart.z, anglesEnd.z)
	};
}

Vector VRRotatableEnt::GetWishAngles(float wishDeltaAngle)
{
	Vector wishAngles = m_vrRotateStartAngles;
	if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_Z))
	{
		wishAngles.z += wishDeltaAngle;
	}
	else if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_X))
	{
		wishAngles.x += wishDeltaAngle;
	}
	else  // Y
	{
		wishAngles.y += wishDeltaAngle;
	}
	return wishAngles;
}

Vector2D VRRotatableEnt::CalculateDeltaPos2D(const Vector& pos)
{
	if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_Z))
	{
		return Vector2D{ pos.y, pos.z };
	}
	else if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_X))
	{
		return Vector2D{ pos.x, pos.z };
	}
	else  // Y
	{
		return Vector2D{ pos.x, pos.y };
	}
}

float VRRotatableEnt::CalculateDeltaAngle(const Vector& currentRotatePos)
{
	Vector2D startPos2D = CalculateDeltaPos2D(m_vrRotateStartPos).Normalize();
	Vector2D currentPos2D = CalculateDeltaPos2D(currentRotatePos).Normalize();
	float startAngle = atan2f(startPos2D.y, startPos2D.x) * 180.f / M_PI;
	float currentAngle = atan2f(currentPos2D.y, currentPos2D.x) * 180.f / M_PI;
	return currentAngle - startAngle;
}

Vector VRRotatableEnt::CalculateAnglesFromWishAngles(const Vector& wishAngles, const Vector& angleStart, const Vector& angleEnd, float maxRotSpeed)
{
	Vector anglesInRange = MoveIntoRange(MyEntityPointer()->pev->angles, angleStart.Length() < angleEnd.Length() ? angleStart : angleEnd);

	float timeSinceLasttime = gpGlobals->time - m_vrLastWishAngleTime;
	m_vrLastWishAngleTime = gpGlobals->time;
	if (timeSinceLasttime <= 0.f)
		return anglesInRange;

	Vector angleDiff = wishAngles - anglesInRange;
	float angleDiffLength = angleDiff.Length();
	if (angleDiffLength <= (timeSinceLasttime * maxRotSpeed))
	{
		return wishAngles;
	}
	else
	{
		return anglesInRange + angleDiff.Normalize() * timeSinceLasttime * maxRotSpeed;
	}
}

float VRRotatableEnt::CalculateAngleDelta(const Vector& angles, const Vector& angleStart, const Vector& angleEnd)
{
	// CalculateAngleDelta: 0.582798; actualAngles: 0.000000, 0.000000, 26.225897; angleStart: 0.000000, 0.000000, 0.000000; angleEnd: 0.000000, 0.000000, -45.000000
	if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_Z))
	{
		return (angles.z - angleStart.z) / (angleEnd.z - angleStart.z);
	}
	else if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_X))
	{
		return (angles.x - angleStart.x) / (angleEnd.x - angleStart.x);
	}
	else  // Y
	{
		return (angles.y - angleStart.y) / (angleEnd.y - angleStart.y);
	}
}

void VRRotatableEnt::VRRotate(CBaseEntity* pPlayer, const Vector& pos, bool fStart)
{
	Vector angleStart;
	Vector angleEnd;
	float maxRotSpeed = 0.f;
	if (CanDoVRDragRotation(pPlayer, angleStart, angleEnd, maxRotSpeed))
	{
		Vector delta = ForcePositiveAngles(angleStart, angleEnd);

		if (fStart)
		{
			NormalizeAngles(MyEntityPointer()->pev->angles);
			m_vrRotateStartPos = pos - MyEntityPointer()->pev->origin;
			m_vrRotateStartAngles = MyEntityPointer()->pev->angles + delta;
			m_vrWishDeltaAngle = 0.f;
			m_vrLastWishAngleTime = gpGlobals->time;
			StartVRDragRotation();
		}
		else
		{
			float wishDeltaAngle = CalculateDeltaAngle(pos - MyEntityPointer()->pev->origin);
			ClampDeltaAngle(wishDeltaAngle);
			if (fabs(wishDeltaAngle) > 45)
			{
				m_vrRotateStartPos = pos - MyEntityPointer()->pev->origin;
				m_vrWishDeltaAngle += wishDeltaAngle;
				wishDeltaAngle = 0.f;
			}
			FollowWishDeltaAngle(pPlayer, GetWishAngles(m_vrWishDeltaAngle + wishDeltaAngle), angleStart, angleEnd, maxRotSpeed);
		}
	}
}

void VRRotatableEnt::FollowWishDeltaAngle(CBaseEntity* pPlayer, const Vector& wishAngles, const Vector& angleStart, const Vector& angleEnd, float maxRotSpeed)
{
	Vector actualAngles = CalculateAnglesFromWishAngles(wishAngles, angleStart, angleEnd, maxRotSpeed);
	float delta = CalculateAngleDelta(actualAngles, angleStart, angleEnd);
	if (delta <= 0.f)
	{
		MyEntityPointer()->pev->angles = angleStart;
		m_draggingCancelled = !SetVRDragRotation(pPlayer, angleStart, 0.f);
	}
	else if (delta >= 1.f)
	{
		MyEntityPointer()->pev->angles = angleEnd;
		m_draggingCancelled = !SetVRDragRotation(pPlayer, angleEnd, 1.f);
	}
	else
	{
		MyEntityPointer()->pev->angles = actualAngles;
		m_draggingCancelled = !SetVRDragRotation(pPlayer, actualAngles, delta);
	}
}

void VRRotatableEnt::VRStopRotate()
{
	StopVRDragRotation();
}
