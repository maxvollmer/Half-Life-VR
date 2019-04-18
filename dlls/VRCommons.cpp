
//#include "VRCommons.h"

#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "VRPhysicsHelper.h"

VRLevelChangeData g_vrLevelChangeData;
GlobalXenMounds gGlobalXenMounds;

// TODO: Probably wanna move those somewhere else:

void GlobalXenMounds::Add(const Vector& position, const string_t multi_manager)
{
	m_xen_mounds.insert(std::make_pair(position, multi_manager));
}

bool GlobalXenMounds::Has(const Vector& position)
{
	for (auto m_xen_mound : m_xen_mounds)
	{
		if ((m_xen_mound.first - position).Length2D() <= XEN_MOUND_MAX_TRIGGER_DISTANCE)
		{
			return true;
		}
	}
	return false;
}

bool GlobalXenMounds::Trigger(CBasePlayer *pPlayer, const Vector& position)
{
	for (auto m_xen_mound : m_xen_mounds)
	{
		if ((m_xen_mound.first - position).Length2D() <= XEN_MOUND_MAX_TRIGGER_DISTANCE)
		{
			FireTargets(STRING(m_xen_mound.second), pPlayer, pPlayer, USE_TOGGLE, 0.0f);
			return true;
		}
	}

	return false;
}


// For pm_shared.cpp
bool VRGlobalIsInstantAccelerateOn()
{
	return CVAR_GET_FLOAT("vr_move_instant_accelerate") != 0.f;
}

bool VRGlobalIsInstantDecelerateOn()
{
	return CVAR_GET_FLOAT("vr_move_instant_decelerate") != 0.f;
}

void VRGlobalGetEntityOrigin(int ent, float* entorigin)
{
	((INDEXENT(ent)->v.absmin + INDEXENT(ent)->v.absmax) * 0.5f).CopyToArray(entorigin);
}

void VRGlobalGetWorldUnstuckDir(const float* pos, const float* velocity, float* unstuckdir)
{
	Vector vunstuckdir;
	VRPhysicsHelper::Instance().GetWorldUnstuckDir(Vector{ pos[0], pos[1], pos[2] }, Vector{ velocity[0], velocity[1], velocity[2] }, vunstuckdir);
	vunstuckdir.CopyToArray(unstuckdir);
}

bool VRGlobalGetNoclipMode()
{
	// TODO: Add something to enable noclip in VR
	return false;
}

bool VRGlobalIsPointInsideEnt(const float* point, int ent)
{
	Vector absmin = INDEXENT(ent)->v.absmin;
	Vector absmax = INDEXENT(ent)->v.absmax;
	return point[0] > absmin.x && point[0] < absmax.x
		&& point[1] > absmin.y && point[1] < absmax.y
		&& point[2] > absmin.z && point[2] < absmax.z;
}
