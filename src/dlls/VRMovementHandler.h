#pragma once

#include "vector.h"

class VRMovementHandler
{
public:
	static Vector DoMovement(const Vector& from, const Vector& to, CBaseEntity* pMovingEntityForTouch = nullptr);
};
