#pragma once

// Common defines and types for VR - Max Vollmer - 2019-04-07

#include <map>
#include "vector.h"

typedef int string_t;
typedef int BOOL;
class CBasePlayer;

enum class VRControllerID : int32_t
{
	WEAPON = 0,
	HAND,
	INVALID
};

#define VR_MUZZLE_ATTACHMENT 0

#include "VRStuff.h"

extern GlobalXenMounds gGlobalXenMounds;
extern VRLevelChangeData g_vrLevelChangeData;
