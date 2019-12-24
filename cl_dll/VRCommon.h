#pragma once

#include <filesystem>

enum class VRPoseType
{
	FLASHLIGHT,
	MOVEMENT,
	TELEPORTER
};

inline std::filesystem::path GetPathFor(const std::string& file)
{
	std::filesystem::path relativePath = gEngfuncs.pfnGetGameDirectory() + file;
	return std::filesystem::absolute(relativePath);
}
