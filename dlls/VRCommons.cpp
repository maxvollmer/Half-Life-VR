
//#include "VRCommons.h"

#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"

VRLevelChangeData g_vrLevelChangeData;

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

GlobalXenMounds gGlobalXenMounds;
