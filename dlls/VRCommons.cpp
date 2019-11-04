
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

bool GlobalXenMounds::Trigger(CBasePlayer* pPlayer, const Vector& position)
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

bool VRGlobalGetNoclipMode()
{
	return CVAR_GET_FLOAT("sv_cheats") != 0.f && CVAR_GET_FLOAT("vr_noclip") != 0.f;
}

bool VRGlobalIsPointInsideEnt(const float* point, int ent)
{
	if (FNullEnt(INDEXENT(ent)))
		return false;

	return UTIL_PointInsideRotatedBBox(INDEXENT(ent)->v.origin, INDEXENT(ent)->v.angles, INDEXENT(ent)->v.mins, INDEXENT(ent)->v.maxs, Vector{ point[0], point[1], point[2] });
}

float VRGetMaxClimbSpeed()
{
	return CVAR_GET_FLOAT("vr_ladder_legacy_movement_speed");
}

bool VRIsLegacyLadderMoveEnabled()
{
	return CVAR_GET_FLOAT("vr_ladder_legacy_movement_enabled") != 0.f;
}

bool VRGetMoveOnlyUpDownOnLadder()
{
	return CVAR_GET_FLOAT("vr_ladder_legacy_movement_only_updown") != 0.f;
}

int VRGetGrabbedLadder(int player)
{
	if (CVAR_GET_FLOAT("vr_ladder_immersive_movement_enabled") != 0.f)
	{
		CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(UTIL_PlayerByIndex(player + 1));
		if (pPlayer == nullptr)
			return -1;

		return pPlayer->GetGrabbedLadderEntIndex();
	}
	else
	{
		return -1;
	}
}

bool VRIsPullingOnLedge(int player)
{
	float vr_ledge_pull_mode = CVAR_GET_FLOAT("vr_ledge_pull_mode");
	if (vr_ledge_pull_mode != 1.f && vr_ledge_pull_mode != 2.f)
		return false;

	CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(UTIL_PlayerByIndex(player + 1));
	if (pPlayer == nullptr)
		return false;

	return pPlayer->m_vrIsPullingOnLedge;
}

bool VRIsAutoDuckingEnabled(int player)
{
	return CVAR_GET_FLOAT("vr_autocrouch_enabled") != 0.f;
}

float VRGetSmoothStepsSetting()
{
	return CVAR_GET_FLOAT("vr_smooth_steps");
}
