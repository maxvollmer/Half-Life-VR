#pragma once

#include <filesystem>

#include "../vr_shared/VRShared.h"

inline std::filesystem::path GetPathFor(const std::string& file)
{
	std::filesystem::path relativePath = gEngfuncs.pfnGetGameDirectory() + file;
	return std::filesystem::absolute(relativePath);
}
