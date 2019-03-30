#pragma once

#define VR_DUCK_START_HEIGHT 0
#define VR_DUCK_STOP_HEIGHT 20

#define XEN_MOUND_MAX_TRIGGER_DISTANCE 128
#define VR_XEN_MOUND_PARABOLA_BEAM_SEGMENT_COUNT 15  // Uneven amount makes the vertex look more smooth

#include <map>
#include "vector.h"

typedef int	string_t;
typedef int	BOOL;
class CBasePlayer;

class GlobalXenMounds
{
public:
	void Add(const Vector& position, const string_t multi_manager);
	bool Trigger(CBasePlayer *pPlayer, const Vector& position);
	bool Has(const Vector& position);
	void Clear() { m_xen_mounds.clear(); }
private:
	std::map<const Vector, const string_t> m_xen_mounds;
};

extern GlobalXenMounds gGlobalXenMounds;

class VRLevelChangeData
{
public:
	Vector lastHMDOffset{};
	Vector clientOriginOffset{};	// Must be Vector instead of Vector2D, so save/load works (only has FIELD_VECTOR, which expects 3 floats)
	BOOL hasData{ false };			// Must be BOOL (int), so save/load works (only has FIELD_BOOLEAN, which expects int)
	float prevYaw{ 0.f };
	float currentYaw{ 0.f };
};

enum class VRControllerID : int32_t {
	WEAPON = 0,
	HAND
};

extern VRLevelChangeData g_vrLevelChangeData;
