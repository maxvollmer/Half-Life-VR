
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

void VRGameFunctions::PrintToConsole(const char* s)
{
	gEngfuncs.Con_DPrintf(s);
}

void VRGameFunctions::SetSkill(int skill)
{
	gEngfuncs.Cvar_SetValue("skill", (float)skill);
	//gEngfuncs.pfnClientCmd("skill " + skill);
	//gEngfuncs.pfnServerCmd("skill " + skill);
}

void VRGameFunctions::SetGraphicsMode(int graphics)
{
	switch (graphics)
	{
	case 1:
		// classic mode
		gEngfuncs.Cvar_SetValue("vr_classic_mode", 1);
		gEngfuncs.Cvar_SetValue("vr_hd_textures_enabled", 0);
		gEngfuncs.Cvar_SetValue("vr_use_hd_models", 0);
		gEngfuncs.pfnClientCmd("gl_texturemode GL_NEAREST");
		gEngfuncs.pfnClientCmd("vr_texturemode GL_NEAREST");
		break;
	case 2:
		// SD mode
		gEngfuncs.Cvar_SetValue("vr_classic_mode", 0);
		gEngfuncs.Cvar_SetValue("vr_hd_textures_enabled", 0);
		gEngfuncs.Cvar_SetValue("vr_use_hd_models", 0);
		gEngfuncs.pfnClientCmd("gl_texturemode GL_LINEAR_MIPMAP_LINEAR");
		gEngfuncs.pfnClientCmd("vr_texturemode GL_LINEAR_MIPMAP_LINEAR");
		break;
	case 3:
		// HD mode
		gEngfuncs.Cvar_SetValue("vr_classic_mode", 0);
		gEngfuncs.Cvar_SetValue("vr_hd_textures_enabled", 1);
		gEngfuncs.Cvar_SetValue("vr_use_hd_models", 1);
		gEngfuncs.pfnClientCmd("gl_texturemode GL_LINEAR_MIPMAP_LINEAR");
		gEngfuncs.pfnClientCmd("vr_texturemode GL_LINEAR_MIPMAP_LINEAR");
		break;
	}
}

void VRGameFunctions::SetMovement(int movement)
{
	switch (movement)
	{
	case 1:
		// off hand
		gEngfuncs.Cvar_SetValue("vr_movement_attachment", 0);
		break;
	case 2:
		// weapon hand
		gEngfuncs.Cvar_SetValue("vr_movement_attachment", 1);
		break;
	case 3:
		// HMD
		gEngfuncs.Cvar_SetValue("vr_movement_attachment", 2);
		break;
	}
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

void VRGameFunctions::OpenMenu()
{
	gEngfuncs.pfnClientCmd("toggleconsole");
}

void VRGameFunctions::CloseMenu()
{
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

float VRGameFunctions::GetCVar(const char* cvar)
{
	return gEngfuncs.pfnGetCvarFloat(cvar);
}

void VRGameFunctions::SetCVar(const char* cvar, float value)
{
	gEngfuncs.Cvar_SetValue(cvar, value);
}
