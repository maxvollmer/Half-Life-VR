
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "doors.h"

#include "VRRotatableEnt.h"

Vector VRRotatableEnt::GetUpdatedAngles(float delta)
{
	Vector angles = m_vrRotateStartAngles;
	if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_Z)) 
	{
		angles.z += delta;
	}
	else if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_X))
	{
		angles.x += delta;
	}
	else  // Y
	{
		angles.y += delta;
	}

	return angles;
}

Vector2D VRRotatableEnt::CalculateDeltaPos2D(const Vector& pos)
{
	if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_Z))
	{
		return Vector2D{ pos.y, pos.z }; // z is roll, around x-axis
	}
	else if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_X))
	{
		return Vector2D{ pos.x, pos.z };  // x is pitch, around y-axis
	}
	else  // Y
	{
		return Vector2D{ pos.x, pos.y };  // y is yaw, around z-axis
	}
}

float VRRotatableEnt::CalculateAngleDelta(const Vector& currentRotatePos)
{
	bool backwards = false;
	const char* axis = "x";
	if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_Z))
	{
		axis = "z";
	}
	else if (FBitSet(MyEntityPointer()->pev->spawnflags, SF_DOOR_ROTATE_X))
	{
		axis = "y";
		backwards = true;
	}

	Vector2D startPos2D = CalculateDeltaPos2D(m_vrRotateStartPos).Normalize();
	Vector2D currentPos2D = CalculateDeltaPos2D(currentRotatePos).Normalize();

	float cosAngle = startPos2D.x * currentPos2D.x + startPos2D.y * currentPos2D.y;
	float sinAngle = startPos2D.x * currentPos2D.y - startPos2D.y * currentPos2D.x;
	float angle = atan2f(sinAngle, cosAngle) * 180.f / M_PI;

	return backwards ? -angle : angle;
}

void VRRotatableEnt::VRRotate(CBaseEntity* pPlayer, const Vector& pos, bool fStart)
{
	Vector angleStart;
	Vector angleEnd;
	if (CanDoVRDragRotation(pPlayer, angleStart, angleEnd))
	{
		if (fStart)
		{
			m_vrRotateStartPos = pos - MyEntityPointer()->pev->origin;
			m_vrRotateStartAngles = MyEntityPointer()->pev->angles;
			m_vrLastWishAngleTime = gpGlobals->time;
			m_vrWishAngleDelta = 0.f;
			StartVRDragRotation();
		}
		else
		{
			// don't do multiple updates in the same frame
			float timeSinceLasttime = gpGlobals->time - m_vrLastWishAngleTime;
			m_vrLastWishAngleTime = gpGlobals->time;
			if (timeSinceLasttime <= 0.f)
				return;

			m_vrLastWishAngleTime = gpGlobals->time;

			float wishAngleDelta = CalculateAngleDelta(pos - MyEntityPointer()->pev->origin);

			UpdateAngles(pPlayer, GetUpdatedAngles(m_vrWishAngleDelta + wishAngleDelta), angleStart, angleEnd);

			if (fabs(wishAngleDelta) > 10)
			{
				m_vrRotateStartPos = pos - MyEntityPointer()->pev->origin;
				m_vrWishAngleDelta += wishAngleDelta;
			}
		}
	}
}

void VRRotatableEnt::UpdateAngles(CBaseEntity* pPlayer, const Vector& newAngles, const Vector& angleStart, const Vector& angleEnd)
{
	float delta = CBaseToggle::AxisDelta(MyEntityPointer()->pev->spawnflags, newAngles, angleStart)
		/ CBaseToggle::AxisDelta(MyEntityPointer()->pev->spawnflags, angleEnd, angleStart);

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
		MyEntityPointer()->pev->angles = newAngles;
		m_draggingCancelled = !SetVRDragRotation(pPlayer, newAngles, delta);
	}
}

void VRRotatableEnt::VRStopRotate()
{
	StopVRDragRotation();
}
