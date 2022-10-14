#pragma once

#include "../vr_shared/VRAchievements.h"

class VRSteamworksManager
{
public:
	static bool Init();

	static void Update();

	static void GiveAchievement(VRAchievement achievement);
};
