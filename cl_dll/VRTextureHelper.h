#pragma once

#include <unordered_map>
#include <string>
#include <array>
#include <filesystem>

class VRTextureHelper
{
public:
	void Init();

	unsigned int GetTexture(const std::string& name);
	unsigned int GetTexture(const std::string& name, unsigned int& width, unsigned int& height);
	unsigned int GetHDGameTexture(const std::string& name);
	unsigned int GetVRSkyboxTexture(const std::string& name, int index);
	unsigned int GetVRHDSkyboxTexture(const std::string& name, int index);
	unsigned int GetHLSkyboxTexture(const std::string& name, int index);
	unsigned int GetHLHDSkyboxTexture(const std::string& name, int index);

	const char* GetSkyboxNameFromMapName(const std::string& mapName);
	const char* GetCurrentSkyboxName();

	static VRTextureHelper& Instance() { return instance; }

private:
	void PreloadAllTextures(const std::filesystem::path& path);
	unsigned int GetTextureInternal(const std::filesystem::path& path, unsigned int& width, unsigned int& height);

	struct Texture
	{
		unsigned int texnum = 0;
		unsigned int width = 0;
		unsigned int height = 0;
	};

	VRTextureHelper() {}

	static VRTextureHelper instance;

	std::unordered_map<std::string, Texture> m_textures;

	static const std::unordered_map<std::string, std::string> m_mapSkyboxNames;
	static const std::array<std::string, 6> m_vrMapSkyboxIndices;
	static const std::array<std::string, 6> m_hlMapSkyboxIndices;

	bool m_isInitialized{ false };

	void UpdateTextureMode();
	std::string m_lastTextureMode;
	int m_glTexMinFilter = 0;
	int m_glTexMagFilter = 0;
};
