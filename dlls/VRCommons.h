#pragma once

// Common defines and types for VR - Max Vollmer - 2019-04-07

#define VR_DUCK_START_HEIGHT 0
#define VR_DUCK_STOP_HEIGHT 20

#define XEN_MOUND_MAX_TRIGGER_DISTANCE 128
#define VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT 15  // Uneven amount makes the vertex look more smooth

#include <map>
#include "vector.h"

typedef int	string_t;
typedef int	BOOL;
class CBasePlayer;

enum class VRControllerID : int32_t {
	WEAPON = 0,
	HAND,
	INVALID
};

#include "VRStuff.h"

extern GlobalXenMounds gGlobalXenMounds;
extern VRLevelChangeData g_vrLevelChangeData;
