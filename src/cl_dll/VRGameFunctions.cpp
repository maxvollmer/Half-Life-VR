
#include "wrect.h"
#include "cl_dll.h"

#include <cstdlib>
#include <chrono>
#include <filesystem>

#include "VRGameFunctions.h"
#include "VRCommon.h"

namespace
{
	bool quickSaveExists = false;
	long long lastTimeQuickSaveChecked = 0;
}

void VRGameFunctions::SetSkill(int skill)
{
	gEngfuncs.Cvar_SetValue("skill", (float)skill);
	//gEngfuncs.pfnClientCmd("skill " + skill);
	//gEngfuncs.pfnServerCmd("skill " + skill);
}

void VRGameFunctions::SetVolume(float volume)
{
	gEngfuncs.Cvar_SetValue("volume", volume);
	//gEngfuncs.pfnClientCmd("volume " + volume);
}

void VRGameFunctions::StartNewGame(bool skipTrainRide)
{
	gEngfuncs.pfnClientCmd(skipTrainRide ? "map c1a0" : "map c0a0");
}

void VRGameFunctions::StartHazardCourse()
{
	gEngfuncs.pfnClientCmd("map t0a0");
}

void VRGameFunctions::CloseMenu()
{
	// TODO: Not sure if this actually works
	gEngfuncs.pfnClientCmd("escape");
}

void VRGameFunctions::QuickLoad()
{
	gEngfuncs.pfnClientCmd("load quick");
}

void VRGameFunctions::QuickSave()
{
	gEngfuncs.pfnClientCmd("save quick");
}

void VRGameFunctions::QuitGame()
{
	std::exit(0);
}

bool VRGameFunctions::QuickSaveExists()
{
	if (!quickSaveExists)
	{
		long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (lastTimeQuickSaveChecked < (currentTime - 1000))
		{
			lastTimeQuickSaveChecked = currentTime;
			std::filesystem::path quickSavePath = GetPathFor("/save/quick.sav");
			quickSaveExists = std::filesystem::exists(quickSavePath);
		}
	}

	return quickSaveExists;
}
