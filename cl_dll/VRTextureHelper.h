#pragma once

#include <unordered_map>
#include <string>
#include <array>

class VRTextureHelper
{
public:
	unsigned int GetTexture(const std::string& name);
	unsigned int GetHDGameTexture(const std::string& name);
	unsigned int GetSkyboxTexture(const std::string& name, int index);
	unsigned int GetHDSkyboxTexture(const std::string& name, int index);

	const char* GetSkyboxNameFromMapName(const std::string& mapName);

	static VRTextureHelper& Instance() { return instance; }

private:
	VRTextureHelper() {}

	static VRTextureHelper	instance;

	std::unordered_map<std::string, unsigned int>		m_textures;

	static const std::unordered_map<std::string, std::string>		m_mapSkyboxNames;
	static const std::array<std::string, 6>							m_mapSkyboxIndices;
};
