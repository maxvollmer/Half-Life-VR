#pragma once

#include <unordered_map>
#include <string>
#include <chrono>


class VRSettings
{
public:
	void Init();

	void InitialUpdateCVARSFromFile();

	void CheckCVARsForChanges();

private:
	enum class RetryMode
	{
		NONE,
		WRITE,
		LOAD
	};

	void RegisterCVAR(const char* name, const char* value);
	void UpdateCVARSFromFile();
	void UpdateFileFromCVARS();
	bool WasSettingsFileChanged();
	bool WasAnyCVARChanged();

	std::unordered_map<std::string, std::string> m_cvarCache;
	void* m_watchVRFolderHandle{ nullptr };
	RetryMode m_needsRetry{ RetryMode::NONE };
	long long m_lastSettingsFileChangedTime{ 0 };
	long long m_nextSettingCheckTime{ 0 };

	void UpdateTextureMode();
	std::string m_gl_texturemode;
	std::string m_vr_texturemode;
};

extern VRSettings g_vrSettings;
