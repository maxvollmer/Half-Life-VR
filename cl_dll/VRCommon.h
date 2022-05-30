#pragma once

#include <filesystem>

#include "../vr_shared/VRShared.h"
#include "hud_iface.h"

inline std::filesystem::path GetRelPathFor(const std::string& file)
{
	if (std::filesystem::exists(gEngfuncs.pfnGetGameDirectory() + file))
	{
		return gEngfuncs.pfnGetGameDirectory() + file;
	}
	else
	{
		return "valve" + file;
	}
}

inline std::filesystem::path GetPathFor(const std::string& file)
{
	std::filesystem::path relativePath = GetRelPathFor(file);
	return std::filesystem::absolute(relativePath);
}
